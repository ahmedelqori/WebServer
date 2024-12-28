/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/28 14:05:48 by ael-qori          #+#    #+#             */
/*   Updated: 2024/12/28 14:05:57 by ael-qori         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"


int main(int ac, char **av)
{
    ConfigParser    configParser;
    
    try {
        if (ac != 2) Error(2, ERR_INPUT, ERR_ARGS);
        configParser.parseFile(av[1]);
        configParser.printHttp();
    } catch (const std::exception& e) {
        std::cerr << std::endl <<e.what() << std::endl << std::endl;
        return 1; 
    }   
}
