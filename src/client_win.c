#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define SERV_PORT 27015
#define SERV_ADDR "45.62.113.14"
#define GET_WORKER 0;
#define REGET_WORKER 1;


int __cdecl senddata(char *dataDath, char *resultPath) 
{
    WSADATA wsaData;
    SOCKET servsock = INVALID_SOCKET, workersock = INVALID_SOCKET;
    char buf[DEFAULT_BUFLEN];
    struct sockaddr_in servaddr, workeraddr;
    int child_process, rqst_type = GET_WORKER;
    
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    // Create a SOCKET for connecting to server
    servsock = socket(AF_INET, SOCK_STREAM, 0);
    if (servsock == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to worker
    workersock = socket(AF_INET, SOCK_STREAM, 0);
    if (workersock == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }


    // Connect to server.
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr = inet_addr(SERV_ADDR);
    iResult = connect(ConnectSocket, (struct sockaddr *)servaddr, sizeof(servaddr));
    if (iResult == SOCKET_ERROR) {
        printf("socker failed to connect server: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    if (rqst_type == GET_WORKER) {
        iResult = send(

    } else if (rqst_type == REGET_WORKER) {

    } else {

    }


    iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %ld\n", iResult);

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Open data file for send
    inFile = fopen(dataPath, "r");
    if (inFile == NULL) {
        printf("valid data file");
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Get file length
    fseek(inFile, 0L, SEEK_END);
    int fileLength = ftell(inFile);
    fseek(inFile, 0L, SEEK_SET);

    // Receive until the peer closes the connection
    do {

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if ( iResult > 0 )
            printf("Bytes received: %d\n", iResult);
        else if ( iResult == 0 )
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while( iResult > 0 );

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}
