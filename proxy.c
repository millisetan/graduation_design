#include <server.h>

server_rec       serv;
server_conf      serv_conf;

pthread_mutex_t rqst_cnt_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rqst_rec_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  rqst_cnt_cond  = PTHREAD_COND_INITIALIZER;


int main(int argc, char **argv)
{
    /* Server set up, option handle */


    /* Server tcp set up */
    int     sockfd;
    struct  sockaddr_in servaddr;

    sockfd = Socker(AF_INET, SOCK_STREAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = serv_conf.port;
    servaddr.sin_addr.s_addr   = serv_conf.addr;

    Connect(sockfd, (sockaddr *) &servaddr, sizeof(servaddr));

    /* Main thread for loop */
    for ( ; ; ) {

        /* Server overloaded ? */
        while (rqst_cnt >= ) {
            Pthread_cond_wait(&rqst_cnt_cond, &rqst_cnt_mutex);
        }

        request_rec *rqst_rec  = malloc(sizeof(request_rec));
        serv.pending_end->next = rqst_rec;
        rqst_rec->pre          = serv.pending_end;
        serv.pending_end       = rqst_rec;

        rqst_rec->clilen = sizeof(rqst_rec->cliaddr);
        rqst_rec->connfd = Accept(sockfd, rqst_rec->cliaddr,&rqst_rec->clilen);
        rqst_rec->worker = serv.balancer->balancer_method();
        

        pthread_t tid;
        Pthread_creat(&tid, NULL, rqst_handle, rqst_rec);

    }

}

