/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/06 14:44:46 by ael-qori          #+#    #+#             */
/*   Updated: 2025/02/24 15:26:13 by ael-qori         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

Server::Server() :  serverIndex(INDEX), currentStateServer(CONF){}

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
            Error(2, ERR_SERVER, ERR_GETADDRINFO);
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
        if (sockFD == -1) Error(2, ERR_SERVER, ERR_SOCKETS);
        if (setsockopt(sockFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) Error(3, ERR_SERVER, ERR_SETSOCKOPT, ERR_REUSEADDR);
        if (setsockopt(sockFD, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) Error(3, ERR_SERVER, ERR_SETSOCKOPT, ERR_REUSEPORT);
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
        if (status == -1) Error(2, ERR_SERVER, ERR_BIND);
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
        if (status == -1) Error(2, ERR_SERVER, ERR_LISTEN);
    }
    currentStateServer = INIT_EPOLL;
}

void Server::init_epoll()
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, handleSignalInterrupt);
    this->epollFD = epoll_create(1);
    *(int*)GlobalEpollFd() = this->epollFD;
    if (epollFD == -1) Error(2, ERR_SERVER, ERR_EPOLL_CREATE);
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
        if (epoll_ctl(epollFD, EPOLL_CTL_ADD, this->socketContainer[index], &event) == -1) (ServerLogger(ERR_CANNOT_REGISTER_CLIENT, Logger::ERROR, false));
    }
}

void Server::addClientToEpoll(int clientFD)
{
    struct epoll_event event;
    
    event.data.fd = clientFD;
    event.events = EPOLLIN;
    if (epoll_ctl(epollFD, EPOLL_CTL_ADD, clientFD, &event) == -1) (close(clientFD), ServerLogger(ERR_CANNOT_ADD_CLIENT, Logger::ERROR, false)); 
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
        if (errno != EAGAIN && errno != EWOULDBLOCK)  (ServerLogger(ERR_CANNOT_ACCEPT, Logger::ERROR, false));
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
    if (bytesReceived <= 0)
    {
        (this->requestHandler.cleanupConnection(epollFD, events[index].data.fd), deleteFromTimeContainer(events[index].data.fd));
        return;
    }
    this->resetTime(events[index].data.fd);
    requestData.append(buffer, bytesReceived);
    if (!requestData.empty()) this->requestHandler.handleRequest(events[index].data.fd, requestData,bytesReceived,epollFD, findCorrectServers(IndexPorts[IndexServer[events[index].data.fd]])
 );
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
    ServerLogger(LOG_START, Logger::INFO, false);
    while (true)
    {
        this->nfds = epoll_wait(epollFD, events, MAX_EVENTS, TIMEOUT_EPOLL);
        if (this->nfds == -1) {
            if (errno == EINTR) {
                ServerLogger(ERR_EPOLL_INTERRUPTED, Logger::WARNING, false);
                continue;
            } else Error(2, ERR_EPOLL_CRITICAL, W_EPOLL_WAIT);
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
    currentStateServer = ADDR; 
}

void Server::start()
{

    switch (this->currentStateServer)
    {
        case CONF       :           (this->checkConfigFile(),        ServerLogger(LOG_CONF, Logger::SUB, true));
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
   usleep(SLEEP_LOGGER);
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

std::vector<ServerConfig>    Server::findCorrectServers(int fd)
{
    std::string                 host;
    std::vector<int>            ports;
    size_t                      index;
    std::vector<ServerConfig>   server;

    index = INDEX;
    host = this->configFile.servers[fd].getHost();
    ports = this->configFile.servers[fd].getPorts();

    while (++index < this->configFile.servers.size())
    {
        if (host != this->configFile.servers[index].getHost())
            continue;
        if (hasCommonElement(ports, this->configFile.servers[index].getPorts()) == false)
            continue;
        server.push_back(this->configFile.servers[index]);
    }
    return server;
}

bool    Server::hasCommonElement(std::vector<int>& v1, std::vector<int> v2) {
    for (std::vector<int>::const_iterator it = v1.begin(); it != v1.end(); ++it) {
        if (std::find(v2.begin(), v2.end(), *it) != v2.end()) {
            return true;
        }
    }
    return false; 
}


bool   ConnectionStatus::isTimedOut() const
{
    return difftime(lastActivityTime, acceptTime) >= TIMEOUT;
}

void    Server::checkConfigFile()
{
    if (CheckForConflictInServerName()) Error(2, ERR_CONF, W_SERVER_NAMES);
    if (!checkValidLocationPath()) Error(2, ERR_CONF, W_LOCATION);
    if (checkForDuplicatedLocations()) Error(2, ERR_CONF, W_LOCATION);
    if (checkForDuplicatePortsInTheSameServer()) Error(2, ERR_CONF, W_PORT);
    currentStateServer = INIT;
}

bool    Server::CheckForConflictInServerName()
{
    size_t                      index;
    std::vector<std::string>    server_names;

    index = INDEX;
    while (++index < this->configFile.servers.size())
    {
        const std::vector<std::string>& names = this->configFile.servers[index].getServerNames();
        server_names.insert(server_names.end(), names.begin(), names.end());
    }
    return ::isDuplicated<std::vector<std::string> >(server_names);
}

bool    Server::checkForDuplicatedLocations()
{
    size_t                         index, iter;
    std::vector<std::string>        locations;
    
    index = INDEX;
    while (++index < this->configFile.servers.size())
    {
        (locations.clear(), iter = INDEX);
        while (++iter < this->configFile.servers[index].getLocations().size())
            locations.push_back(this->configFile.servers[index].getLocations()[iter].getPath());
        if (::isDuplicated<std::vector<std::string> >(locations)) return true;
    }
    return false;
}

bool    Server::checkForDuplicatePortsInTheSameServer()
{
    size_t                        index;

    index = INDEX;
    while(++index < this->configFile.servers.size())
        if (::isDuplicated<std::vector<int> >(this->configFile.servers[index].getPorts())) return true;
    return false;
}

bool    Server::checkValidLocationPath()
{
    size_t              index, iter = INDEX;

    index = INDEX;
    while ((iter = INDEX) && ++index < this->configFile.servers.size())
        while (++iter < this->configFile.servers[index].getLocations().size())
            if (isValidPath(this->configFile.servers[index].getLocations()[iter].getPath()) == false) return false;
    regenarateNewValidPath();
    return true;
}

bool    Server::isValidPath(std::string str)
{
    size_t              index;

    index = INDEX;
    if (str[++index] != SLASH) return false;
    while (++index < str.size())
        if (isValidCharInPath(str[index]) == false) return false;
    return true;
}

void    Server::regenarateNewValidPath()
{
    size_t              index, iter = INDEX;
    std::string         path;

    index = INDEX;
    while ((iter = INDEX) && ++index < this->configFile.servers.size())
        while (++iter < this->configFile.servers[index].getLocations().size())
           (path = simplifyPath(this->configFile.servers[index].getLocations()[iter].getPath()), this->configFile.servers[index].getLocations()[iter].setPath(path));
}


std::string    Server::simplifyPath(std::string str)
{
    size_t                      index;
    std::string                 path;
    std::vector<std::string>    Array;

    index =INDEX;
    path += SLASH;
    Array = splitString(str, SLASH_STR);
    while (++index < Array.size())
        path += Array[index];
    return path;
}
