#include "server.h"
#include "rtt.h"
#define WORKER_DEBUG
#ifdef  WORKER_DEBUG
#include <inttypes.h>
#endif
// Error result_f 

pthread_t ntid;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mesg_mutex = PTHREAD_MUTEX_INITIALIZER;
worker this;
int load, pre_load, proxyfd;
struct sockaddr_in proxyaddr;
request_t request;
response_t response;
request_t request_cli;
response_t response_cli;
typedef struct {
    unsigned long file_len;
	unsigned char md5[MD5_DIGEST_LENGTH];
} file_hdr;

void Dg_send_recv(int fd, const void *outbuff, size_t outbytes,
             void *inbuff, size_t inbytes,
             const struct sockaddr *destaddr, socklen_t destlen);

void *request_counter(void *arg)
{
    update_ack *update_ack_tmp;
    int status, diff, tmp_load;
    update *update_tmp;
#ifdef WORKER_DEBUG
    printf("Enter request_counter.\n");
#endif
    for ( ; ; ) {
        if (waitpid(-1, &status, 0) == -1) {
            continue;
        }
        
#ifdef WORKER_DEBUG
        printf("ending a request.\n");
#endif
        Pthread_mutex_lock(&count_mutex);
        load--;
        diff = load - pre_load;
        Pthread_mutex_unlock(&count_mutex);

        /* foward count to proxy */
        if ((diff > -7) && (diff < 7)) continue;

        update_tmp = (update *)&request;
        update_tmp->type = UPDT;
        update_tmp->index = this.index;
        update_tmp->passwd = this.passwd;

        Pthread_mutex_lock(&mesg_mutex);
        tmp_load = load;
        diff = tmp_load - pre_load;
        if ((diff <= -7) || (diff >= 7)) {
            update_tmp->load = tmp_load;
            Dg_send_recv(proxyfd, &request, sizeof(request), &response, 
                        sizeof(response), (SA *)&proxyaddr, sizeof(proxyaddr));
            update_ack_tmp = (update_ack *)&response;
            pre_load = tmp_load;
        }
        Pthread_mutex_unlock(&mesg_mutex);
    }
}


