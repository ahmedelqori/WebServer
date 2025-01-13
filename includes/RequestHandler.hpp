/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aes-sarg <aes-sarg@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/13 20:28:36 by aes-sarg          #+#    #+#             */
/*   Updated: 2025/01/14 00:38:21 by aes-sarg         ###   ########.fr       */
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

class RequestHandler
{
private:
    struct ChunkedUploadState
    {
        string partial_request;
        bool headers_parsed;
        size_t content_remaining;
        string upload_path;
        ofstream output_file;

        ChunkedUploadState &operator=(const ChunkedUploadState &other)
        {
            if (this != &other)
            {
                this->partial_request = other.partial_request;
                this->headers_parsed = other.headers_parsed;
                this->content_remaining = other.content_remaining;
                this->upload_path = other.upload_path;
                if (this->output_file.is_open())
                    this->output_file.close();
                if (other.output_file.is_open())
                    this->output_file.open(other.upload_path, ios::binary | ios::app);
            }
            return *this;
        }
    };
    map<int, ChunkedUploadState> chunked_uploads;
    map<int, ResponseInfos> responses_info;

public: 
    // Req
    // Request request;

    // Note
    ConfigParser configFile;

    RequestHandler(const ConfigParser configParser);
    void handleRequest(int client_sockfd, string req, int epoll_fd);
    ResponseInfos processRequest(const Request &request);
    ResponseInfos handleGet(const Request &request);
    ResponseInfos handlePost(const Request &request);
    void cleanupConnection(int epoll_fd, int fd);
    void processChunkedData(int client_sockfd, const string &data, int epoll_fd);
    ResponseInfos handleDelete(const Request &request);
    void modifyEpollEvent(int epoll_fd, int fd, uint32_t events);
    void handleWriteEvent(int epoll_fd, int current_fd);
    ResponseInfos serveRessourceOrFail(RessourceInfo ressource);
    bool matchLocation(LocationConfig &loc, const string url);
};

ostream &operator<<(ostream &os, const Request &request);
