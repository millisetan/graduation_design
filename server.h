#pragma once

#include   <stdio.h>
#include   <stdlib.h>
#include   <sys/types.h>
#include   <sys/socket>
#include   <sys/time>
#include   <netinet/inet.h>
#include   <errno.h>
#include   <fcntl.h>
#include   <signal>
#include   <unistd.h>
#include   <string.h>
#include   <pthread.h>
#include   <>
#include   <>
#include   <>
#include   <>
#include   <>
#include   <>



#define MAX_REQUESTS 10000;
#define SERV_PORT    66666;
#define SERV_IP      66666;


typedef struct server_rec   server_rec;
typedef struct server_conf   server_conf;
typedef struct proxy    proxy;
typedef struct request_rec  request_rec;
typedef struct proxy_balancer    proxy_balancer;
typedef struct worker      worker;
typedef struct balancer_method   balancer_method;


struct server_rec {
    request_rec *pending;
    request_rec *pending_end;
    request_rec *processing;
    request_rec *processing_end;
    long    rqst_cnt;
    char *hostname;
    proxy_balancer *balancer;

}

struct server_conf {
    const char *domain;
    const char *id;
    time_t     *timeout;
    long       max_requst;
    short      port;
    uint32_t   addr;


}

struct request_rec {
    request_rec *next;
    request_rec *pre;
    int connfd;
    sockaddr_in cliaddr;
    socklen_t   clilen;
    worker      *worker;
}

struct balancer {

    worker *(*finder)(balancer *bl, request_rec *rqt);
}

struct worker {
    worker *workers;
    worker *workers_end;
    int max_workers;


}

struct balancer_method {

}



