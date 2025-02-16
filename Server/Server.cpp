/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/06 14:44:46 by ael-qori          #+#    #+#             */
/*   Updated: 2025/02/16 16:26:53 by ael-qori         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

Server::Server() : currentStateServer(INIT), serverIndex(INDEX){}
Server::~Server()
{
    int index = INDEX;

    while (++index < this->res.size()) freeaddrinfo(this->res[index]);
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
            Error(2, "Error Server:: ", "in getaddre info");
    }
}

void Server::createLinkedListOfAddr()
{
    int index = INDEX;

    while (++index < this->configFile.servers.size())
        this->CreateAddrOfEachPort(index);
    currentStateServer = SOCKETS;
}

void Server::createSockets()
{
    int index = INDEX;
    int sockFD = INDEX;
    int opt = 1;
    int flags;

    while (++index < this->res.size())
    {
        sockFD = socket(this->res[index]->ai_family, this->res[index]->ai_socktype | SOCK_NONBLOCK, this->res[index]->ai_protocol);
        if (sockFD == -1) Error(2, "Error Server:: ", "sockets");
        if (setsockopt(sockFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) Error(3, "Error Server:: ", "setsockopt", "SO_REUSEADDR");
        if (setsockopt(sockFD, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) Error(3, "Error Server:: ", "setsockopt", "SO_REUSEPORT");
        this->socketContainer.push_back(sockFD);
    }
    currentStateServer = BIND;
}

void Server::bindSockets()
{
    int index = INDEX;
    int status = INDEX;

    while (++index < this->socketContainer.size())
    {
        status = bind(this->socketContainer[index], this->res[index]->ai_addr, this->res[index]->ai_addrlen);
        if (status == -1) Error(2, "Error Server:: ", "bind");
    }
    currentStateServer = LISTEN;
}

void Server::listenForConnection()
{
    int index = INDEX;
    int status = INDEX;

    while (++index < this->res.size())
    {
        status = listen(this->socketContainer[index], 10);
        if (status == -1) Error(2, "Error Server:: ", "listen");
    }
    currentStateServer = INIT_EPOLL;
}

void Server::init_epoll()
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    this->epollFD = epoll_create(1024);
    if (epollFD == -1) Error(2, "Error Server:: ", "epoll_create");
    this->registerAllSockets();
    currentStateServer = EPOLL;
}

void Server::registerAllSockets()
{
    size_t index = INDEX;

    while (++index < this->socketContainer.size())
    {
        this->event.events = EPOLLIN;
        this->event.data.fd = this->socketContainer[index];
        if (epoll_ctl(epollFD, EPOLL_CTL_ADD, this->socketContainer[index], &event) == -1) 
            (ServerLogger("Cannot Add Register Client", Logger::ERROR, false));
    }
}

void Server::addClientToEpoll(int clientFD)
{
    struct epoll_event event;
    
    event.data.fd = clientFD;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epollFD, EPOLL_CTL_ADD, clientFD, &event) == -1)
        (close(clientFD), ServerLogger("Cannot Add Client To Epoll", Logger::ERROR, false)); 
}

void Server::acceptConnection(int index)
{
    int                         acceptFD;
    socklen_t                   addrLen;
    struct sockaddr_storage     clientAddr;
    
    addrLen = sizeof(clientAddr);
    acceptFD = accept(events[index].data.fd, (struct sockaddr *)&clientAddr, &addrLen);
    if (acceptFD == -1)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)  (ServerLogger("Cannot Accept Connection", Logger::ERROR, false));
        return;
    }
    this->addClientToEpoll(acceptFD);
}

void Server::processData(int index)
{
    int             bytesReceived;
    char            buffer[1024];
    std::string     requestData;
    
    memset(buffer, 0, sizeof(buffer));
    bytesReceived = recv(events[index].data.fd, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived <= 0)
    {
        epoll_ctl(epollFD, EPOLL_CTL_DEL, events[index].data.fd, NULL);
        (close(events[index].data.fd), ServerLogger("Client disconnected", Logger::INFO, false));
        return;
    }
    ClientStatus.push_back(make_pair(events[index].data.fd, ConnectionStatus()));
    // std::cout << index<< "\t" << ClientStatus[0].first << "\t" <<ClientStatus[0].second.acceptTime << std::endl;
    requestData.append(buffer, bytesReceived);
    if (!requestData.empty())
        this->requestHandler.handleRequest(events[index].data.fd, requestData, epollFD);
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
    int index           = INDEX;
    std::string         fdStr, eventFlags;
    
    while (++index < this->nfds)
    {
        fdStr = itoa(this->events[index].data.fd);
        if (this->events[index].events & EPOLLIN) this->acceptAndAnswer(index);
        if (this->events[index].events & EPOLLOUT) (this->requestHandler.handleWriteEvent(epollFD, events[index].data.fd), this->CheckForTimeOut(events[index].data.fd));
    }
}

void Server::loopAndWait()
{
    ServerLogger("Server Started", Logger::INFO, false);
    while (true)
    {
        this->nfds = epoll_wait(epollFD, events, 1024, -1);
        if (this->nfds == -1) {
            if (errno == EINTR) {
                ServerLogger("epoll_wait interrupted by signal. Retrying...", Logger::WARNING, false);
                continue;
            } else {
                ServerLogger("epoll_wait failed: " + std::string(strerror(errno)), Logger::ERROR, false);
                Error(2, "Critical Error in Server:: ", "epoll_wait");
            }
        }
        if (this->nfds > 0) this->findServer();
    }
    close(epollFD);
}

void Server::init()
{
    memset(&this->hints, 0, sizeof(this->hints));
    this->hints.ai_family = AF_INET;
    this->hints.ai_flags = AI_PASSIVE;
    this->hints.ai_socktype = SOCK_STREAM;
    this->requestHandler.server_config = this->configFile;  
    currentStateServer = ADDR; 
}

void Server::start()
{

    switch (this->currentStateServer)
    {
        case INIT       :           (this->init(),                   ServerLogger(LOG_INIT, Logger::SUB, true)); 
        case ADDR       :           (this->createLinkedListOfAddr(), ServerLogger(LOG_ADDR, Logger::SUB, true));
        case SOCKETS    :           (this->createSockets(),          ServerLogger(LOG_SOCKETS, Logger::SUB, true)); 
        case BIND       :           (this->bindSockets(),            ServerLogger(LOG_BIND, Logger::SUB, true)); 
        case LISTEN     :           (this->listenForConnection(),    ServerLogger(LOG_LISTEN, Logger::SUB, true));
        case INIT_EPOLL :           (this->init_epoll(),             ServerLogger(LOG_INIT_EPOLL, Logger::SUB, true));
        default: break;
    }
    this->loopAndWait();
}

void    Server::ServerLogger(std::string message ,Logger::Level level , bool is_sub)
{
   usleep(500 * 1000);
   if (is_sub) this->logger.overwriteLine(message);
   else this->logger.log(level, message);
}

void    Server::CheckForTimeOut(int fd)
{
    int i = -1;

    while (++i < this->ClientStatus.size())
        if (ClientStatus[i].first == fd) break;
    if (i == this->ClientStatus.size()) return;
    if (ClientStatus[i].second.isTimedOut())
        (close(fd), ClientStatus.erase(ClientStatus.begin() + i));
}