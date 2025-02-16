/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/05 11:28:02 by ael-qori          #+#    #+#             */
/*   Updated: 2025/02/16 16:22:44 by ael-qori         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "../includes/ConfigParser.hpp"
#include "../includes/Request.hpp"
#include "../includes/Response.hpp"
#include "../includes/ServerUtils.hpp"
#include "../includes/RequestHandler.hpp"
#include "../includes/Logger.hpp"
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

        bool isTimedOut() const
        {
            return difftime(lastActivityTime, acceptTime) >= 4;
        }

        time_t acceptTime;
        time_t lastActivityTime;
};

enum StateServer
{
    INIT,
    ADDR,
    SOCKETS,
    BIND,
    LISTEN,
    INIT_EPOLL,
    EPOLL,
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
        std::vector<std::pair<int, ConnectionStatus> > ClientStatus;
        addrinfo hints;
        Logger logger;
        int serverIndex;
        
        StateServer currentStateServer; 

        Server();
        ~Server();
    
        void    start();
        void    init();
        void    ServerLogger(std::string message,Logger::Level level, bool is_sub);
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
        void    addClientToEpoll(int clientFD);
        void    CheckForTimeOut(int);
};
#endif