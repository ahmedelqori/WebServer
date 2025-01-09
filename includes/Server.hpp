/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/05 11:28:02 by ael-qori          #+#    #+#             */
/*   Updated: 2025/01/06 14:43:58 by ael-qori         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "../includes/ConfigParser.hpp"

#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

class  Server
{
    public:
        ConfigParser configFile;
        std::vector<addrinfo *> res; 
        addrinfo hints;
        
    

        int serverIndex;
        
        Server();

        void    start();
        void    init();
        void    createLinkedListOfAddr();
};




#endif