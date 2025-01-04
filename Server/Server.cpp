/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/29 14:07:48 by ael-qori          #+#    #+#             */
/*   Updated: 2024/12/29 15:12:04 by ael-qori         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

int main() {
    char ip4[INET_ADDRSTRLEN]; // space to hold the IPv4 string
    struct sockaddr_in sa; // pretend this is loaded with something
    
    inet_ntop(AF_INET, &(sa.sin_addr), ip4, INET_ADDRSTRLEN);
    
    printf("The IPv4 address is: %s\n", ip4);
}