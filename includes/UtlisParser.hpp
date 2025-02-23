/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   UtlisParser.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/28 13:43:42 by ael-qori          #+#    #+#             */
/*   Updated: 2025/02/23 21:58:59 by ael-qori         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_PARSER_HPP
#define UTILS_PARSER_HPP

#include <map> 
#include <string> 
#include <iostream>
#include <stdexcept>
#include <bits/stdc++.h>

long                            parse_size(const std::string& str);
std::string                     itoa(size_t n);
std::string                     trim(const std::string& str);
std::vector<std::string>        splitString(const std::string& input, const std::string& delimiters);

bool                            is_number(std::string str, int index);
bool                            is_ipaddress(std::string str, int index);
bool                            is_valid_server_name(std::string str, int index);
bool                            is_hostname(std::string str, int index);
bool                            is_valid_size(std::string str, int index);
bool                            is_statuscode(std::string str, int index);
bool                            is_duplicated(std::vector<std::string> vec, std::string v, int index);



void                            Error(int count, ...);


//  Macros

#define ERR_SYNTAX                  "Syntax Error:: "
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

#define TIMEOUT_MESSAGE "HTTP/1.1 408 Request Timeout\r\n\r\n"

#define ON                          "ON"
#define OFF                         "OFF"

#define O_PAR                       "{"
#define C_PAR                       "}"

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

#define TIMEOUT                     45
#define MAX_EVENTS                  1024
#define BUFFER_SIZE_SERVER          8192
#endif
