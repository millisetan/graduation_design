#include "server.h"
#include "rtt.h"

pthread_t ntid;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mesg_mutex = PTHREAD_MUTEX_INITIALIZER;
worker this;
int pre_load, proxyfd;
request_t request;
response_t response;
Dg_send_recv(int fd, const void *outbuff, size_t outbytes,
             void *inbuff, size_t inbytes,
             const SA *destaddr, socklen_t destlen);

void *request_counter(void *arg)
{
    int status, diff;
    for ( ; ; ) {
        if (waitpid(-1, &status, 0) == -1) {
            continue;
        }
        Pthread_mutex_lock(&count_mutex);
        this.load--;
        diff = load - pre_load;
        Pthread_mutex_unlock(&count_mutex);
        /* foward count to proxy */
        if ((diff > -7) && (diff < 7)) continue;

        update *update_tmp = &request;
        update->type = UPDT;
        update->index = this.index;
        update->passwd = this.passwd;
        Pthread_mutex_lock(&mesg_mutex);
        
        Dg_send_recv(proxyfd, &request, sizeof(request), &response, 
                    sizeof(response), &proxyaddr, sizeof(proxyaddr));
        Pthread_mutex_unlock(&mesg_mutex);
        update *update_ack_tmp = &response;
    }
}


int main(int argc, char **argv)
{
    char regist = REGIST;
    char buf[BUF_SIZE];
    ssize_t len;
    int listenfd, connfd;
    long timestamp = random();
    pid_t childpid;
    struct sockaddr_in workeraddr, proxyaddr;
    socklen_t workerlen;

    /* Option and config file */
    openlog(NULL,LOG_CONS|LOG_PID, LOG_LOCAL1);


    /* creat a worker */
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    Listen(listenfd, MAX_REQUESTS);

    workerlen = sizeof(workeraddr);
    Getsockname(listenfd, (struct sockaddr *)&workeraddr, &workerlen);

    /* connect to server and register */
    proxyfd = Socket(AF_INET, SOCK_STREAM, 0);
    memset(&proxyaddr, 0, sizeof(proxyaddr));
    proxyaddr.sin_family = AF_INET;
    proxyaddr.sin_port = htons(PROXY_PORT);
    inet_pton(AF_INET, PROXY_ADDR, &proxyaddr.sin_addr);

    regist *regist_tmp = &request;
    regist_tmp->type = RGST;
    regist_tmp->port = (uint16_t)workeraddr.sin_port;
    regist_tmp->addr = (uint32_t)workeraddr.sin_addr;
    Dg_send_recv(proxyfd, &request, sizeof(request), &response, 
                sizeof(response), &proxyaddr, sizeof(proxyaddr));
    regist_ack *regist_ack_tmp = &response;
    this.passwd = regist_ack_tmp->passwd;
    this.index  = regist_ack_tmp->index;
    this.load   = 0;
    this.status = WKR_RDY;
    regist_ack_tmp->port = (uint16_t)workeraddr.sin_port;
    regist_ack_tmp->addr = (uint32_t)workeraddr.sin_addr;

    Pthread_create(&ntid, NULL, request_counter, NULL);

    for( ; ; ) {
        if ( (connfd = accept(listenfd, NULL, NULL)) == -1) {
            if (errno == EINTR)
                continue;
            else { 
            syslog(LOG_EMERG, "accepting new request failed\n");
            exit(1);
            }
        }
        if ( (childpid = fork()) == -1) {
            syslog(LOG_EMERG, "child fork failed\n");
            exit(1);
        }
        if (childpid == 0) {

        }
        pthread_mutex_lock(&count_mutex);
        count++;
        pthread_mutex_unlock(&count_mutex);
    }
    return 0;
}