int main(int argc, char **argv)
{
    file_hdr data_hdr, result_hdr;
    char buf[BUFSIZE];
    ssize_t len;
    int listenfd, connfd, diff, tmp_load;
    long timestamp = random();
    pid_t childpid;
    pthread_t tid;
    struct sockaddr_in workeraddr;
    socklen_t workerlen;
    update *update_tmp;
    update_ack *update_ack_tmp;

    /* Option and config file */
    openlog(NULL,LOG_CONS|LOG_PID, LOG_LOCAL1);


    /* creat a worker */
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    Listen(listenfd, MAX_REQUESTS);

    workerlen = sizeof(workeraddr);
    Getsockname(listenfd, (SA *)&workeraddr, &workerlen);

    /* connect to server and register */
    proxyfd = Socket(AF_INET, SOCK_DGRAM, 0);
    memset(&proxyaddr, 0, sizeof(proxyaddr));
    proxyaddr.sin_family = AF_INET;
    proxyaddr.sin_port = htons(PROXY_PORT);
    inet_pton(AF_INET, PROXY_IP, &proxyaddr.sin_addr);

    regist *regist_tmp = (regist *)&request_cli;
    regist_tmp->type = RGST;
    regist_tmp->port = (uint16_t)workeraddr.sin_port;
    regist_tmp->addr = (uint32_t)workeraddr.sin_addr.s_addr;
#ifdef WORKER_DEBUG
    printf("REGISTER\n");
    printf("liistenning addr : %s, port : %d\n", inet_ntoa(*(struct in_addr *)&regist_tmp->addr), ntohs(regist_tmp->port));
#endif
    Dg_send_recv(proxyfd, (void *)&request_cli, sizeof(request), (void *)&response_cli, sizeof(response_cli), (SA *)&proxyaddr, sizeof(proxyaddr));
    regist_ack *regist_ack_tmp = (regist_ack *)&response_cli;
    
#ifdef WORKER_DEBUG
    printf("ack : %"PRIu16", index : %"PRIu32", passwd : %"PRIu32"\n", regist_ack_tmp->ack, regist_ack_tmp->index, regist_ack_tmp->passwd);
#endif
    this.passwd = regist_ack_tmp->passwd;
    this.index  = regist_ack_tmp->index;
    this.load   = 0;
    this.status = WKR_RDY;

    Pthread_create(&tid, NULL, request_counter, NULL);

    for( ; ; ) {
        connfd = Accept(listenfd, NULL, NULL);
        if (!Fork()) {
#ifdef WORKER_DEBUG
            printf("Accept new request.\n");
#endif
            char data_f[20], result_f[20];
            sprintf(data_f,"%u.data", getpid());
            sprintf(result_f,"%u.data", getpid());
            FILE *inFile = fopen(data_f, "wb");
            MD5_CTX mdContext;

            void *p = &data_hdr;
            int bytes, bytes_read, bytes_writen;
            bytes =sizeof(data_hdr);
            while (bytes > 0) {
                bytes_read = recv(connfd, p, bytes, 0);
                if (bytes_read == -1) {
                    err_sys("send error");
                }
                bytes -= bytes_read;
                p += bytes_read;
            }

#ifdef WORKER_DEBUG
            printf("file md5: ");
            for(int n=0; n<MD5_DIGEST_LENGTH; n++)
                printf("%02x", data_hdr.md5[n]);
            printf("file length: %d", data_hdr.file_len);
            printf("\n");
#endif
            bytes = data_hdr.file_len;
            unsigned char tmp_md5[MD5_DIGEST_LENGTH];
            MD5_Init (&mdContext);
            while (bytes > 0) {
                bytes_read = Recv(connfd, buf, min(sizeof(buf),bytes), 0);
                MD5_Update (&mdContext, buf, bytes_read);
                bytes_writen = fwrite(buf, 1, bytes_read, inFile); 
                if (bytes_read != bytes_writen) {
                    printf("read != write\n");
                }
                bytes -= bytes_read;
            }
            MD5_Final (tmp_md5,&mdContext);
            if (!strncmp(tmp_md5, data_hdr.md5, MD5_DIGEST_LENGTH)) {
                printf("md5 test successful!\n");
            }
            Fclose(inFile);

            FILE *outFile = fopen(result_f, "rb");
            result_hdr.file_len = 0;
            MD5_Init (&mdContext);
            while ((bytes = fread (buf, 1, BUFSIZE, outFile)) != 0) {
                result_hdr.file_len += bytes;
                MD5_Update (&mdContext, buf, bytes);
            }
            MD5_Final (result_hdr.md5,&mdContext);
#ifdef WORKER_DEBUG
            printf("file md5: ");
            for(int n=0; n<MD5_DIGEST_LENGTH; n++)
                printf("%02x", result_hdr.md5[n]);
            printf("file length: %d", result_hdr.file_len);
            printf("\n");
#endif
            p = &result_hdr;
            bytes =sizeof(result_hdr);
            while (bytes > 0) {
                bytes_writen = send(connfd, p, bytes, 0);
                if (bytes_writen == -1) {
                    err_sys("send error");
                }
                bytes -= bytes_writen;
                p += bytes_writen;
            }

            fseek(outFile, 0, SEEK_SET);
            while (1) {
                bytes = fread(buf, 1, sizeof(buf), outFile);
                if (bytes == 0) {
                    break;
                }
                if (bytes == -1) {

                }
                p = buf;
                while (bytes > 0) {
                    bytes_writen = send(connfd, p, bytes, 0);
                    if (bytes_writen == -1) {
                        err_sys("send error");
                    }
                    bytes -= bytes_writen;
                            p += bytes_writen;
                }
            }
            Fclose(outFile);
            remove(data_f);
            remove(result_f);
            close(connfd);

                
                       
        }

        close(connfd);
        Pthread_mutex_lock(&count_mutex);
        load++;
#ifdef WORKER_DEBUG
        printf("request count: %d.\n", load);
#endif
        diff = load - pre_load;
        Pthread_mutex_unlock(&count_mutex);

                /* foward count to proxy */
        if ((diff > -7) && (diff < 7)) continue;

        update_tmp = (update *)&request_cli;
        update_tmp->type = UPDT;
        update_tmp->index = this.index;
        update_tmp->passwd = this.passwd;

        Pthread_mutex_lock(&mesg_mutex);
        tmp_load = load;
        diff = tmp_load - pre_load;
        if ((diff < -7) || (diff > 7)) {
#ifdef WORKER_DEBUG
            printf("UPDATING request count.\n");
#endif
            update_tmp->load = tmp_load;
            Dg_send_recv(proxyfd, &request_cli, sizeof(request_cli), &response_cli, 
                        sizeof(response_cli), (SA *)&proxyaddr, sizeof(proxyaddr));
            update_ack_tmp = (update_ack *)&response_cli;
#ifdef WORKER_DEBUG
            if (update_ack_tmp->ack == 1) {
                printf("UPDATING successful.\n");
            }
#endif
            pre_load = tmp_load;
        }
        Pthread_mutex_unlock(&mesg_mutex);
    }
    return 0;
}


