/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/23 21:52:11 by ael-qori          #+#    #+#             */
/*   Updated: 2025/01/23 23:47:05 by ael-qori         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# ifndef LOGGER_HPP
#define LOGGER_HPP

# include <string>
# include <ctime>
# include <iostream>

#define RESET           "\033[0m"
#define BLACK           "\033[30m"      
#define RED             "\033[31m"      
#define GREEN           "\033[32m"      
#define YELLOW          "\033[33m"      
#define BLUE            "\033[34m"      
#define MAGENTA         "\033[35m"
#define CYAN            "\033[36m"      
#define WHITE           "\033[37m"      
#define BOLDBLACK       "\033[1m\033[30m" 
#define BOLDRED         "\033[1m\033[31m" 
#define BOLDGREEN       "\033[1m\033[32m" 
#define BOLDYELLOW      "\033[1m\033[33m" 
#define BOLDBLUE        "\033[1m\033[34m" 
#define BOLDMAGENTA     "\033[1m\033[35m" 
#define BOLDCYAN        "\033[1m\033[36m" 
#define BOLDWHITE       "\033[1m\033[37m" 

# define SUB_COLOR      "\033[33m"
# define INFO_COLOR     BOLDCYAN   
# define ERROR_COLOR    "\033[31m"   
# define WARNING_COLOR  "\033[33m"  


# define LOG_INIT       "Server is initializing."
# define LOG_ADDR       "Server is setting up the address."
# define LOG_SOCKETS    "Server is creating sockets."
# define LOG_BIND       "Server is binding sockets."
# define LOG_LISTEN     "Server is listening for connections."
# define LOG_INIT_EPOLL "Server is initializing epoll."
# define LOG_EPOLL      "Server is waiting for events with epoll."


class Logger {
    public:
        bool sub;
        enum Level { INFO, WARNING, ERROR, SUB };
        Logger();
        ~Logger(){};

        std::string getTimestamp();
        std::string levelToString(Level level);
        std::string levelToColor(Level level);
        void deleteLine();
        void logSubInfo(const std::string message);
        void overwriteLine(const std::string message);
        void log(Level level, const std::string message);

};

#endif