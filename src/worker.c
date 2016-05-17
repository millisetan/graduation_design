#include "server.h"
#include "rtt.h"
#include <limits.h>
#define WORKER_DEBUG

#ifdef  WORKER_DEBUG
#include <inttypes.h>
#endif

pthread_t ntid;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mesg_mutex = PTHREAD_MUTEX_INITIALIZER;
worker this[PROXY_NUM];
int itvl = 3, load, pre_load, proxyfd[PROXY_NUM];
struct sockaddr_in proxyaddr[PROXY_NUM];

ssize_t dg_send_recv(int fd, const void *outbuff, size_t outbytes,
             void *inbuff, size_t inbytes,
             const struct sockaddr *destaddr, socklen_t destlen);

void *request_counter(void *arg)
{
    update_ack *update_ack_tmp;
    int status, diff, tmp_load;
    update *update_tmp;
    request_t request;
    response_t response;
    int error = waitpid((intptr_t)arg, &status, 0);
#ifdef WORKER_DEBUG
    printf("Request handle by pid %ld, return value %d\n", (intptr_t)arg, (int)WEXITSTATUS(status));
#endif
    Pthread_mutex_lock(&count_mutex);
    load--;
#ifdef WORKER_DEBUG
    printf("request count decrement : %d.\n", load);
#endif
    diff = load - pre_load;
    Pthread_mutex_unlock(&count_mutex);

    /* foward count to proxy */
    
    if ( (diff > -itvl) && (diff < itvl)){
        pthread_exit(NULL);
    }


    Pthread_mutex_lock(&mesg_mutex);
    tmp_load = load;
    diff = tmp_load - pre_load;
    if ( (diff <= -itvl) || (diff >= itvl)) {
        for (int i = 0; i < PROXY_NUM; i++) {
            if (this[i].status == WKR_FLD) {
                continue;
            }
#ifdef WORKER_DEBUG
            printf("UPDATING request count : %d\n", tmp_load);
#endif
            update_tmp = (update *)&request;
            update_tmp->type = UPDT;
            update_tmp->index = this[i].index;
            update_tmp->passwd = this[i].passwd;
            update_tmp->load = tmp_load;
            ssize_t ret = dg_send_recv(proxyfd[i], &request, sizeof(request), &response, 
                        sizeof(response), (SA *)&proxyaddr[i], sizeof(proxyaddr[i]));
            if (ret < 0) {
                this[i].status = WKR_FLD;
                continue;
            }
            update_ack_tmp = (update_ack *)&response;
#ifdef WORKER_DEBUG
            printf("UPDATING successfuly.\n");
#endif
        }
        pre_load = tmp_load;
    }
    Pthread_mutex_unlock(&mesg_mutex);
}


