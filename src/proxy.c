#include "server.h"
#include "rtt.h"

#define PROXY_DEBUG
#ifdef  PROXY_DEBUG
#include <inttypes.h>
#endif
worker *workers;
int nworker, wkrcur =-1, least_loaded;
struct hdr {
    uint32_t seq;
    uint32_t ts;
} send_recv_hdr;
request_t request;
response_t response;
struct msghdr msgsend, msgrecv;
struct iovec iovsend[2], iovrecv[2];
struct sockaddr_in rqstaddr;

int main()
{
    int clifd, wkrfd, testfd, option =1, maxfdp, activity;
    int mesg_len;
    fd_set rset;
    struct sockaddr_in clilstnaddr, wkrlstnaddr, testaddr;

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
        activity  = Select( (clifd>wkrfd ? clifd : wkrfd)+1, &rset, NULL, NULL, NULL);

        /* Handle client request */
        if (FD_ISSET(clifd, &rset)) { 
            /* Recieve request message */
            msgrecv.msg_name = &rqstaddr;
            msgrecv.msg_namelen = sizeof(rqstaddr);
            msgrecv.msg_iov = iovrecv;
            msgrecv.msg_iovlen = 2;
            iovrecv[0].iov_base = &send_recv_hdr;
            iovrecv[0].iov_len = sizeof(struct hdr);
            iovrecv[1].iov_base = &request;
            iovrecv[1].iov_len = sizeof(request_t);
            Recvmsg(clifd, &msgrecv, 0);
            /* Test worker avaliability for reget worker */
            if (request.type == REGET) {
                /* check worker available */
                reget *tmp = (reget *)&request;
                int i;
                for (i=0; i<nworker; i++) {
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
                                workers[i].status = WKR_FLD;
#ifdef PROXY_DEBUG
                                printf("Worker %d failed\n", i);
#endif
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
                int loop = nworker;
                do {
                    wkrcur = (wkrcur +1)%nworker;
                } while((workers[wkrcur].status != WKR_RDY) && --loop);
                get_ack *result = (get_ack *)&response;
                if(workers[wkrcur].status == WKR_RDY) {
#ifdef PROXY_DEBUG
                    printf("select worker id: %u\n", wkrcur);
#endif
                    result->ack = 1;
                    result->port = workers[wkrcur].port;
                    result->addr = workers[wkrcur].addr;
                } else {
                    result->ack = 0;
                    result->port = 0;
                    result->addr = 0;
                }
                msgsend.msg_name = &rqstaddr;
                msgsend.msg_namelen = sizeof(rqstaddr);
                msgsend.msg_iov = iovsend;
                msgsend.msg_iovlen = 2;
                iovsend[0].iov_base = &send_recv_hdr;
                iovsend[0].iov_len = sizeof(struct hdr);
                iovsend[1].iov_base = &response;
                iovsend[1].iov_len = sizeof(response_t);
                Sendmsg(clifd, &msgsend, 0);
            }
        }
        /* Handle worker request */
        if (FD_ISSET(wkrfd, &rset)) {
            /* Recieve worker request message */
            msgrecv.msg_name = &rqstaddr;
            msgrecv.msg_namelen = sizeof(rqstaddr);
            msgrecv.msg_iov = iovrecv;
            msgrecv.msg_iovlen = 2;
            iovrecv[0].iov_base = &send_recv_hdr;
            iovrecv[0].iov_len = sizeof(struct hdr);
            iovrecv[1].iov_base = &request;
            iovrecv[1].iov_len = sizeof(request_t);
            Recvmsg(wkrfd, &msgrecv, 0);

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

                msgsend.msg_name = &rqstaddr;
                msgsend.msg_namelen = sizeof(rqstaddr);
                msgsend.msg_iov = iovsend;
                msgsend.msg_iovlen = 2;
                iovsend[0].iov_base = &send_recv_hdr;
                iovsend[0].iov_len = sizeof(struct hdr);
                iovsend[1].iov_base = &response;
                iovsend[1].iov_len = sizeof(response_t);
                Sendmsg(wkrfd, &msgsend, 0);

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
                    if (update_tmp->index == least_loaded) {
                        if (workers[least_loaded].load < update_tmp->load) {
                            workers[least_loaded].load = update_tmp->load;
                            int i;
                            for (i =0; i < nworker; i++) {
                                if (workers[i].load < workers[least_loaded].load) {
                                    least_loaded = i;
                                }
                            }
                            workers[least_loaded].status = WKR_RDY;
                        } else {
                            workers[least_loaded].load = update_tmp->load;
                        }
                    } else {
                        workers[update_tmp->index].load = update_tmp->load;
                        if (update_tmp->load > (workers[least_loaded].load + 50)) {
                            workers[update_tmp->index].status = WKR_OVL;
                        } else if (update_tmp->load >= workers[least_loaded].load) {
                            workers[update_tmp->index].status = WKR_RDY;
                        } else {
                            least_loaded = update_tmp->index;
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
                iovsend[0].iov_len = sizeof(struct hdr);
                iovsend[1].iov_base = &response;
                iovsend[1].iov_len = sizeof(response_t);
                Sendmsg(wkrfd, &msgsend, 0);
            }
        }
    }
}
