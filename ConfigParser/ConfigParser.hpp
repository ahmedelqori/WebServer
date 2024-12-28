 /* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/27 10:02:23 by ael-qori          #+#    #+#             */
/*   Updated: 2024/12/27 16:00:41 by ael-qori         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP



#include <map> 
#include <string> 
#include <iostream>
#include <stdexcept>
#include <bits/stdc++.h>

#define O_PAR "{"
#define C_PAR "}"
#define INDEX -1
#define WHITE_SPACES " \t\n\r\f\v"

#define RESET "\033[0m"
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define BOLDBLACK "\033[1m\033[30m"
#define BOLDRED "\033[1m\033[31m"
#define BOLDGREEN "\033[1m\033[32m"
#define BOLDYELLOW "\033[1m\033[33m"
#define BOLDBLUE "\033[1m\033[34m"
#define BOLDMAGENTA "\033[1m\033[35m"
#define BOLDCYAN "\033[1m\033[36m"
#define BOLDWHITE "\033[1m\033[37m"

class LocationConfig
{
    private:
        bool                                directoryListing;
        std::string                         path;
        std::string                         root;
        std::vector <std::string>           methods;
        std::string                         redirectionPath;
        std::string                         indexFile;
        int                                 redirectionCode;
    public:
        bool                                getDirectoryListing() const;
        std::string                         getPath() const; 
        std::string                         getRoot() const;
        std::vector<std::string>             getMethods() const;
        std::string                         getRedirectionPath() const;
        int                                 getRedirectionCode() const;
        std::string                         getIndexFile() const;
    
        void                                setDirectoryListing(bool b);
        void                                setPath(std::string path); 
        void                                setRoot(std::string root);
        void                                setMethods(std::string key);
        void                                setRedirectionCode(int statusCode);
        void                                setRedirectionPath(std::string path);
        void                                setIndexFile(std::string index);

};

class ServerConfig
{
    private:
        int                                 port;
        std::string                         clientMaxBodySize;
        std::string                         host;
        std::vector<std::string>            serverNames;
        std::vector<LocationConfig>         locations;
        std::map<std::string, std::string>  errorPages;
    public:
        ServerConfig();
        int                                 locationIndex;
        int                                 getPort() const;
        std::string                         getClientMaxBodySize() const;
        std::string                         getHost() const;
        std::vector<std::string>            getServerNames() const;
        std::vector<LocationConfig>         &getLocations();
        std::map<std::string, std::string>  getErrorPages() const;

        void                                setPort(int port);
        void                                setHost(std::string &host);
        void                                setClientMaxBodySize(std::string &size);
        void                                setServerNames(std::string &server_name);
        void                                setLocations(LocationConfig location);
        void                                setErrorPages(std::string &key, std::string &value);
    };

class ConfigParser
{
    private:
        std::vector <ServerConfig> servers;
        enum  ErrorPagesState
        {
            ERROR,
            ERROR_FILE,
            DONE_ERROR_PAGES
        };

        enum  RedirectionsState
        {
            STATUS_CODE,
            REDIRECTION_FILE  
        };

        enum  LocationsState
        {
            PATH,
            METHODS,
            ROOT,
            REDIRECTION,
            AUTO_INDEX,
            INDEX_FILE,
            PATH_INFO,
            END_LOCATION
        };

        enum  ServerState
        {
            HTTP,
            SERVER,
            HOST_PORT,
            SERVER_NAME,
            ERROR_PAGES,
            ALLOW_METHODS,
            CLIENT_MAX_BODY_SIZE,
            LOCATIONS,
            DONE
        };
    public:
        std::vector<std::string> fileContent;
        int                      current;
        int                      index;

        ServerState         currentServerState;
        ErrorPagesState     currentErrorPages;
        LocationsState      currentLocationState;
        RedirectionsState   currentRedirectState;
        
        ConfigParser();
        
        void    parseFile(const char *file_path);
        void    fileToVector(std::ifstream &file);
        void    checkClosedParenthesis();
        void    deleteEmptyLines();

        void    parse();
        void    parseErrorPages();
        void    parseLocations();

        void    handleHttpState();
        void    handleServerState();
        void    handleHostPortState();
        void    handleServerNameState();
        void    handleErrorPagesState();
        void    handleErrorFileState();
        void    handleClientMaxBodySizeState();
        void    handlePathLocationState();
        void    handleMethodsState();
        void    handleRootState();
        void    handleRedirectionState();
        void    handleDirectoryListingState();
        void    handleIndexFileState();

        void    printHttp();
};



/// ERRORS

#define ERR_SYNTAX  "Syntax Error:: "
#define ERR_INPUT "InputError:: "
#define ERR_ARGS "Incorrect number of arguments. Expected 1 argument."
#define ERR_FILE_NOT_EXIST "File doesnt exist"
#define ERR_MIS_PAR "Missing closing parenthesis Or parenthesis not in single line"
#define ERR_OPEN_PAR "Open parenthesis doesnt exist for "
#define ERR_CLOSE_PAR "Closed parenthesis doesnt exist for "
#define ERR_DUPLICATED "Duplicated"
#define ERR_INVALID_STATUS_CODE "Invalid Status Code"
#define ERR_UNKNOWN_METHOD "Unknown Method"
#define LINE    "Line "
#define W_HTTP "http"
#define W_ERROR   "error"
#define W_SERVER "server"
#define W_HOST "host"
#define W_PORT "port"
#define W_HOST_PORT "host_port"
#define W_SERVER_NAMES "server_names"
#define W_ERRORS_PAGES "errors_pages"
#define W_CLIENT_MAX_BODY_SIZE "client_max_body_size"
#define W_LOCATION "location"
#define W_METHODS  "methods"
#define W_ROOT  "root"
#define W_REDIRECTION "redirect"
#define W_AUTO_INDEX "auto_index"
#define W_INDEX "index"

#define GET "GET"
#define POST "POST"
#define DELETE "DELETE"

#define ON  "ON"
#define OFF "OFF"


#endif