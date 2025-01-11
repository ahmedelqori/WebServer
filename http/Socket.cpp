#include "Socket.hpp"

Socket::Socket(int port) : port(port)
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        throw runtime_error("Failed to create socket");
    }
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        throw std::runtime_error("Failed to set SO_REUSEADDR option");
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
}

Socket::~Socket()
{
    close(sockfd);
}

void Socket::bindSocket()
{
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        throw runtime_error("Failed to bind socket");
    }
}

void Socket::listenSocket()
{
    if (listen(sockfd, 5) < 0)
    {
        throw runtime_error("Failed to listen on socket");
    }
}

int Socket::acceptConnection()
{
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
    if (client_sockfd < 0)
    {
        throw runtime_error("Failed to accept connection");
    }
    return client_sockfd;
}

int Socket::getPort() const
{
    return port;
}

int Socket::getSocketFd() const
{
    return sockfd;
}