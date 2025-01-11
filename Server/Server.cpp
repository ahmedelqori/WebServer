/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/06 14:44:46 by ael-qori          #+#    #+#             */
/*   Updated: 2025/01/08 10:47:02 by ael-qori         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include <sys/epoll.h> // Include epoll header

Server::Server():serverIndex(INDEX){}


void    Server::createLinkedListOfAddr()
{
    int index = INDEX;

    while (++index < this->configFile.servers.size())
    {
        addrinfo* res = NULL;
        this->res.push_back(res);
        if (getaddrinfo(this->configFile.servers[index].getHost().c_str(),
            itoa(this->configFile.servers[index].getPort()).c_str(), 
            &this->hints , 
            &this->res[index]) != 0) 
            throw std::runtime_error("Error in getaddre info");
    }
}

void    Server::createSockets()
{
    int index = INDEX;
    int sockFD = INDEX;

    while (++index < this->configFile.servers.size())
    {
        sockFD = socket(this->res[index]->ai_family, this->res[index]->ai_socktype, this->res[index]->ai_protocol);
        if (sockFD == -1)
            Error(2, "Error Server:: ", "sockets");
        this->socketContainer.push_back(sockFD);
    }
}

void    Server::bindSockets()
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

void    Server::listenForConnection()
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


// void Server::acceptAndAnswer()
// {
//     int index = INDEX;
//     int acceptFD = INDEX;
//     char buffer[1024];

//     while (++index < this->res.size())
//     {
//         acceptFD = accept(this->socketContainer[index], this->res[index]->ai_addr, &this->res[index]->ai_addrlen);
//         if (acceptFD == -1)
//         {
//             Error(2, "Error Server:: ", "accept");
//             continue;
//         }

//         memset(buffer, 0, sizeof(buffer));
//         int bytesReceived = recv(acceptFD, buffer, sizeof(buffer) - 1, 0);
//         if (bytesReceived == -1)
//         {
//             Error(2, "Error Server:: ", "recv");
//             close(acceptFD);
//             continue;
//         }
//         buffer[bytesReceived] = '\0'; 
//         std::cout << "Received: " << buffer << std::endl;

//         std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
//         if (send(acceptFD, response.c_str(), response.size(), 0) == -1)
//         {
//             Error(2, "Error Server:: ", "send");
//         }

//         close(acceptFD);
//     }
// }

void    Server::init()
{
    char addr[INET6_ADDRSTRLEN];
    memset(&this->hints, 0, sizeof(this->hints));
    this->hints.ai_family = AF_INET;
    this->hints.ai_socktype = SOCK_STREAM;
    this->hints.ai_flags = AI_PASSIVE;
    this->createLinkedListOfAddr();
    this->createSockets();
    this->bindSockets();
    this->listenForConnection();
    // this->acceptAndAnswer();
}


// Updated Server class methods
void Server::start() {
    this->init();

    int epollFD = epoll_create1(0); // Create an epoll instance
    if (epollFD == -1) {
        Error(2, "Error Server:: ", "epoll_create1");
    }

    struct epoll_event event;
    struct epoll_event events[1024]; // Array to store triggered events
    int nfds;

    // Register all sockets to epoll
    for (size_t i = 0; i < this->socketContainer.size(); ++i) {
        event.events = EPOLLIN; // Monitor for read events
        event.data.fd = this->socketContainer[i];
        if (epoll_ctl(epollFD, EPOLL_CTL_ADD, this->socketContainer[i], &event) == -1) {
            Error(2, "Error Server:: ", "epoll_ctl (add socket)");
        }
    }

    while (true) {
        nfds = epoll_wait(epollFD, events, 1024, -1); // Wait for events
        if (nfds == -1) {
            Error(2, "Error Server:: ", "epoll_wait");
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].events & EPOLLIN) {
                if (std::find(this->socketContainer.begin(), this->socketContainer.end(), events[i].data.fd) != this->socketContainer.end()) {
                    // Incoming connection
                    struct sockaddr_storage clientAddr;
                    socklen_t addrLen = sizeof(clientAddr);
                    int acceptFD = accept(events[i].data.fd, (struct sockaddr *)&clientAddr, &addrLen);
                    if (acceptFD == -1) {
                        Error(2, "Error Server:: ", "accept");
                        continue;
                    }

                    // Register the accepted socket with epoll
                    event.events = EPOLLIN | EPOLLET; // Edge-triggered mode
                    event.data.fd = acceptFD;
                    if (epoll_ctl(epollFD, EPOLL_CTL_ADD, acceptFD, &event) == -1) {
                        Error(2, "Error Server:: ", "epoll_ctl (add acceptFD)");
                        close(acceptFD);
                    }
                } else {
                    // Data ready to read
                    char buffer[1024];
                    memset(buffer, 0, sizeof(buffer));
                    int bytesReceived = recv(events[i].data.fd, buffer, sizeof(buffer) - 1, 0);
                    if (bytesReceived <= 0) {
                        // Handle client disconnect or error
                        epoll_ctl(epollFD, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                        close(events[i].data.fd);
                        continue;
                    }

                    std::cout << "Received: " << buffer << std::endl;

                    // Send response
                    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
                    if (send(events[i].data.fd, response.c_str(), response.size(), 0) == -1) {
                        Error(2, "Error Server:: ", "send");
                    }

                    close(events[i].data.fd); // Close after handling request
                }
            }
        }
    }

    close(epollFD); // Close epoll instance
}
