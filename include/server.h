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



#define MAX_REQUESTS 10000;
#define MAX_WORKER   100;
#define SERV_PORT    56789;
#define SERV_IP      "45.63.113.14";
#define PROXY_PORT    66789;
#define PROXY_IP      "45.63.113.14";
#define MESG_LEN     100;



typedef enum {RQST, RERQST, RGST, UPDT} mesg_type;

typedef struct {
    mesg_type type;
    char data[16];
} request_t;

typedef struct {
    uint16_t ack;
    char data[16];
} response_t;

typedef struct {
    mesg_type type;
} get;

typedef struct {
    uint16_t ack;
    uint16_t port;
    uint32_t addr;
} get_ack;

typedef struct  {
    mesg_type type;
    uint16_t port;
    uint32_t addr;
} reget;

typedef struct  {
    uint16_t ack;
    uint16_t port;
    uint32_t addr;
} reget_ack;

typedef struct {
    mesg_type type;
    uint16_t port;
    uint32_t addr;
} regist; 

typedef struct {
    uint16_t ack;
    uint32_t index;
    uint32_t passwd;
} regist_ack; 

typedef struct {
    mesg_type type;
    uint32_t index;
    uint32_t passwd;
    int load;
} update;

tyepdef struct {
    uint16_t ack;
} update_ack; 

typefe struct {
    const char *domain;
    const char *id;
    uint16_t   port;
    uint32_t   addr;
} server_conf;

typedef struct {
    int passwd;
    int load;
    int status;
    uint16_t port;
    uint32_t addr;
} worker; 

#define WKR_FLD 0
#define WKR_RDY 1
#define WKR_OVL 2


