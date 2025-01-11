#pragma once

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdexcept>

using namespace std;

class Socket {
public:
    Socket(int port);
    ~Socket();

    void bindSocket();
    void listenSocket();
    int acceptConnection();
    int getPort()const;
    int getSocketFd()const;

private:
    int sockfd;
    sockaddr_in addr;
    int port;
};

