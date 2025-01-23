/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbentahi <mbentahi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/28 14:05:48 by ael-qori          #+#    #+#             */
/*   Updated: 2025/01/23 16:49:23 by mbentahi         ###   ########.fr       */
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