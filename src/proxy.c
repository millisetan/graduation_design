#include "server.h"
#include "rtt.h"

#define PROXY_DEBUG
#ifdef  PROXY_DEBUG
#include <inttypes.h>
#endif


 #define RECV_MESG(fd)  msgrecv.msg_name = &rqstaddr;\
                        msgrecv.msg_namelen = sizeof(rqstaddr);\
                        msgrecv.msg_iov = iovrecv;\
                        msgrecv.msg_iovlen = 2;\
                        iovrecv[0].iov_base = &send_recv_hdr;\
                        iovrecv[0].iov_len = sizeof(send_recv_hdr);\
                        iovrecv[1].iov_base = &request;\
                        iovrecv[1].iov_len = sizeof(request_t);\
                        Recvmsg(fd, &msgrecv, 0);

#define SEND_MESG(fd)   msgsend.msg_name = &rqstaddr;\
                        msgsend.msg_namelen = sizeof(rqstaddr);\
                        msgsend.msg_iov = iovsend;\
                        msgsend.msg_iovlen = 2;\
                        iovsend[0].iov_base = &send_recv_hdr;\
                        iovsend[0].iov_len = sizeof(send_recv_hdr);\
                        iovsend[1].iov_base = &response;\
                        iovsend[1].iov_len = sizeof(response_t);\
                        Sendmsg(fd, &msgsend, 0);


static struct msghdr msgsend, msgrecv;
static struct iovec iovsend[2], iovrecv[2];
static struct sockaddr_in rqstaddr;
static struct sockaddr_in clilstnaddr, wkrlstnaddr, testaddr;

