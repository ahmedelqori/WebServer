/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aes-sarg <aes-sarg@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/28 14:05:48 by ael-qori          #+#    #+#             */
/*   Updated: 2025/01/14 00:48:40 by aes-sarg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "includes/Server.hpp"


int main(int ac, char **av)
{
    Server    Server;

    try {
        if (ac != 2) Error(2, ERR_INPUT, ERR_ARGS);
        Server.configFile.parseFile(av[1]);
        Server.start();
    } catch (const std::exception& e) {
        std::cerr << std::endl <<e.what() << std::endl << std::endl;
        return 1; 
    }
}