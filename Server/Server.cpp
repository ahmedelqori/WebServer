/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aes-sarg <aes-sarg@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/06 14:44:46 by ael-qori          #+#    #+#             */
/*   Updated: 2025/01/17 16:11:57 by aes-sarg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

Server::Server() : serverIndex(INDEX)
{
}

void Server::CreateAddrOfEachPort(int serverIndex)
{
    static int  indexRes  = 0; 
    int         index     = INDEX;

    while (++index < this->configFile.servers[serverIndex].getPorts().size())
    {
        addrinfo *res = NULL;
        this->res.push_back(res);
        if (getaddrinfo(this->configFile.servers[serverIndex].getHost().c_str(),
                        itoa(this->configFile.servers[serverIndex].getPorts()[index]).c_str(),
                        &this->hints,
                        &this->res[indexRes++]) != 0)
            throw std::runtime_error("Error in getaddre info");
    }
}

void Server::createLinkedListOfAddr()
{
    int index = INDEX;

    while (++index < this->configFile.servers.size())
        this->CreateAddrOfEachPort(index);
}

void Server::createSockets()
{
    int index = INDEX;
    int sockFD = INDEX;
    int opt = 1;

    while (++index < this->res.size())
    {
        sockFD = socket(this->res[index]->ai_family, this->res[index]->ai_socktype, this->res[index]->ai_protocol);
        if (sockFD == -1) Error(2, "Error Server:: ", "sockets");
        if (setsockopt(sockFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) Error(2, "Error Server:: ", "setsockopt");
        if (fcntl(sockFD, F_SETFL, O_NONBLOCK) < 0) Error(2, "Error Server:: ", "fcntl - non-blocking");
        this->socketContainer.push_back(sockFD);
    }
}

void Server::bindSockets()
{
    int index = INDEX;
    int status = INDEX;

    while (++index < this->socketContainer.size())
    {
        status = bind(this->socketContainer[index], this->res[index]->ai_addr, this->res[index]->ai_addrlen);
        if (status == -1)
            Error(2, "Error Server:: ", "bind");
    }
}

void Server::listenForConnection()
{
    int index = INDEX;
    int status = INDEX;
    while (++index < this->res.size())
    {
        status = listen(this->socketContainer[index], 10);
        if (status == -1)
            Error(2, "Error Server:: ", "listen");
    }
}

void Server::init_epoll()
{
    this->epollFD = epoll_create(1024);
    if (epollFD == -1)
        Error(2, "Error Server:: ", "epoll_create");
    this->registerAllSockets();
}

void Server::registerAllSockets()
{
    size_t index = INDEX;

    while (++index < this->socketContainer.size())
    {
        this->event.events = EPOLLIN;
        this->event.data.fd = this->socketContainer[index];
        if (epoll_ctl(epollFD, EPOLL_CTL_ADD, this->socketContainer[index], &event) == -1)
        {
            Error(2, "Error Server:: ", "epoll_ctl (add socket)");
        }
    }
}

void Server::acceptConnection(int index)
{
    struct sockaddr_storage clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int acceptFD = accept(events[index].data.fd, (struct sockaddr *)&clientAddr, &addrLen);

    if (acceptFD == -1)
        Error(2, "Error Server:: ", "accept");

    struct epoll_event event;
    event.events = EPOLLIN; //| EPOLLET; // I comment EPOLLET , cause I faced an issue when uploading chunked data. read the manual to know about it :).
    event.data.fd = acceptFD;
    if (epoll_ctl(epollFD, EPOLL_CTL_ADD, acceptFD, &event) == -1)
    {
        close(acceptFD);
        Error(2, "Error Server:: ", "epoll_ctl (add acceptFD)");
    }

    std::cout << "New client connected, fd: " << acceptFD << std::endl;
}

void Server::processData(int index)
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    int bytesReceived = recv(events[index].data.fd, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0)
    {
        epoll_ctl(epollFD, EPOLL_CTL_DEL, events[index].data.fd, NULL);
        close(events[index].data.fd);
        std::cout << "Client disconnected, fd: " << events[index].data.fd << std::endl;
        return;
    }

    string requestData;

    requestData.append(buffer, bytesReceived);
    if (!requestData.empty())
    {
        this->requestHandler.handleRequest(events[index].data.fd, requestData, epollFD);
    }

}

void Server::acceptAndAnswer(int index)
{
    if (std::find(this->socketContainer.begin(), this->socketContainer.end(), events[index].data.fd) != this->socketContainer.end())
        acceptConnection(index);
    else
        processData(index);
}

void Server::findServer()
{
    int index = INDEX;
    while (++index < this->nfds)
        if (this->events[index].events & EPOLLIN)
            this->acceptAndAnswer(index);
        else if (this->events[index].events & EPOLLOUT)
            this->requestHandler.handleWriteEvent(epollFD, events[index].data.fd);
}

void Server::loopAndWait()
{
    while (true)
    {
        this->nfds = epoll_wait(epollFD, events, 1024, -1);
        if (this->nfds == -1)
            Error(2, "Error Server:: ", "epoll_wait");
        this->findServer();
    }
    close(epollFD);
}

void Server::init()
{
    char addr[INET6_ADDRSTRLEN];

    memset(&this->hints, 0, sizeof(this->hints));
    this->hints.ai_family = AF_INET;
    this->hints.ai_socktype = SOCK_STREAM;
    this->hints.ai_flags = AI_PASSIVE;

    this->requestHandler.server_config = this->configFile;
    
    // this->requestHandler(this->configFile);
    this->createLinkedListOfAddr();
    this->createSockets();
    this->bindSockets();
    this->listenForConnection();
}

void Server::start()
{
    this->init();
    this->init_epoll();
    this->loopAndWait();
}