int main()
{
    int clifd, wkrfd, testfd, option =1;
    int nworker =0, wkrcur =-1, lst_ind =0, nrequest =0, nfailed =0;
    fd_set rset;
    worker *workers;
    request_t request;
    response_t response;
    dg_hdr send_recv_hdr;

    /* Listenning for client */
    clifd = Socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(clifd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    memset(&clilstnaddr, 0, sizeof(clilstnaddr));
    clilstnaddr.sin_family = AF_INET;
    clilstnaddr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, SERV_IP, &clilstnaddr.sin_addr);
    
    Bind(clifd, (struct sockaddr *)&clilstnaddr, sizeof(clilstnaddr));
    
    /* Listenning for worker */
    wkrfd = Socket(AF_INET, SOCK_DGRAM, 0);
    Setsockopt(wkrfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    memset(&wkrlstnaddr, 0, sizeof(wkrlstnaddr));
    wkrlstnaddr.sin_family = AF_INET;
    wkrlstnaddr.sin_port = htons(PROXY_PORT);
    inet_pton(AF_INET, PROXY_IP, &wkrlstnaddr.sin_addr);
    Bind(wkrfd, (struct sockaddr *)&wkrlstnaddr, sizeof(wkrlstnaddr));

    workers = Calloc(DEFAULT_NWORKER, sizeof(worker));

    FD_ZERO(&rset);
    for ( ; ; ) {
        FD_SET(clifd, &rset);
        FD_SET(wkrfd, &rset);
        Select( (clifd>wkrfd ? clifd : wkrfd)+1, &rset, NULL, NULL, NULL);

        /* Handle client request */
        if (FD_ISSET(clifd, &rset)) { 
            RECV_MESG(clifd);
            /* Test worker avaliability for reget worker */
            if (request.type == REGET) {
                /* check worker available */
                reget *tmp = (reget *)&request;
                for (int i=0; i<nworker; i++) {
                    if ((tmp->port == workers[i].port) &&
                        (tmp->addr == workers[i].addr)) {
                        if (workers[i].status == WKR_FLD) break;
                        testfd = Socket(AF_INET, SOCK_STREAM, 0);
                        memset(&testaddr, 0, sizeof(testaddr));
                        testaddr.sin_family = AF_INET;
                        testaddr.sin_port = tmp->port;
                        testaddr.sin_addr.s_addr = tmp->addr;
                        if (connect(testfd, (struct sockaddr *)&testaddr,
                                     sizeof(testaddr)) < 0) {
                            //worker close socket(crash or terminated)
                            workers[i].status = WKR_FLD;
#ifdef PROXY_DEBUG
                            printf("Worker %d failed\n", i);
#endif
                            if ((++nfailed < nworker) && (i == lst_ind)) {
                                for (int j =0; j < nworker; j++) {
                                    if ( (workers[j].load < workers[lst_ind].load) &&
                                         (workers[j].status != WKR_FLD)) {
                                        lst_ind = j;
                                    }
                                }
                                workers[lst_ind].status = WKR_RDY;
                            }
                        } else {
                            //worker listen on socket, but stopped.

                        }
                        close(testfd);
                        break;
                    }
                }
            }
            /* Send response back */
            if ((request.type == GET) || (request.type == REGET)) {
#ifdef PROXY_DEBUG
                if (request.type == GET) {
                    printf("GETTING new worker, ");
                }
                else if (request.type == REGET) {
                    printf("REGETTING new worker, ");
                }
#endif
                get_ack *result = (get_ack *)&response;
                if (nfailed < nworker) {
                    result->ack = 0; //default ack;
                } else {
                    result->ack = 2; // all workers failed;
                }
                for (int i =1; i <= nworker; i++) {
                    if (workers[(wkrcur+i)%nworker].status == WKR_RDY) {
                        wkrcur = (wkrcur+i)%nworker;
                        result->ack = 1; 
#ifdef PROXY_DEBUG
                        printf("select worker id: %u\n", wkrcur);
#endif
                        break;
                    }
                }
#ifdef PROXY_DEBUG
                if (result->ack != 1) { 
                    printf("no availble worker\n");
                }
#endif
                result->port = workers[wkrcur].port;
                result->addr = workers[wkrcur].addr;
                SEND_MESG(clifd);
            }
        }
        /* Handle worker request */
        if (FD_ISSET(wkrfd, &rset)) {
            RECV_MESG(wkrfd);
            /* Handle worker register */
            if (request.type == RGST) {
                regist *tmp = (regist *)&request;
                int valid =0;
#ifdef PROXY_DEBUG
                printf("REGISTERING.\n");
#endif
                int i;
                for (i=0; i<nworker; i++) {
                    if ((tmp->port == workers[i].port) &&
                        (tmp->addr == workers[i].addr)) {
#ifdef PROXY_DEBUG
                        /* happenned when regist_ack lost on the way */
                        printf("worker recovery.\n");
#endif
                        worker *wkr_tmp = &workers[i];
                        wkr_tmp->status = WKR_RDY;
                        wkr_tmp->load = 0;
                        break;
                    }
                }
                if (i == nworker) {
                    nworker++;
                    if (nworker > DEFAULT_NWORKER) {
                        workers = Realloc(workers, sizeof(worker)*(++nworker));
                    }
                    worker *wkr_tmp = &workers[nworker-1];
                    wkr_tmp->status = WKR_RDY;
                    wkr_tmp->passwd = random();
                    wkr_tmp->load = 0;
                    wkr_tmp->port = tmp->port;
                    wkr_tmp->addr = tmp->addr;
#ifdef PROXY_PORT
                        printf("New worker addr : %s, port : %d, ID : %d. now worker : %d\n", inet_ntoa(*(struct in_addr *)&tmp->addr), ntohs(tmp->port), i, nworker);
#endif
                }
                regist_ack *result = (regist_ack *)&response;
                result->ack = 1;
                result->index = i;
                result->passwd = workers[i].passwd;
                SEND_MESG(wkrfd);
            }
            /* Handle worker information update */
            else if(request.type == UPDT) {
#ifdef PROXY_DEBUG
            printf("UPDATE.\n");
#endif

                update *update_tmp = (update *)&request;
                int valid =0;
                if (workers[update_tmp->index].passwd == update_tmp->passwd) {
                    /* If worker reconnect to proxy */
                    if (workers[update_tmp->index].status == WKR_FLD) {
                        workers[update_tmp->index].status == WKR_RDY;
                    }
                    valid = 1;
#ifdef PROXY_DEBUG
                    printf("Worker ID: %u, update value: %u\n", update_tmp->index, update_tmp->load);
#endif
                    if (update_tmp->index == lst_ind) {
                        if (workers[lst_ind].load < update_tmp->load) {
                            workers[lst_ind].load = update_tmp->load;
                            for (int i =0; i < nworker; i++) {
                                if ( (workers[i].load < workers[lst_ind].load) &&
                                     (workers[i].status != WKR_FLD)) {
                                    lst_ind = i;
                                }
                            }
                            workers[lst_ind].status = WKR_RDY;
                        } else {
                            workers[lst_ind].load = update_tmp->load;
                        }
                    } else {
                        workers[update_tmp->index].load = update_tmp->load;
                        if (update_tmp->load > (workers[lst_ind].load + 50)) {
                            workers[update_tmp->index].status = WKR_OVL;
                        } else if (update_tmp->load >= workers[lst_ind].load) {
                            workers[update_tmp->index].status = WKR_RDY;
                        } else {
                            lst_ind = update_tmp->index;
                        }
                    }
                }
                update_ack *result = (update_ack *)&response;
                result->ack = valid;

#ifdef PROXY_DEBUG
                if (valid == 1) {
                    printf("update successful\n");
                } else {
                    printf("update failed\n");
                }
#endif
                msgsend.msg_name = &rqstaddr;
                msgsend.msg_namelen = sizeof(rqstaddr);
                msgsend.msg_iov = iovsend;
                msgsend.msg_iovlen = 2;
                iovsend[0].iov_base = &send_recv_hdr;
                iovsend[0].iov_len = sizeof(send_recv_hdr);
                iovsend[1].iov_base = &response;
                iovsend[1].iov_len = sizeof(response_t);
                Sendmsg(wkrfd, &msgsend, 0);
            }
        }
    }
}
