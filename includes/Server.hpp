/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/05 11:28:02 by ael-qori          #+#    #+#             */
/*   Updated: 2025/02/24 13:54:34 by ael-qori         ###   ########.fr       */
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



class ConnectionStatus
{
    public:
        time_t                                              acceptTime;
        time_t                                              lastActivityTime;

        ConnectionStatus();
        bool                                                isTimedOut() const;
};

enum StateServer
{
    CONF,
    INIT,
    ADDR,
    SOCKETS,
    BIND,
    LISTEN,
    INIT_EPOLL,
    EPOLL
};

template <typename T> bool isDuplicated(T data)
{
    ssize_t index, checker, size;

    index = INDEX;
    size = static_cast<ssize_t>(data.size());
    while ((checker = INDEX) && ++index < size)
        while (++checker < size)
            if (data[index] == data[checker] && index != checker)
                return true;
   return false; 
}

class  Server
{
    private:
        int                                                 nfds;
        int                                                 epollFD;
        struct                                              epoll_event event;
        struct                                              epoll_event events[MAX_EVENTS];

    public:
        int                                                 serverIndex;
        Logger                                              logger;
        addrinfo                                            hints;
        StateServer                                         currentStateServer; 
        ConfigParser                                        configFile;
        RequestHandler                                      requestHandler;
        std::vector<int>                                    socketContainer;
        std::vector<addrinfo *>                             res;
        std::vector<std::pair<int, ConnectionStatus> >      ClientStatus;
        
        std::map<int, int>                                  IndexServer;
        std::map<int, int>                                  IndexPorts;

        Server();
        ~Server();
    
        void                                                init();
        void                                                start();
        void                                                findServer();
        void                                                init_epoll();
        void                                                loopAndWait();
        void                                                bindSockets();
        void                                                resetTime(int);
        void                                                updateTime(int);
        void                                                createSockets();
        void                                                timeoutChecker();
        void                                                processData(int);
        void                                                checkConfigFile();
        void                                                acceptAndAnswer(int);
        void                                                CheckForTimeOut(int);
        void                                                registerAllSockets();
        void                                                addClientToEpoll(int);
        void                                                acceptConnection(int);  
        void                                                listenForConnection();
        void                                                regenarateNewValidPath();
        bool                                                checkValidLocationPath();
        void                                                createLinkedListOfAddr();
        bool                                                isValidPath(std::string);
        void                                                CreateAddrOfEachPort(int);
        void                                                deleteFromTimeContainer(int);
        bool                                                checkForDuplicatedLocations();
        bool                                                CheckForConflictInServerName();
        bool                                                checkForDuplicatePortsInTheSameServer();
        void                                                ServerLogger(std::string,Logger::Level, bool);
        bool                                                hasCommonElement(std::vector<int>&, std::vector<int>);
        std::string                                         simplifyPath(std::string);
        std::vector<ServerConfig>                           findCorrectServers(int);
};

#endif
