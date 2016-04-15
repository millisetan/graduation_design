#include "rtt.h"
#include "server.h"

#define CLIENT_DEBUG


int dg_send_recv(int fd, const void *outbuff, size_t outbytes,
             void *inbuff, size_t inbytes,
             const struct sockaddr *destaddr, socklen_t destlen);

int send_and_get (const char *data_f, const char *result_f)
{
    int proxyfd, workerfd, is_reget =0, bytes, bytes_read, bytes_writen, error;
    struct sockaddr_in proxyaddr, workeraddr;
    char buf[BUFSIZE];
    MD5_CTX mdContext;
	unsigned char tmp_md5[MD5_DIGEST_LENGTH];
    file_hdr data_hdr, result_hdr;
    FILE *inFile, *outFile;
    struct timeval recv_timeout;
    void *p;
    request_t request;
    response_t response;
    fd_set rset;
    socklen_t err_size;

    proxyfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (proxyfd < 0) {

    }
    memset(&proxyaddr, 0, sizeof(proxyaddr));
    proxyaddr.sin_family = AF_INET;
    proxyaddr.sin_port = htons(SERV_PORT);
    error = inet_pton(AF_INET, SERV_IP, &proxyaddr.sin_addr);
    if (error < 0) {

    }

    signal(SIGPIPE, SIG_IGN);
REGET:
    if (is_reget) {
        exit(1);
        reget *reget_tmp = (reget *)&request;
        reget_tmp->type = REGET;
        reget_tmp->port = workeraddr.sin_port;
        reget_tmp->addr = workeraddr.sin_addr.s_addr; 
    } else {
        get *get_tmp = (get *)&request;
        get_tmp->type = GET;
    }
    /* connect to server, quit if failed */
    error = dg_send_recv(proxyfd, (void *)&request, sizeof(request), (void *)&response, sizeof(response), (SA *)&proxyaddr, sizeof(proxyaddr));
    if (error < 0) {

    }
    
    if (is_reget) {
        reget_ack *reget_ack_tmp = (reget_ack *)&response;
        workerfd = Socket(AF_INET, SOCK_STREAM, 0);
        memset(&workeraddr, 0, sizeof(workeraddr));
        workeraddr.sin_family = AF_INET;
        workeraddr.sin_port = reget_ack_tmp->port;
        workeraddr.sin_addr.s_addr = reget_ack_tmp->addr;
    } else {
        get_ack *get_ack_tmp = (get_ack *)&response;
        workerfd = Socket(AF_INET, SOCK_STREAM, 0);
        memset(&workeraddr, 0, sizeof(workeraddr));
        workeraddr.sin_family = AF_INET;
        workeraddr.sin_port = get_ack_tmp->port;
        workeraddr.sin_addr.s_addr = get_ack_tmp->addr;
    }

    if (connect(workerfd, (struct sockaddr *)&workeraddr, sizeof(workeraddr)) < 0) {
        is_reget = 1;
        goto REGET;
    }

    /* Get file length and md5 */
    inFile = fopen (data_f, "rb");
    if (!inFile) {

    }
    data_hdr.file_len = 0;
    MD5_Init (&mdContext);
    while ((bytes_read = fread (buf, 1, BUFSIZE, inFile)) != 0) {
        data_hdr.file_len += bytes_read;
        MD5_Update (&mdContext, buf, bytes_read);
    }
    MD5_Final (data_hdr.md5,&mdContext);

    /* send file header(file length and md5) */
#ifdef CLIENT_DEBUG
    printf("send data header.\n");
#endif
    p = &data_hdr;
    bytes =sizeof(data_hdr);
    while (bytes > 0) {
        bytes_writen = send(workerfd, p, bytes, 0);
        if (bytes_writen == -1) {
            if (errno == EINTR) continue;
            is_reget = 1;
            Fclose(inFile);
            close(workerfd);
            goto REGET;
        }
        bytes -= bytes_writen;
        p += bytes_writen;
    }

    /* Send data file */
#ifdef CLIENT_DEBUG
    printf("send data file.\n");
#endif
    fseek(inFile, 0, SEEK_SET);
    bytes = 0;
    while (!feof(inFile)) {
        bytes_read = fread(buf, 1, sizeof(buf), inFile);
        bytes += bytes_read;
        if (ferror(inFile)) {
            is_reget = 1;
            Fclose(inFile);
            close(workerfd);
            goto REGET;
        }
        p = buf;
        while (bytes_read > 0) {
            bytes_writen = send(workerfd, p, bytes_read, 0);
            if (bytes_writen == -1) {
                is_reget = 1;
                Fclose(inFile);
                close(workerfd);
                goto REGET;
            }
            bytes_read -= bytes_writen;
            p += bytes_writen;
        }
    }
    if (bytes != data_hdr.file_len) {

    }
    Fclose(inFile);
        
    
    /* recieve file header(file length and md5) */
#ifdef CLIENT_DEBUG
    printf("recieve result header.\n");
#endif
    p = &result_hdr;
    bytes =sizeof(result_hdr);
    FD_ZERO(&rset);
    FD_SET(workerfd, &rset);
    recv_timeout.tv_sec = 900;
    recv_timeout.tv_usec = 0;
    if (!select(workerfd +1, &rset, NULL, NULL, &recv_timeout)) {
#ifdef CLIENT_DEBUG
        printf("Receiving worker result timeout\n");
#endif
        is_reget = 1;
        close(workerfd);
        goto REGET;
    } 
    while (bytes > 0) {
        FD_ZERO(&rset);
        FD_SET(workerfd, &rset);
        recv_timeout.tv_sec = 20;
        recv_timeout.tv_usec = 0;
        if (!select(workerfd +1, &rset, NULL, NULL, &recv_timeout)) {
#ifdef CLIENT_DEBUG
            printf("Receiving worker result timeout\n");
#endif
            is_reget = 1;
            close(workerfd);
            goto REGET;
        } 
        getsockopt(workerfd, SOL_SOCKET, SO_ERROR, &error, &err_size);
        if (error) {
            errno = error;
            is_reget = 1;
            close(workerfd);
            goto REGET;
        }
        bytes_writen = recv(workerfd, p, bytes, 0);
        if (bytes_writen == 0) {
            is_reget = 1;
            close(workerfd);
            goto REGET;
        }
        bytes -= bytes_writen;
        p += bytes_writen;
    }


    /* recieve result file */
#ifdef CLIENT_DEBUG
    printf("recieve result file.\n");
    printf("file length: %lu\n", result_hdr.file_len);
#endif
    bytes = result_hdr.file_len;
    outFile = fopen(result_f, "wb");
    MD5_Init (&mdContext);
    while (bytes > 0) {
        FD_ZERO(&rset);
        FD_SET(workerfd, &rset);
        recv_timeout.tv_sec = 20;
        recv_timeout.tv_usec = 0;
        if (!select(workerfd +1, &rset, NULL, NULL, &recv_timeout)) {
#ifdef CLIENT_DEBUG
            printf("Receiving worker result timeout\n");
#endif
            is_reget = 1;
            fclose(outFile);
            close(workerfd);
            goto REGET;
        } 
        getsockopt(workerfd, SOL_SOCKET, SO_ERROR, &error, &err_size);
        if (error) {
            errno = error;
            is_reget = 1;
            fclose(outFile);
            close(workerfd);
            goto REGET;
        }
        bytes_read = Recv(workerfd, buf, min(sizeof(buf),bytes), 0);
        MD5_Update (&mdContext, buf, bytes_read);
        bytes_writen = fwrite(buf, 1, bytes_read, outFile); 
        if (ferror(outFile)) {
            is_reget = 1;
            fclose(outFile);
            close(workerfd);
            goto REGET;
        }
        bytes -= bytes_read;
    }
    fclose(outFile);

    MD5_Final (tmp_md5,&mdContext);
    if (!strncmp(tmp_md5, result_hdr.md5, MD5_DIGEST_LENGTH)) {
#ifdef CLIENT_DEBUG
        printf("Recieve result file successfully.\n");
#endif
        Close(workerfd);
        return 0;
    } else {
#ifdef CLIENT_DEBUG
    printf("EEEEEEEEE.\n");
#endif
        remove(result_f);
        is_reget = 1;
        close(workerfd);
        goto REGET;
    }
}

int main(int argc, char **argv)
{
    send_and_get(argv[1], argv[2]);
    return 0;
}

