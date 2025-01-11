#pragma once

#include "../http/Socket.hpp"
#include "../http/Request.hpp"
#include "../http/HttpParser.hpp"
#include "../http/ServerUtils.hpp"
#include "../http/Response.hpp"
#include "../config/Config.hpp"
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/epoll.h>

#include <fcntl.h>
#include <map>
#include <sys/stat.h>
#include <vector>
#include <algorithm>

#define MAX_EVENTS 1024
using namespace std;

class Server
{
public:
    Server(int port, Config &config);
    void start();

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
    Request request;
    Socket server_socket;
    struct epoll_event ev;
    vector<epoll_event> events = vector<epoll_event>(MAX_EVENTS);
    Config server_config;
    map<int, ResponseInfos> responses_info;
    void handleRequest(int client_sockfd, string req, int epoll_fd);
    string readRequest(int client_sockfd);
    ResponseInfos processRequest(const Request &request);
    ResponseInfos handleGet(const Request &request);
    ResponseInfos handlePost(const Request &request);
    void processChunkedData(int client_sockfd, const string& data, int epoll_fd);
    ResponseInfos handleDelete(const Request &request);
    void modifyEpollEvent(int epoll_fd, int fd, uint32_t events);
    void handleEpollEvent(int epoll_fd, const epoll_event& event);
    void handleWriteEvent(int epoll_fd, int current_fd);
    void cleanupConnection(int epoll_fd, int fd);

    ResponseInfos serveRessourceOrFail(RessourceInfo ressource);
    bool matchLocation(Location &loc, const string url);

    string requestToParse;
};

ostream &operator<<(ostream &os, const Request &request);