int main(int argc, char **argv)
{
    file_hdr data_hdr, result_hdr;
    char buf[BUFSIZE];
    ssize_t len;
    int listenfd, connfd, diff, tmp_load;
    long timestamp = rand();
    pid_t childpid;
    pthread_t tid;
    struct sockaddr_in workeraddr;
    socklen_t workerlen;
    update *update_tmp;
    update_ack *update_ack_tmp;
    request_t request_cli;
    response_t response_cli;
    fd_set rset, wset;
    struct timeval time_out;

    /* Option and config file */
    openlog(NULL,LOG_CONS|LOG_PID, LOG_LOCAL1);


    /* creat a worker */
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    Listen(listenfd, MAX_REQUESTS);
    workerlen = sizeof(workeraddr);
    Getsockname(listenfd, (SA *)&workeraddr, &workerlen);

    /* connect to server and register */
    proxyfd[0] = Socket(AF_INET, SOCK_DGRAM, 0);
    memset(&proxyaddr[0], 0, sizeof(proxyaddr[0]));
    proxyaddr[0].sin_family = AF_INET;
    proxyaddr[0].sin_port = htons(PROXY_PORT);
    inet_pton(AF_INET, PROXY_IP, &proxyaddr[0].sin_addr);

    proxyfd[1] = Socket(AF_INET, SOCK_DGRAM, 0);
    memset(&proxyaddr[1], 0, sizeof(proxyaddr[1]));
    proxyaddr[1].sin_family = AF_INET;
    proxyaddr[1].sin_port = htons(PROXY_PORT1);
    inet_pton(AF_INET, PROXY_IP1, &proxyaddr[1].sin_addr);

    regist *regist_tmp = (regist *)&request_cli;
    regist_tmp->type = RGST;
    regist_tmp->port = (uint16_t)workeraddr.sin_port;
    regist_tmp->addr = (uint32_t)workeraddr.sin_addr.s_addr;
    for (int i = 0; i < PROXY_NUM; i++) {
#ifdef WORKER_DEBUG
        printf("REGISTER ");
#endif
        ssize_t ret = dg_send_recv(proxyfd[i], (void *)&request_cli, sizeof(request_cli), (void *)&response_cli, sizeof(response_cli), (SA *)&proxyaddr[i], sizeof(proxyaddr[i]));
        if (ret < 0) {
            this[i].status = WKR_FLD;
        } else {
            regist_ack *regist_ack_tmp = (regist_ack *)&response_cli;
                
#ifdef WORKER_DEBUG
            printf(", Worker ID : %d\n", regist_ack_tmp->index);
#endif
            this[i].passwd = regist_ack_tmp->passwd;
            this[i].index  = regist_ack_tmp->index;
            this[i].load   = 0;
            this[i].status = WKR_RDY;
        }
    }

    for( ; ; ) {
        connfd = Accept(listenfd, NULL, NULL);
        childpid = fork();
        if (childpid < 0) {
            close(connfd);
            continue;
        }
        if (childpid == 0) {
            FILE *inFile, *outFile;
            MD5_CTX mdContext;
            unsigned char tmp_md5[MD5_DIGEST_LENGTH];
            int error;
            socklen_t err_size = sizeof(error);
            void *p = &data_hdr;
            int bytes, bytes_read, bytes_writen;

            /* Recieve data file header */
#ifdef WORKER_DEBUG
            printf("recieving header from client.\n");
#endif
            bytes = sizeof(data_hdr);
            FD_ZERO(&rset);
            while (bytes > 0) {
                FD_SET(connfd, &rset);
                time_out.tv_sec = 60;
                time_out.tv_usec = 0;
                if (!select(connfd +1, &rset, NULL, NULL, &time_out)) {
#ifdef WORKER_DEBUG
                    printf("recieving data from client timeout, terminate.\n");
#endif
                    close(connfd);
                    exit(1);
                }
                getsockopt(connfd, SOL_SOCKET, SO_ERROR, &error, &err_size);
                if (error) {
                    close(connfd);
                    errno = error;
                    exit(1);
                }
                bytes_read = recv(connfd, p, bytes, 0);
                if (!bytes_read) {
#ifdef WORKER_DEBUG
                    printf("Client abort conncetion, terminate.\n");
#endif
                    exit(1);
                }
                bytes -= bytes_read;
                p += bytes_read;
            }

            /* Recieve data file content */
#ifdef WORKER_DEBUG
            printf("recieving data from client.\n");
#endif
            inFile = tmpfile();
            bytes = data_hdr.file_len;
            MD5_Init (&mdContext);
            while (bytes > 0) {
                FD_ZERO(&rset);
                FD_SET(connfd, &rset);
                time_out.tv_sec = 60;
                time_out.tv_usec = 0;
                if (!select(connfd +1, &rset, NULL, NULL, &time_out)) {
#ifdef WORKER_DEBUG
                    printf("recieving data from client timeout, terminate.\n");
#endif
                    close(connfd);
                    exit(1);
                }
                getsockopt(connfd, SOL_SOCKET, SO_ERROR, &error, &err_size);
                if (error) {
                    close(connfd);
                    errno = error;
                    exit(1);
                }
                bytes_read = Recv(connfd, buf, min(sizeof(buf),bytes), 0);
                if (!bytes_read) {
#ifdef WORKER_DEBUG
                    printf("Client abort conncetion, terminate.\n");
#endif
                    exit(1);
                }
                MD5_Update (&mdContext, buf, bytes_read);
                bytes_writen = fwrite(buf, 1, bytes_read, inFile); 
                bytes -= bytes_read;
            }
            MD5_Final (tmp_md5,&mdContext);
            if (strncmp(tmp_md5, data_hdr.md5, MD5_DIGEST_LENGTH)) {
#ifdef WORKER_DEBUG
                printf("md5 test failed, terminate.\n");
#endif
                close(connfd);
                exit(1);
            }


            /* prepare result file header */
            outFile = inFile; //error file
            fseek(outFile, 0, SEEK_SET);
            result_hdr.file_len = 0;
            MD5_Init (&mdContext);
            while ((bytes = fread (buf, 1, BUFSIZE, outFile)) != 0) {
                result_hdr.file_len += bytes;
                MD5_Update (&mdContext, buf, bytes);
            }
            MD5_Final (result_hdr.md5,&mdContext);

                        



            /* Send result file header */
#ifdef WORKER_DEBUG
            printf("send result header!\n");
#endif
            p = &result_hdr;
            bytes =sizeof(result_hdr);
            while (bytes > 0) {
                FD_ZERO(&wset);
                FD_SET(connfd, &wset);
                select(connfd +1, NULL, &wset, NULL, NULL);
                getsockopt(connfd, SOL_SOCKET, SO_ERROR, &error, &err_size);
                if (error) {
                    if (error == EPIPE) {

                    }
                    errno  = error;
                    close(connfd);
                    exit(1);
                }
                bytes_writen = send(connfd, p, bytes, 0);
                bytes -= bytes_writen;
                p += bytes_writen;
            }

            /* Send result file content */
#ifdef WORKER_DEBUG
            printf("send result file!\n");
#endif
            fseek(outFile, 0, SEEK_SET);
            bytes = result_hdr.file_len;
            while (bytes >0) {
                bytes_read = fread(buf, 1, sizeof(buf), outFile);
                bytes -= bytes_read;
                p = buf;
                while (bytes_read > 0) {
                    FD_ZERO(&wset);
                    FD_SET(connfd, &wset);
                    select(connfd +1, NULL, &wset, NULL, NULL);
                    getsockopt(connfd, SOL_SOCKET, SO_ERROR, &error, &err_size);
                    if (error) {
                        if (error == EPIPE) {

                        }
                        errno = error;
                        close(connfd);
                        exit(1);
                    }
                    bytes_writen = send(connfd, p, bytes_read, 0);
                    bytes_read -= bytes_writen;
                    p += bytes_writen;
                }
            }
            /*
            for(int i=0; i < 30; i++) {
                for(int j=0; j < INT_MAX; j++) {
                    int kk = i+j;
                }
            }
            */
            sleep(20);
#ifdef WORKER_DEBUG
            printf("connection end!\n");
#endif
            close(connfd);
            exit(0);
        }
        close(connfd);
        Pthread_create(&tid, NULL, request_counter, (void *) (intptr_t)childpid);
        Pthread_mutex_lock(&count_mutex);
        load++;
#ifdef WORKER_DEBUG
        printf("request count increment: %d.\n", load);
#endif
        diff = load - pre_load;
        Pthread_mutex_unlock(&count_mutex);

                /* foward count to proxy */
        if ((diff > -itvl) && (diff < itvl)) continue;


        Pthread_mutex_lock(&mesg_mutex);
        tmp_load = load;
        diff = tmp_load - pre_load;
        if ((diff < -itvl) || (diff > itvl)) {
            for (int i =0; i < PROXY_NUM; i++) {
                if (this[i].status == WKR_FLD) {
                    continue;
                }
#ifdef WORKER_DEBUG
                printf("UPDATING request count.\n");
#endif
                update_tmp = (update *)&request_cli;
                update_tmp->load = tmp_load;
                update_tmp->type = UPDT;
                update_tmp->index = this[i].index;
                update_tmp->passwd = this[i].passwd;
                ssize_t ret = dg_send_recv(proxyfd[i], &request_cli, sizeof(request_cli), &response_cli, 
                            sizeof(response_cli), (SA *)&proxyaddr[i], sizeof(proxyaddr[i]));
                if (ret < 0) {
                    this[i].status = WKR_FLD;
                    continue;
                }
                update_ack_tmp = (update_ack *)&response_cli;
#ifdef WORKER_DEBUG
                if (update_ack_tmp->ack == 1) {
                    printf("UPDATING successful.\n");
                }
#endif
            }
            pre_load = tmp_load;
        }
        Pthread_mutex_unlock(&mesg_mutex);
    }
    return 0;
}


