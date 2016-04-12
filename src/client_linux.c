#include "rtt.h"
#include "server.h"


int send_and_get (const char *data, const char *result)
{
    request_t request;
    response_t response;
    int proxyfd, workerfd;
    struct sockaddr_in proxyaddr, workeraddr;

    proxyfd = Socket(AF_INET, SOCK_STREAM, 0);
    memset(&proxyaddr, 0, sizeof(proxyaddr));
    proxyaddr.sin_family = AF_INET;
    proxyaddr.sin_port = htons(PROXY_PORT);
    inet_pton(AF_INET, PROXY_ADDR, &proxyaddr.sin_addr);

    get *get_tmp = &request;
    get->type = GET;
    Dg_send_recv(proxyfd, &request, sizeof(request), &response, sizeof(response), &proxyaddr, sizeof(proxyaddr));
    get_ack *get_ack_tmp = &response;
    workerfd = Socket(AF_INET, SOCK_STREAM, 0);
    memset(&workeraddr, 0, sizeof(workeraddr);
    workeraddr.sin_family = AF_INET;
    workeraddr.sin_port = get_ack_tmp->port;
    workeraddr.sin_addr = get_ack_tmp->addr;

    Connect(workerfd, (struct sockaddr *)&workeraddr, sizeof(workeraddr));

    Close(workerfd);
    sleep(100);
    

}


