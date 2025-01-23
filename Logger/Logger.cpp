/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/23 21:51:34 by ael-qori          #+#    #+#             */
/*   Updated: 2025/01/23 23:46:17 by ael-qori         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Logger.hpp"


Logger::Logger():sub(false){}

void Logger::log(Level level, const std::string message) {
    if (sub) deleteLine();
    sub = false;
    std::string logEntry = getTimestamp() + " [" + levelToString(level) + "] " + message;
    std::cout << levelToColor(level) << logEntry << RESET << std::endl;
}

std::string Logger::getTimestamp() {
    char            buf[20];
    std::time_t     now;
    
    now = std::time(0);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buf);
}

void Logger::logSubInfo(const std::string message) {
    std::cout << "\t" << SUB_COLOR << message << RESET << std::endl;
}

void Logger::overwriteLine(const std::string message) {
    if(!sub)std::cout << "\r" << std::string(150, ' ');
    sub = true;
    std::cout << "\r" << "\t" << BOLDMAGENTA << message << RESET << std::string(30,' ') << std::flush;
}

void Logger::deleteLine()
{
    std::cout << "\r" << std::string(100, ' ') << "\r";    
}

std::string Logger::levelToString(Level level) {
    switch (level) {
        case INFO:      return  "INFO";
        case WARNING:   return  "WARNING";
        case ERROR:     return  "ERROR";
        default:        return  "UNKNOWN";
    }
}

std::string Logger::levelToColor(Level level) {
    switch (level) {
        case INFO:      return  INFO_COLOR;
        case WARNING:   return  WARNING_COLOR;
        case ERROR:     return  ERROR_COLOR;
        default:        return  RESET;
    }
}
