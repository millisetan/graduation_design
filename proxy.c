#include "server.h"
#include "rtt.h"


int main()
{
    int clifd, wkrfd, testfd, option =1, maxfdp, activity, wkrcur;
    int nworker;
    int least_loaded;
    int mesg_len;
    request_t request;
    response_t response;
    worker *workers;
    fd_set rset;
    struct hdr {
        uint32_t seq;
        uint32_t ts;
    } send_recv_hdr;
    struct msghdr msgsend, msgrecv;
    struct iovec iovsend[2], iovrecv[2];
    struct sockaddr_in clilstnaddr, wkrlstnaddr, rqstaddr, testaddr;

    /* Listenning for client */
    clifd = Socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(clifd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    memset(&clilstnaddr, 0, sizeof(clilstnaddr));
    clilstnaddr.sin_family = AF_INET;
    clilstnaddr.sin_port = htons(SERV_PORT);
    clilstnaddr.sin_addr.s_addr = htonl(SERV_IP);
    Bind(clifd, (struct sockaddr *)&clilstnaddr, sizeof(clilstnaddr))
    
    /* Listenning for worker */
    wkrfd = Socket(AF_INET, SOCK_DGRAM, 0);
    Setsockopt(wkrfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    memset(&wkrlstnaddr, 0, sizeof(wkrlstnaddr));
    wkrlstnaddr.sin_family = AF_INET;
    wkrlstnaddr.sin_port = htons(PROXY_PORT);
    wkrlstnaddr.sin_addr.s_addr = htonl(PROXY_IP);
    Bind(wkrfd, (struct sockaddr *)&wkrlstnaddr, sizeof(wkrlstnaddr));

    FD_ZERO(&rset);
    for ( ; ; ) {
        FD_SET(clifd, &rset);
        FD_SET(wkrfd, &rset);
        activity  = Select( (clifd>wkrfd ? clifd : wkrfd)+1, &rest, NULL, NULL, NULL);

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
            iovrecv[1].iov_len = sizeof(requst_t);
            Recvmsg(clifd, &msgrecv, 0);
            /* Test worker avaliability for reget worker */
            if (request.type == RERQST) {
                /* check worker available */
                reget *tmp = &request;
                for (int i=0; i<nworker; i++) {
                    if ((tmp->port == workers[i]->port) &&
                        (tmp->addr == workers[i]->addr)) {
                        if (workers[i]->status == WKR_FLD) break;
                        testfd = Socket(AF_INET, SOCK_STREAM, 0);
                        memset(&testaddr, 0, sizeof(testaddr));
                        testaddr.sin_family = AF_INET;
                        testaddr.sin_port = tmp.port;
                        testaddr.sin_addr = tmp.addr;
                        if ((connect(testfd, (struct sockaddr *)&testaddr,
                                     sizeof(testaddr)) < 0) &&
                            (errno == ECONNREFUSED)) {
                                workers[i]->status = WKR_FLD;
                        }
                        close(testfd);
                        break;
                    }
                }
            }
            /* Send response back */
            if ((request.type == RERQST) || (request.type == RERQST)) {
                do {
                    wkrcur = (wkrcur +1)%nworker;
                } while(workers[wkrcur]->status != WKR_RDY);
                get_ack *result = &response;
                result->ack = 1;
                result->port = wkrcur.port;
                result->addr = wkrcur.addr;
                msgsend.msg_name = &rqstaddr;
                msgsend.msg_namelen = sizeof(rqstaddr);
                msgsend.msg_iov = iovsend;
                msgsend.msg_iovlen = 2;
                iovsend[0].iov_base = &send_send_hdr;
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
            iovrecv[1].iov_len = sizeof(requst_t);
            Recvmsg(wkrfd, &msgrecv, 0);

            /* Handle worker register */
            if (request.type == RGST) {
                regist *tmp = request;
                int i;
                for (i=0; i<nworker; i++) {
                    if ((tmp->port == workers[i]->port) &&
                        (tmp->addr == workers[i]->addr)) {
                        worker *wkr_tmp = workers[i];
                        wkr_tmp->status = WKR_RDY;
                        wkr_tmp->load = 0;
                        break;
                    }
                }
                if (i == nworker) {
                    workers = realloc(workers, sizeof(worker)*(++nworker));
                    if (!workers) {
                        /* fatal error */
                    }
                    worker *wkr_tmp = workers[nworker-1];
                    wkr_tmp->status = WKR_RDY;
                    wkr_tmp->passwd = random();
                    wkr_tmp->load = 0;
                    wkr_tmp->port = tmp->port;
                    wkr_tmp->addr = tmp->addr;
                }
                regist_ack *result = &response;
                result->ack = 1;
                result->index = i;
                result->passwd = workers[i]->passwd;
                msgsend.msg_name = &rqstaddr;
                msgsend.msg_namelen = sizeof(rqstaddr);
                msgsend.msg_iov = iovsend;
                msgsend.msg_iovlen = 2;
                iovsend[0].iov_base = &send_send_hdr;
                iovsend[0].iov_len = sizeof(struct hdr);
                iovsend[1].iov_base = &response;
                iovsend[1].iov_len = sizeof(response_t);
                Sendmsg(wkrfd, &msgsend, 0);

            }
            /* Handle worker information update */
            else (requst.type == UPDT) {
                update *update_tmp = &request;
                int valid =0;
                if (workers[update_tmp->index]->passwd == update_tmp->passwd) {
                    /* If worker reconnect to proxy */
                    if (workers[update_tmp->index]->status == WKR_FLD) {
                        workers[update_tmp->index]->status == WKR_RDY;
                    }
                    valid = 1;
                    if (update_tmp->index == least_loaded) {
                        if (workers[least_loaded]->load < update_tmp->load) {
                            workers[least_loaded]->load = update_tmp->load;
                            for (int i =0; i < nworker; i++) {
                                if ((workers[i]->status == WKR_RDY) &&
                                    (workers[i]->load < update_tmp->load)) {
                                    least_loaded = i;
                                }
                            }
                        } else {
                            workers[least_loaded]->load = update_tmp->load;
                        }
                    } else {
                        workers[update_tmp->index]->load = update_tmp->load;
                        if (update_tmp->load > (workers[least_loaded]->load + 50)) {
                            workers[update_tmp->index]->status = WKR_OVL;
                        } else if (update_tmp->load >= workers[least_loaded]->load) {
                            workers[update_tmp->index]->status = WKR_RDY;
                        } else {
                            least_loaded = update_tmp->index;
                        }
                    }
                }
                update_ack *result = &response;
                result->ack = valid;
                msgsend.msg_name = &rqstaddr;
                msgsend.msg_namelen = sizeof(rqstaddr);
                msgsend.msg_iov = iovsend;
                msgsend.msg_iovlen = 2;
                iovsend[0].iov_base = &send_send_hdr;
                iovsend[0].iov_len = sizeof(struct hdr);
                iovsend[1].iov_base = &response;
                iovsend[1].iov_len = sizeof(response_t);
                Sendmsg(wkrfd, &msgsend, 0);
            }
        }
    }
}
