/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/05 11:28:02 by ael-qori          #+#    #+#             */
/*   Updated: 2025/01/23 13:37:44 by ael-qori         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "../includes/ConfigParser.hpp"
#include "../includes/Request.hpp"
#include "../includes/Response.hpp"
#include "../includes/ServerUtils.hpp"
#include "../includes/RequestHandler.hpp"

#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <ctime>


# define TIMEOUT 5

class ConnectionStatus
{
    public:
        ConnectionStatus(){
            time_t now = time(NULL);
            this->acceptTime = now;
            this->lastActivityTime = now;
        }
        time_t acceptTime;
        time_t lastActivityTime;
};


class  Server
{
    private:
        int epollFD;
        struct epoll_event event;
        struct epoll_event events[1024];
        int nfds;
    public:
        ConfigParser configFile;
        RequestHandler requestHandler;
        std::vector<addrinfo *> res;
        std::vector<int> socketContainer;
        std::map<int, ConnectionStatus*> ClientStatus;
        addrinfo hints;

        int serverIndex;
        
        Server();
        ~Server();
    
        void    start();
        void    init();
        void    createLinkedListOfAddr();
        void    CreateAddrOfEachPort(int serverIndex);
        void    createSockets();
        void    bindSockets();
        void    listenForConnection();
        void    acceptAndAnswer(int index);
        void    init_epoll();
        void    registerAllSockets();
        void    loopAndWait();
        void    findServer();
        void    processData(int index);
        void    acceptConnection(int index);

        
};




#endif