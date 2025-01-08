/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/06 14:44:46 by ael-qori          #+#    #+#             */
/*   Updated: 2025/01/07 17:49:53 by ael-qori         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

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


void Server::acceptAndAnswer()
{
    int index = INDEX;
    int acceptFD = INDEX;
    char buffer[1024];

    while (++index < this->res.size())
    {
        acceptFD = accept(this->socketContainer[index], this->res[index]->ai_addr, &this->res[index]->ai_addrlen);
        if (acceptFD == -1)
        {
            Error(2, "Error Server:: ", "accept");
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(acceptFD, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived == -1)
        {
            Error(2, "Error Server:: ", "recv");
            close(acceptFD);
            continue;
        }
        buffer[bytesReceived] = '\0'; 
        std::cout << "Received: " << buffer << std::endl;

        std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
        if (send(acceptFD, response.c_str(), response.size(), 0) == -1)
        {
            Error(2, "Error Server:: ", "send");
        }

        close(acceptFD);
    }
}

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
    this->acceptAndAnswer();
}

void    Server::start()
{
    this->init();
    while (++serverIndex < this->configFile.servers.size())
    {

        freeaddrinfo(this->res[serverIndex]);
    }
}
