/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/06 14:44:46 by ael-qori          #+#    #+#             */
/*   Updated: 2025/01/06 14:46:56 by ael-qori         ###   ########.fr       */
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

void    Server::init()
{
    memset(&this->hints, 0, sizeof(this->hints));
    this->hints.ai_family = AF_INET;
    this->hints.ai_socktype = SOCK_STREAM;
    this->hints.ai_flags = AI_PASSIVE;
    this->createLinkedListOfAddr();
}

void    Server::start()
{
    this->init();
    while (++serverIndex < this->configFile.servers.size())
    {

        freeaddrinfo(this->res[serverIndex]);
    }
}
