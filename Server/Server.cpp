/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aes-sarg <aes-sarg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/06 14:44:46 by ael-qori          #+#    #+#             */
/*   Updated: 2025/02/21 16:00:45 by aes-sarg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

Server::Server() :  serverIndex(INDEX), currentStateServer(INIT){}

ConnectionStatus::ConnectionStatus(){
    time_t now = time(NULL);
    this->acceptTime = now;
    this->lastActivityTime = now;
}

Server::~Server()
{
    size_t index = INDEX;

    while (++index < this->res.size()) freeaddrinfo(this->res[index]);
}

void Server::CreateAddrOfEachPort(int serverIndex)
{
    static int  indexRes  = 0; 
    static int  indexPort = 3;
    size_t      index     = INDEX;

    while (++index < this->configFile.servers[serverIndex].getPorts().size())
    {
        addrinfo *res = NULL;
        this->res.push_back(res);
        if (getaddrinfo(this->configFile.servers[serverIndex].getHost().c_str(),
                        itoa(this->configFile.servers[serverIndex].getPorts()[index]).c_str(),
                        &this->hints,
                        &this->res[indexRes++]) != 0)
            Error(2, "Error Server:: ", "in getaddre info");
        IndexPorts.insert(pair<int, int>(indexPort, serverIndex));
        indexPort++;
    }
}

void Server::createLinkedListOfAddr()
{
    size_t index = INDEX;

    while (++index < this->configFile.servers.size())
        this->CreateAddrOfEachPort(index);
    currentStateServer = SOCKETS;
}

void Server::createSockets()
{
    size_t  index = INDEX;
    int     sockFD = INDEX;
    int     opt = 1;

    while (++index < this->res.size())
    {
        sockFD = socket(this->res[index]->ai_family, this->res[index]->ai_socktype | SOCK_NONBLOCK, this->res[index]->ai_protocol);
        if (sockFD == -1) Error(2, "Error Server:: ", "sockets");
        if (setsockopt(sockFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) Error(3, "Error Server:: ", "setsockopt", "SO_REUSEADDR");
        // if (setsockopt(sockFD, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) Error(3, "Error Server:: ", "setsockopt", "SO_REUSEPORT");
        this->socketContainer.push_back(sockFD);
    }
    currentStateServer = BIND;
}

void Server::bindSockets()
{
    size_t  index = INDEX;
    int     status = INDEX;

    while (++index < this->socketContainer.size())
    {
        status = bind(this->socketContainer[index], this->res[index]->ai_addr, this->res[index]->ai_addrlen);
        if (status == -1) Error(2, "Error Server:: ", "bind");
    }
    currentStateServer = LISTEN;
}

void Server::listenForConnection()
{
    size_t  index = INDEX;
    int     status = INDEX;

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
    this->epollFD = epoll_create(1);
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
        if (epoll_ctl(epollFD, EPOLL_CTL_ADD, this->socketContainer[index], &event) == -1) (ServerLogger("Cannot Add Register Client", Logger::ERROR, false));
    }
}

void Server::addClientToEpoll(int clientFD)
{
    struct epoll_event event;
    
    event.data.fd = clientFD;
    event.events = EPOLLIN;
    if (epoll_ctl(epollFD, EPOLL_CTL_ADD, clientFD, &event) == -1) (close(clientFD), ServerLogger("Cannot Add Client To Epoll", Logger::ERROR, false)); 
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
    ClientStatus.push_back(make_pair(acceptFD, ConnectionStatus()));
    IndexServer[acceptFD] = events[index].data.fd;
    this->addClientToEpoll(acceptFD);
}

void Server::processData(int index)
{
    int             bytesReceived;
    char            buffer[BUFFER_SIZE_SERVER];
    std::string     requestData;
    
    memset(buffer, 0, sizeof(buffer));
    bytesReceived = recv(events[index].data.fd, buffer, sizeof(buffer) - 1, 0);

    // std::cout << "IndexServer: " << IndexServer[events[index].data.fd] << std::endl;
    // std::cout << "IndexPorts" << IndexPorts[IndexServer[events[index].data.fd]] << std::endl;
        if (bytesReceived <= 0)
    {
        (this->requestHandler.cleanupConnection(epollFD, events[index].data.fd), deleteFromTimeContainer(events[index].data.fd),ServerLogger("Client disconnected", Logger::INFO, false));
        return;
    }
    this->resetTime(events[index].data.fd);
    requestData.append(buffer, bytesReceived);
    if (!requestData.empty()) this->requestHandler.handleRequest(events[index].data.fd, requestData, epollFD, configFile.servers[IndexPorts[IndexServer[events[index].data.fd]]] );
}

void Server::acceptAndAnswer(int index)
{
    if (std::find(this->socketContainer.begin(), this->socketContainer.end(), events[index].data.fd) != this->socketContainer.end()) acceptConnection(index);
    else processData(index);
}

void Server::findServer()
{
    int index     = INDEX;
    
    while (++index < this->nfds)
    {
        if (this->events[index].events & EPOLLIN) this->acceptAndAnswer(index);
        if (this->events[index].events & EPOLLOUT) (this->requestHandler.handleWriteEvent(epollFD, events[index].data.fd), this->resetTime(events[index].data.fd));
    }
}

void Server::loopAndWait()
{
    ServerLogger("Server Started", Logger::INFO, false);
    while (true)
    {
        this->nfds = epoll_wait(epollFD, events, MAX_EVENTS, 5000);
        if (this->nfds == -1) {
            if (errno == EINTR) {
                ServerLogger("epoll_wait interrupted by signal. Retrying...", Logger::WARNING, false);
                continue;
            } else {
                ServerLogger("epoll_wait failed: " + std::string(strerror(errno)), Logger::ERROR, false);
                Error(2, "Critical Error in Server:: ", "epoll_wait");
            }
        }
        if (this->nfds == 0) this->timeoutChecker();
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
    // this->requestHandler.server_config = this->configFile;  
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
    size_t i = INDEX;

    while (++i < this->ClientStatus.size())
        if (ClientStatus[i].first == fd) break;
    if (i == this->ClientStatus.size()) return;

    if (ClientStatus[i].second.isTimedOut())
        (ClientStatus.erase(ClientStatus.begin() + i),send(fd, TIMEOUT_MESSAGE, strlen(TIMEOUT_MESSAGE),0),requestHandler.cleanupConnection(epollFD,fd));
}

void    Server::timeoutChecker()
{
    size_t i = INDEX;

    while (++i < this->ClientStatus.size())
        (updateTime(this->ClientStatus[i].first),CheckForTimeOut(this->ClientStatus[i].first));
}

void    Server::updateTime(int fd)
{
    size_t i = INDEX;

    while (++i < this->ClientStatus.size())
        if (ClientStatus[i].first == fd) break;
    if (i == this->ClientStatus.size()) return;
    ClientStatus[i].second.lastActivityTime = time(NULL);
}

void    Server::resetTime(int fd)
{
    size_t i = INDEX;

    while (++i < this->ClientStatus.size())
        if (ClientStatus[i].first == fd) break;
    if (i == this->ClientStatus.size()) return;
    ClientStatus[i].second.acceptTime = time(NULL);
    ClientStatus[i].second.lastActivityTime = time(NULL);
}

void    Server::deleteFromTimeContainer(int fd)
{
    size_t i = INDEX;
    
    while (++i < this->ClientStatus.size())
        if (ClientStatus[i].first == fd) break;
    if (i == this->ClientStatus.size()) return;
    ClientStatus.erase(ClientStatus.begin() + i);
}

ServerConfig    Server::findCorrectServer(int fd)
{
    std::cout << IndexServer[fd] << std::endl;
}

bool   ConnectionStatus::isTimedOut() const
{
    return difftime(lastActivityTime, acceptTime) >= TIMEOUT;
}
