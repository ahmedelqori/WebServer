/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   UtlisParser.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aes-sarg <aes-sarg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/28 13:43:42 by ael-qori          #+#    #+#             */
/*   Updated: 2025/02/26 17:52:48 by aes-sarg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_PARSER_HPP
#define UTILS_PARSER_HPP

#include <map> 
#include <string> 
#include <iostream>
#include <stdexcept>
#include <bits/stdc++.h>
#include "./Logger.hpp"

long                            parse_size(const std::string& str);
std::string                     itoa(size_t n);
std::string                     trim(const std::string& str);
std::vector<std::string>        splitString(const std::string& input, const std::string& delimiters);

void                            *GlobalEpollFd();
void                            *GlobalCondition();
bool                            isValidCharInPath(char c);
void                            handleSignalInterrupt(int sig);
void                            handleSignalInterrupt(int sig);
bool                            is_number(std::string str, int index);
bool                            is_hostname(std::string str, int index);
bool                            is_ipaddress(std::string str, int index);
bool                            is_valid_size(std::string str, int index);
bool                            is_statuscode(std::string str, int index);
bool                            is_valid_server_name(std::string str, int index);
bool                            is_duplicated(std::vector<std::string> vec, std::string v, int index);
void                            Error(int count, ...);


//  Macros

#define ERR_SYNTAX                  "Syntax Error:: "
#define ERR_SERVER                  "Server Error:: "
#define ERR_INPUT                   "InputError:: "
#define ERR_CONF                    "Configfile Error:: "
#define ERR_ARGS                    "Incorrect number of arguments. Expected 1 argument."
#define ERR_FILE_NOT_EXIST          "File doesnt exist"
#define ERR_MIS_PAR                 "Missing closing parenthesis Or parenthesis not in single line"
#define ERR_OPEN_PAR                "Open parenthesis doesnt exist for "
#define ERR_CLOSE_PAR               "Unkown Param or Closed parenthesis doesnt exist for "
#define ERR_DUPLICATED              "Duplicated"
#define ERR_INVALID_STATUS_CODE     "Invalid Status Code"
#define ERR_UNKNOWN_METHOD          "Unknown Method"
#define ERR_GETADDRINFO             "in getaddre info"
#define ERR_SOCKETS                 "in Sockets"
#define ERR_SETSOCKOPT              "setsockopt"
#define ERR_REUSEADDR               "SO_REUSEADDR"
#define ERR_REUSEPORT               "SO_REUSEPORT"
#define ERR_BIND                    "in Bind"
#define ERR_LISTEN                  "in Listen"
#define ERR_EPOLL_CREATE            "in epoll_create"
#define ERR_CANNOT_REGISTER_CLIENT  "Cannot Add Register Client"
#define ERR_CANNOT_ADD_CLIENT       "Cannot Add Client To Epoll"
#define ERR_CANNOT_ACCEPT           "Cannot Accept Connection"
#define ERR_EPOLL_INTERRUPTED       "epoll_wait interrupted by signal. Retrying..."
#define ERR_EPOLL_CRITICAL          "Critical Error in Server:: "
#define CLIENT_CONNECTED            "Client connected"    
#define CLIENT_DISCONNECTED         "Client disconnected"    

#define LINE                        "Line "
#define W_HTTP                      "http"
#define W_ERROR                     "error"
#define W_SERVER                    "server"
#define W_HOST                      "host"
#define W_PORT                      "port"
#define W_EMPTY                     "Empty File"
#define W_HOST_PORT                 "host_port"
#define W_SERVER_NAMES              "server_names"
#define W_ERRORS_PAGES              "errors_pages"
#define W_CLIENT_MAX_BODY_SIZE      "client_max_body_size"
#define W_LOCATION                  "location"
#define W_METHODS                   "methods"
#define W_ROOT                      "root"
#define W_REDIRECTION               "redirect"
#define W_AUTO_INDEX                "auto_index"
#define W_INDEX                     "index"
#define W_UPLOAD                    "upload"
#define W_CGI                       "cgi_extension"
#define GET                         "GET"
#define POST                        "POST"
#define DELETE                      "DELETE"
#define W_EPOLL_WAIT                "Epoll Wait"

#define TIMEOUT_MESSAGE "HTTP/1.1 408 Request Timeout\r\n\r\n"

#define ON                          "ON"
#define OFF                         "OFF"

#define O_PAR                       "{"
#define C_PAR                       "}"

#define SLASH                       '/'
#define UNDER_SCORE                 '_'
#define HYPHENS                     '-'

#define SLASH_STR                   "/"

#define INDEX                       -1

#define WHITE_SPACES                " \t\n\r\f\v"

#define RESET                       "\033[0m"
#define BLACK                       "\033[30m"
#define RED                         "\033[31m"
#define GREEN                       "\033[32m"
#define YELLOW                      "\033[33m"
#define BLUE                        "\033[34m"
#define MAGENTA                     "\033[35m"
#define CYAN                        "\033[36m"
#define WHITE                       "\033[37m"
#define BOLDBLACK                   "\033[1m\033[30m"
#define BOLDRED                     "\033[1m\033[31m"
#define BOLDGREEN                   "\033[1m\033[32m"
#define BOLDYELLOW                  "\033[1m\033[33m"
#define BOLDBLUE                    "\033[1m\033[34m"
#define BOLDMAGENTA                 "\033[1m\033[35m"
#define BOLDCYAN                    "\033[1m\033[36m"
#define BOLDWHITE                   "\033[1m\033[37m"

#define BYTE                        "B"
#define KILO                        "K"
#define MEGA                        "M"
#define GIGA                        "G"

#define TIMEOUT                     10
#define TIMEOUT_EPOLL               5000
#define MAX_EVENTS                  1024
#define BUFFER_SIZE_SERVER          8192
#define SLEEP_LOGGER                500000
#endif
