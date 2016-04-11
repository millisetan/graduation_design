#include <sys/select.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/ip.h>
#include <syslog.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_REQUESTS 10000
#define PROXY_PORT   56789
#define PROXY_ADDR   "45.62.113.14"
#define REGIST       0
#define ACK_REGIST   1

pthread_t ntid;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
int count;

void *request_counter(void *arg)
{
    int status;
    for ( ; ; ) {
        if (waitpid(-1, &status, 0) == -1) {
            continue;
        }
        pthread_mutex_lock(&count_mutex);
        count--;
        pthread_mutex_unlock(&count_mutex);
        /* foward count to proxy */

    }
}


int main(int argc, char **argv)
{
    char regist = REGIST;
    char buf[BUF_SIZE];
    ssize_t len;
    int listenfd, connfd, proxyfd;
    long timestamp = random();
    pid_t childpid;
    struct sockaddr_in workeraddr, proxyaddr;
    socklen_t workerlen;

    /* Option and config file */
    openlog(NULL,LOG_CONS|LOG_PID, LOG_LOCAL1);


    /* creat a worker */
    if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        syslog(LOG_EMERG, "socket creation failed\n");
        exit(1);
    }

    if (listen(listenfd, MAX_REQUESTS) == -1) {
        syslog(LOG_EMERG, "socket listenning failed\n");
        exit(1);
    }

    workerlen = sizeof(workeraddr);
    if (getsockname(listenfd, (struct sockaddr *)&workeraddr, &workerlen) == -1) {
        syslog(LOG_EMERG, "getting listenfd socket failed\n");
        exit(1);
    }

    /* connect to server and register */
    if ( (proxyfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        syslog(LOG_EMERG, "proxy socket creation failed\n");
        exit(1);
    }
    memset(&proxyaddr, 0, sizeof(proxyaddr));
    proxyaddr.sin_family = AF_INET;
    proxyaddr.sin_port = htons(PROXY_PORT);
    inet_pton(AF_INET, PROXY_ADDR, &proxyaddr.sin_addr);

    if (sendto(proxyfd, (struct sockaddr *)&proxyaddr, sizeof(proxyaddr)) == -1) {
        syslog(LOG_EMERG, "connecting to proxy failed\n");
        exit(1);
    }
    /* register from proxy */
    send(proxyfd, &regist, sizeof(char), 0);
    timestamp++;
    send(proxyfd, &timestamp, sizeof(long), 0);
    send(proxyfd, &workeraddr, workerlen, 0);
    do {
        recv(proxyfd, buf, 1, 0);
    while (*(char *)buf != ACK_REGIST)

    int err = pthread_create(&ntid, NULL, request_counter, NULL);
    if (err != 0) {
        syslog(LOG_EMERG, "thread request_counter creation failed\n");
        exit(1);
    }
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


