#include "rtt.h"
#include "server.h"

#define CLIENT_DEBUG

request_t request;
response_t response;
typedef struct {
    unsigned long file_len;
	unsigned char md5[MD5_DIGEST_LENGTH];
} file_hdr;

void Dg_send_recv(int fd, const void *outbuff, size_t outbytes,
             void *inbuff, size_t inbytes,
             const struct sockaddr *destaddr, socklen_t destlen);

int send_and_get (const char *data_f, const char *result_f)
{
    int proxyfd, workerfd;
    struct sockaddr_in proxyaddr, workeraddr;
    file_hdr data_hdr, result_hdr;
    char buf[BUFSIZE];

    proxyfd = Socket(AF_INET, SOCK_DGRAM, 0);
    memset(&proxyaddr, 0, sizeof(proxyaddr));
    proxyaddr.sin_family = AF_INET;
    proxyaddr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, SERV_IP, &proxyaddr.sin_addr);

    get *get_tmp = (get *)&request;
    get_tmp->type = GET;
    Dg_send_recv(proxyfd, (void *)&request, sizeof(request), (void *)&response, sizeof(response), (SA *)&proxyaddr, sizeof(proxyaddr));
    get_ack *get_ack_tmp = (get_ack *)&response;
    workerfd = Socket(AF_INET, SOCK_STREAM, 0);
    memset(&workeraddr, 0, sizeof(workeraddr));
    workeraddr.sin_family = AF_INET;
    workeraddr.sin_port = get_ack_tmp->port;
    workeraddr.sin_addr.s_addr = get_ack_tmp->addr;

    int con_retc = connect(workerfd, (struct sockaddr *)&workeraddr, sizeof(workeraddr));
#ifdef CLIENT_DEBUG
    printf("Connected to worker, addr: %s, port: %d.\n", inet_ntoa((struct in_addr)workeraddr.sin_addr), ntohs(workeraddr.sin_port));
#endif

    FILE *inFile = Fopen (data_f, "rb");
#ifdef CLIENT_DEBUG
    printf("infile :%s, outfile :%s\n", data_f, result_f);
#endif
    MD5_CTX mdContext;
    int bytes;

    data_hdr.file_len = 0;
    MD5_Init (&mdContext);
    while ((bytes = fread (buf, 1, BUFSIZE, inFile)) != 0) {
        data_hdr.file_len += bytes;
        MD5_Update (&mdContext, buf, bytes);
    }
    MD5_Final (data_hdr.md5,&mdContext);
#ifdef CLIENT_DEBUG
    printf("file md5: ");
    for(int n=0; n<MD5_DIGEST_LENGTH; n++)
        printf("%02x", data_hdr.md5[n]);
    printf("file length: %d", data_hdr.file_len);
    printf("\n");
#endif

    fseek(inFile, 0, SEEK_SET);
    void *p = &data_hdr;
    bytes =sizeof(data_hdr);
    int bytes_writen;
    while (bytes > 0) {
        bytes_writen = send(workerfd, p, bytes, 0);
        if (bytes_writen == -1) {
            err_sys("send error");
        }
        bytes -= bytes_writen;
        p += bytes_writen;
    }

    while (1) {
        bytes = fread(buf, 1, sizeof(buf), inFile);
        if (bytes == 0) {
            break;
        }
        if (bytes == -1) {

        }
        p = buf;
        while (bytes > 0) {
            bytes_writen = send(workerfd, p, bytes, 0);
            if (bytes_writen == -1) {
                err_sys("send error");
            }
            bytes -= bytes_writen;
            p += bytes_writen;
        }
    }
    Fclose(inFile);
        
    p = &result_hdr;
    bytes =sizeof(result_hdr);
    while (bytes > 0) {
        bytes_writen = recv(workerfd, p, bytes, 0);
        if (bytes_writen == -1) {
            err_sys("send error");
        }
        bytes -= bytes_writen;
        p += bytes_writen;
    }
#ifdef CLIENT_DEBUG
    printf("file md5: ");
    for(int n=0; n<MD5_DIGEST_LENGTH; n++)
        printf("%02x", data_hdr.md5[n]);
    printf("file length: %d", data_hdr.file_len);
    printf("\n");
#endif

    bytes = result_hdr.file_len;
    int bytes_read;
    FILE *outFile = fopen(result_f, "wb");
	unsigned char tmp_md5[MD5_DIGEST_LENGTH];
    MD5_Init (&mdContext);
    while (bytes > 0) {
        bytes_read = Recv(workerfd, buf, min(sizeof(buf),bytes), 0);
        MD5_Update (&mdContext, buf, bytes_read);
        bytes_writen = fwrite(buf, 1, bytes_read, outFile); 
        bytes -= bytes_read;
    }
    MD5_Final (tmp_md5,&mdContext);
    if (!strncmp(tmp_md5, result_hdr.md5, MD5_DIGEST_LENGTH)) {
#ifdef CLIENT_DEBUG
        printf("ma5 test successful.\n");
#endif
        Close(workerfd);
    }
}

int main(int argc, char **argv)
{
    send_and_get(argv[1], argv[2]);
    return 0;
}

