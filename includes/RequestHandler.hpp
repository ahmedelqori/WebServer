/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aes-sarg <aes-sarg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/13 20:28:36 by aes-sarg          #+#    #+#             */

/*   Updated: 2025/02/23 15:32:58 by aes-sarg         ###   ########.fr       */

/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "MyType.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <string>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "Request.hpp"
#include "ServerUtils.hpp"
#include "HttpParser.hpp"
#include "ConfigParser.hpp"
#include "Logger.hpp"

class ChunkedUploadState
{
public:
    string partial_request;
    bool headers_parsed;
    size_t content_remaining;
    string upload_path;
    size_t total_size;
    bool validCRLF;
    ofstream output_file;
    Request request;
    HttpParser parser;
    vector<ServerConfig> servers_config;

    ChunkedUploadState()
        : headers_parsed(false), content_remaining(0), total_size(0), validCRLF(false), output_file(), request(), parser(), servers_config()
    {
    }

    ~ChunkedUploadState()
    {
        if (output_file.is_open())
        {
            output_file.close();
        }
    }

    ChunkedUploadState(const ChunkedUploadState &other)
        : partial_request(other.partial_request),
          headers_parsed(other.headers_parsed),
          content_remaining(other.content_remaining),
          upload_path(other.upload_path),
          total_size(other.total_size),
          validCRLF(other.validCRLF),
            output_file()

    {
        if (other.output_file.is_open())
        {
            output_file.open(upload_path.c_str(), ios::binary | ios::app);
        }
    }

    ChunkedUploadState &operator=(const ChunkedUploadState &other)
    {
        if (this != &other)
        {
            partial_request = other.partial_request;
            headers_parsed = other.headers_parsed;
            content_remaining = other.content_remaining;
            upload_path = other.upload_path;
            total_size = other.total_size;
            validCRLF = other.validCRLF;

            if (output_file.is_open())
                output_file.close();
            if (other.output_file.is_open())
                output_file.open(upload_path.c_str(), ios::binary | ios::app);
        }
        return *this;
    }

    ofstream &getOutputFileStream()
    {
        if (!output_file.is_open())
            output_file.open(upload_path.c_str(), ios::binary | ios::app);
        return output_file;
    }
};

class RequestHandler
{
private:
    map<int, ChunkedUploadState> chunked_uploads;
    map<int, ResponseInfos> responses_info;
    vector<string> lastLocations;

public:
    RequestHandler();
    ~RequestHandler();
   
    map<int, ChunkedUploadState> requestStates;

    bool getFinalUrl(string &url, int);
    bool alreadyExist(string url);
    ResponseInfos serverRootOrRedirect(RessourceInfo ressource);
    ServerConfig getServer(vector<ServerConfig>, std::string host);
    bool hasErrorPage(int code, int);
    string getErrorPage(int code, int);
    void handleError(int client_sockfd, int epoll_fd, int code);
    void sendTimeOutResponse(int,vector<ServerConfig> configs);
    void handlePostRequest(int client_sockfd, int epoll_fd);
  
    void handleRequest(int client_sockfd, string req, int, int epoll_fd, vector<ServerConfig>);

    bool isNewClient(int client_sockfd);
    ResponseInfos processRequest(const Request &request);
    bool is_CgiRequest(string url, map<string, string> cgiInfos);
    ResponseInfos handleGet(int);
    void checkMaxBodySize(int);
    void cleanupConnection(int epoll_fd, int fd);
    void processPostData(int client_sockfd, const string &data, int epoll_fd);
    void processChunkedData(int client_sockfd, const string &data, int epoll_fd);
    ResponseInfos handleDelete(int);
    void modifyEpollEvent(int epoll_fd, int fd, uint32_t events);
    bool handleWriteEvent(int epoll_fd, int current_fd);
    ResponseInfos serveRessourceOrFail(RessourceInfo ressource, int client_sockfd);
    bool matchLocation(LocationConfig &loc, const string url, const Request &request);
};

ostream &operator<<(ostream &os, const Request &request);
