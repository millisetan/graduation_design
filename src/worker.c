#include "server.h"
#include "rtt.h"

pthread_t ntid;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mesg_mutex = PTHREAD_MUTEX_INITIALIZER;
worker this;
int load, pre_load, proxyfd;
struct sockaddr_in proxyaddr;
void Dg_send_recv(int fd, const void *outbuff, size_t outbytes,
             void *inbuff, size_t inbytes,
             const struct sockaddr *destaddr, socklen_t destlen);

void *request_counter(void *arg)
{
    update_ack *update_ack_tmp;
    int status, diff, tmp_load;
    request_t request;
    response_t response;
    update *update_tmp;
    for ( ; ; ) {
        if (waitpid(-1, &status, 0) == -1) {
            continue;
        }
        
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
        if ((diff < -7) && (diff > 7)) {
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
    char buf[BUFSIZE];
    ssize_t len;
    int listenfd, connfd, diff, tmp_load;
    long timestamp = random();
    pid_t childpid;
    struct sockaddr_in workeraddr;
    socklen_t workerlen;
    request_t request;
    response_t response;
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

    regist *regist_tmp = (regist *)&request;
    regist_tmp->type = RGST;
    regist_tmp->port = (uint16_t)workeraddr.sin_port;
    regist_tmp->addr = (uint32_t)workeraddr.sin_addr.s_addr;
    Dg_send_recv(proxyfd, (void *)&request, sizeof(request), (void *)&response, sizeof(response), (SA *)&proxyaddr, sizeof(proxyaddr));
    regist_ack *regist_ack_tmp = (regist_ack *)&response;
    this.passwd = regist_ack_tmp->passwd;
    this.index  = regist_ack_tmp->index;
    this.load   = 0;
    this.status = WKR_RDY;

    Pthread_create(NULL, NULL, request_counter, NULL);

    for( ; ; ) {
        connfd = Accept(listenfd, NULL, NULL);
        if (!Fork()) {


        }
        close(connfd);

        Pthread_mutex_lock(&count_mutex);
        load++;
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
        if ((diff < -7) && (diff > 7)) {
            update_tmp->load = tmp_load;
            Dg_send_recv(proxyfd, &request, sizeof(request), &response, 
                        sizeof(response), (SA *)&proxyaddr, sizeof(proxyaddr));
            update_ack_tmp = (update_ack *)&response;
            pre_load = tmp_load;
        }
        Pthread_mutex_unlock(&mesg_mutex);
    }
    return 0;
}


