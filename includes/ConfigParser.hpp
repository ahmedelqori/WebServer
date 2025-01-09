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

# ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <map> 
#include <string> 
#include <iostream>
#include <stdexcept>
#include <bits/stdc++.h>
#include "UtlisParser.hpp"

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

class LocationConfig
{
    private:
        int                                     redirectionCode;
        bool                                    directoryListing;
        std::string                             path;
        std::string                             root;
        std::string                             redirectionPath;
        std::string                             indexFile;
        std::vector <std::string>               methods;
    public:
        int                                     getRedirectionCode() const;
        bool                                    getDirectoryListing() const;
        std::string                             getPath() const; 
        std::string                             getRoot() const;
        std::string                             getIndexFile() const;
        std::string                             getRedirectionPath() const;
        std::vector<std::string>                getMethods() const;

        void                                    setDirectoryListing(bool b);
        void                                    setPath(std::string path); 
        void                                    setRoot(std::string root);
        void                                    setMethods(std::string key);
        void                                    setRedirectionCode(int statusCode);
        void                                    setRedirectionPath(std::string path);
        void                                    setIndexFile(std::string index);

};

class ServerConfig
{
    private:
        int                                     port;
        std::string                             clientMaxBodySize;
        std::string                             host;
        std::vector<std::string>                serverNames;
        std::vector<LocationConfig>             locations;
        std::map<std::string, std::string>      errorPages;
    public:
        ServerConfig();

        int                                     locationIndex;
        int                                     getPort() const;
        std::string                             getClientMaxBodySize() const;
        std::string                             getHost() const;
        std::vector<std::string>                getServerNames() const;
        std::vector<LocationConfig>             &getLocations();
        std::map<std::string, std::string>      getErrorPages() const;

        void                                    setPort(int port);
        void                                    setHost(std::string &host);
        void                                    setClientMaxBodySize(std::string &size);
        void                                    setServerNames(std::string &server_name);
        void                                    setLocations(LocationConfig location);
        void                                    setErrorPages(std::string &key, std::string &value);
    };

class ConfigParser
{
    private:
        
    public:
        std::vector <ServerConfig> servers;
        std::vector<std::string>                fileContent;
        int                                     current;
        int                                     index;
        
        ServerState                             currentServerState;
        ErrorPagesState                         currentErrorPages;
        LocationsState                          currentLocationState;
        RedirectionsState                       currentRedirectState;
        
        ConfigParser();         
        
        void                                    parseFile(const char *file_path);
        void                                    fileToVector(std::ifstream &file);
        void                                    checkClosedParenthesis();
        void                                    deleteEmptyLines();
        
        void                                    parse();
        void                                    parseErrorPages();
        void                                    parseLocations();
        
        void                                    handleHttpState();
        void                                    handleServerState();
        void                                    handleHostPortState();
        void                                    handleServerNameState();
        void                                    handleErrorPagesState();
        void                                    handleErrorFileState();
        void                                    handleClientMaxBodySizeState();
        void                                    handlePathLocationState();
        void                                    handleMethodsState();
        void                                    handleRootState();
        void                                    handleRedirectionState();
        void                                    handleDirectoryListingState();
        void                                    handleIndexFileState();
        
        void                                    printHttp();
};

#endif