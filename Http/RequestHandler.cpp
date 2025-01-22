/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbentahi <mbentahi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/13 20:43:44 by aes-sarg          #+#    #+#             */
/*   Updated: 2025/01/22 15:39:13 by mbentahi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/RequestHandler.hpp"
#include "../includes/Cgi.hpp"

RequestHandler::RequestHandler()
{
}

void RequestHandler::cleanupConnection(int epoll_fd, int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    responses_info.erase(fd);
    chunked_uploads.erase(fd);
}

void RequestHandler::handleWriteEvent(int epoll_fd, int current_fd)
{
    if (responses_info.find(current_fd) == responses_info.end())
    {
        cleanupConnection(epoll_fd, current_fd);
        return;
    }

    Response response;
    ResponseInfos &response_info = responses_info[current_fd];

    response.setStatus(response_info.status, response_info.statusMessage);
    map<string, string>::const_iterator head = response_info.headers.begin();
    while (head != response_info.headers.end())
    {
        response.addHeader(head->first, head->second);
        head++;
    }

    response.setBody(response_info.body);

    string response_str = response.getResponse();
    ssize_t bytes_written = write(current_fd, response_str.c_str(), response_str.length());

    if (bytes_written == -1)
    {
        Error(1, "write Error");
        return;
    }

    if (static_cast<size_t>(bytes_written) < response_str.length())
    {
        // Update the remaining response data
        responses_info[current_fd].body = response_str.substr(bytes_written);
        return;
    }

    // Complete response sent, clean up
    cleanupConnection(epoll_fd, current_fd);
}
void RequestHandler::modifyEpollEvent(int epoll_fd, int fd, uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1)
    {
        Error(1, "epoll_ctl mod");
        throw INTERNAL_SERVER_ERROR;
    }
}

static bool isChunkedRequest(Request request)
{
    return (request.getMethod() == POST &&
            request.hasHeader("transfer-encoding") &&
            request.hasHeader("content-type") &&
            request.getHeader("transfer-encoding") == "chunked");
}

static bool isPostMethod(Request request)
{
    return (request.getMethod() == POST);
}

bool RequestHandler::isNewClient(int client_sockfd)
{
    return chunked_uploads.find(client_sockfd) == chunked_uploads.end();
}
void RequestHandler::handleRequest(int client_sockfd, string req, int epoll_fd)
{

    try
    {
        if (isNewClient(client_sockfd))
        {
            HttpParser parser;
            request = parser.parse(req);

            if (isChunkedRequest(request))
            {
                LocationConfig location;
                if (this->matchLocation(location, request.getDecodedPath(), request))
                {
                    ChunkedUploadState state;
                    state.headers_parsed = true;
                    state.content_remaining = 0;

                    state.upload_path = location.getRoot() + request.getDecodedPath() + ServerUtils::generateUniqueString() +
                                        ServerUtils::getFileExtention(request.getHeader("content-type"));
                    state.output_file.open(state.upload_path.c_str(), std::ios::binary);

                    if (!state.output_file.is_open())
                        throw NOT_FOUND;
                    chunked_uploads[client_sockfd] = state;
                    processChunkedData(client_sockfd, request.getBody(), epoll_fd);
                }
            }
            else if (isPostMethod(request))
            {
                LocationConfig location;

                if (this->matchLocation(location, request.getDecodedPath(), request))
                {
                    ChunkedUploadState state;
                    state.headers_parsed = true;
                    state.content_remaining = 0;
                    state.total_size = 0;

                    state.upload_path = location.getRoot() + request.getDecodedPath() + ServerUtils::generateUniqueString() +
                                        ServerUtils::getFileExtention(request.getHeader("content-type"));
                    state.output_file.open(state.upload_path.c_str(), std::ios::binary);

                    if (!state.output_file.is_open())
                        throw NOT_FOUND;
                    chunked_uploads[client_sockfd] = state;
                    processPostData(client_sockfd, request.getBody(), epoll_fd);
                }
            }
            else
            {
                responses_info[client_sockfd] = processRequest(request);
                modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
            }
        }
        else
        {
            if (isChunkedRequest(request))
                processChunkedData(client_sockfd, req, epoll_fd);
            else
                processPostData(client_sockfd, req, epoll_fd);
        }
    }
    catch (int code)
    {
        cout << "Error code: " << code << endl;
        map<int, ChunkedUploadState>::iterator it = chunked_uploads.find(client_sockfd);
        if (it != chunked_uploads.end())
        {
            if (it->second.output_file.is_open())
            {
                it->second.output_file.close();
            }
            remove(it->second.upload_path.c_str());
            chunked_uploads.erase(it);
            cout << "client data cleared" << endl;
        }

        responses_info[client_sockfd] = ServerUtils::ressourceToResponse(
            Request::generateErrorPage(code),
            code);
        cout << "error response generated" << endl;
        modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
    }
    catch (exception &e)
    {
        cout << "Error: " << e.what() << endl;
        responses_info[client_sockfd] = ServerUtils::ressourceToResponse(
            Request::generateErrorPage(INTERNAL_SERVER_ERROR),
            INTERNAL_SERVER_ERROR);
        modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
    }
}

ResponseInfos RequestHandler::processRequest(const Request &request)
{
    // cout << "Process request opened " << endl;
    if (request.getMethod() == GET)
        return handleGet(request);
    else if (request.getMethod() == DELETE)
        return handleDelete(request);
    else
        return ServerUtils::ressourceToResponse(ServerUtils::generateErrorPage(NOT_EXIST), NOT_EXIST);
    // else if (request.getMethod() == POST)
    //     return handlePost(request);
}

ResponseInfos RequestHandler::handleGet(const Request &request)
{

    string url = request.getDecodedPath();
    LocationConfig bestMatch;
    RessourceInfo ressource;
    if (url.length() >= 4 && url.substr(url.length() - 4) == ".php")
    {
        cout << request << endl;
        try
        {
            CGI cgi;
            ResponseInfos response;
            response = cgi.execute(request, url);
            response = cgi.parseOutput(cgi.getOutputPipe());
            cout << "response: ..........................." << endl;
            cout << "response: " << response.status << endl;
            cout << "response: " << response.statusMessage << endl;
            for (map<string, string>::const_iterator it = response.headers.begin(); it != response.headers.end(); ++it)
            {
                cout << "response header: " << it->first << ": " << it->second << endl;
            }
            cout << "response: " << response.body << endl;
            cout << "get response" << endl;
            return response;
        }
        catch (CGIException &e)
        {
            std::cerr << "CGI: ERROR : " << e.what() << '\n';
        }
        catch (exception &e)
        {
            std::cerr << "CGI: ERROR : " << e.what() << '\n';
        }

        // return ServerUtils::ressourceToResponse(ServerUtils::generateErrorPage(NOT_ALLOWED), NOT_ALLOWED);
    }

    if (!matchLocation(bestMatch, url, request))
    {
        string f_path = "www" + url;
        ressource.autoindex = false;
        ressource.redirect = "";
        ressource.path = f_path;
        ressource.root = "www";
        ressource.url = url;
        return serveRessourceOrFail(ressource);
    }

    string fullPath = bestMatch.getRoot() + url;

    ressource.autoindex = bestMatch.getDirectoryListing();
    ressource.indexFile = bestMatch.getIndexFile();
    ressource.redirect = bestMatch.getRedirectionPath();
    ressource.path = fullPath;
    ressource.root = bestMatch.getRoot();
    ressource.url = url;
    if (!ServerUtils::isMethodAllowed(request.getMethod(), bestMatch.getMethods()))
        return ServerUtils::ressourceToResponse(Request::generateErrorPage(NOT_ALLOWED), NOT_ALLOWED);
    return serveRessourceOrFail(ressource);
}

static ResponseInfos deleteDir(const string path)
{
    DIR *dir = opendir(path.c_str());
    if (!dir)
        return ServerUtils::ressourceToResponse(
            ServerUtils::generateErrorPage(FORBIDEN),
            FORBIDEN);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            string fullPath = path + "/" + entry->d_name;
            struct stat statbuf;
            if (stat(fullPath.c_str(), &statbuf) == -1)
            {
                closedir(dir);
                return ServerUtils::ressourceToResponse(
                    ServerUtils::generateErrorPage(FORBIDEN),
                    FORBIDEN);
            }

            if (S_ISDIR(statbuf.st_mode))
            {
                ResponseInfos resp = deleteDir(fullPath);
                if (resp.status != NO_CONTENT)
                {
                    closedir(dir);
                    return resp;
                }
            }
            else
            {
                if (remove(fullPath.c_str()) != 0)
                {
                    closedir(dir);
                    return ServerUtils::ressourceToResponse(
                        ServerUtils::generateErrorPage(FORBIDEN),
                        FORBIDEN);
                }
            }
        }
    }
    closedir(dir);
    if (rmdir(path.c_str()) == 0)
        return ServerUtils::ressourceToResponse("", NO_CONTENT);
    return ServerUtils::ressourceToResponse(
        ServerUtils::generateErrorPage(FORBIDEN),
        FORBIDEN);
}

static ResponseInfos deleteOrFail(const string path)
{
    switch (ServerUtils::checkResource(path))
    {

    case DIRECTORY:
        return deleteDir(path);
    case REGULAR:
        if (remove(path.c_str()) == 0)
            return ServerUtils::ressourceToResponse("", NO_CONTENT);
        return ServerUtils::ressourceToResponse(
            ServerUtils::generateErrorPage(FORBIDEN),
            FORBIDEN);
        break;
    case NOT_EXIST:
        return ServerUtils::ressourceToResponse(
            ServerUtils::generateErrorPage(NOT_FOUND),
            NOT_FOUND);
        break;
    default:
        return ServerUtils::ressourceToResponse(
            ServerUtils::generateErrorPage(INTERNAL_SERVER_ERROR),
            INTERNAL_SERVER_ERROR);
        break;
    }
    return ServerUtils::ressourceToResponse(
        ServerUtils::generateErrorPage(NOT_FOUND),
        NOT_FOUND);
}

ResponseInfos RequestHandler::handleDelete(const Request &request)
{

    LocationConfig bestMatch;

    if (matchLocation(bestMatch, request.getDecodedPath(), request))
    {
        if (bestMatch.getPath() == "/")
            return ServerUtils::ressourceToResponse(
                ServerUtils::generateErrorPage(FORBIDEN),
                FORBIDEN);
        if (!ServerUtils::isMethodAllowed(DELETE, bestMatch.getMethods()))
            return ServerUtils::ressourceToResponse(
                ServerUtils::generateErrorPage(NOT_ALLOWED),
                NOT_ALLOWED);
        return deleteOrFail(bestMatch.getRoot() + request.getDecodedPath());
    }
    return ServerUtils::ressourceToResponse(
        ServerUtils::generateErrorPage(FORBIDEN),
        FORBIDEN);
}

static ServerConfig getServer(ConfigParser configParser, std::string host)
{

    std::vector<ServerConfig> currentServers = configParser.servers;
    int i = INDEX;
    while (++i < currentServers.size())
    {
        stringstream server(currentServers[i].getHost(), ios_base::app | ios_base::out);
        server << ':';
        server << currentServers[i].getPort();
        if (!host.empty() && server.str() == host)
            return currentServers[i];
    }

    return currentServers[0];
}

bool RequestHandler::matchLocation(LocationConfig &loc, const string url, const Request &request)
{

    vector<LocationConfig> locs = getServer(server_config, request.getHeader("host")).getLocations();
    LocationConfig bestMatch;
    size_t bestMatchLength = 0;
    bool found = false;

    for (int i = 0; i < locs.size(); i++)
    {
        if (url.find(locs[i].getPath()) == 0)
        {

            if (locs[i].getPath().length() > bestMatchLength)
            {
                found = true;
                bestMatch = locs[i];

                bestMatchLength = locs[i].getPath().length();
            }
        }
    }
    loc = bestMatch;
    return found;
}

ResponseInfos RequestHandler::serveRessourceOrFail(RessourceInfo ressource)
{

    switch (ServerUtils::checkResource(ressource.path))
    {
    case DIRECTORY:
        return ServerUtils::serverRootOrRedirect(ressource);
        break;
    case REGULAR:
        return ServerUtils::serveFile(ressource.path, OK);
        break;
    case NOT_EXIST:
        return ServerUtils::serveFile("www/404.html", NOT_FOUND);
        break;
    case UNDEFINED:
        return ServerUtils::serveFile("www/404.html", NOT_FOUND);
        break;
    default:
        return ServerUtils::serveFile("www/404.html", NOT_FOUND);
        break;
    }
}

void RequestHandler::checkMaxBodySize()
{
    size_t maxBodySize = getServer(server_config, request.getHeader("host")).getClientMaxBodySize();
    string contentLenghtStr = request.getHeader("content-length").empty() ? "0" : request.getHeader("content-length");

    stringstream ss(contentLenghtStr);
    size_t contentLenght;
    ss >> contentLenght;

    // cout << "max body size " << maxBodySize << " content length " << contentLenght << endl;
    if (contentLenght > maxBodySize)
        throw PAYLOAD_TOO_LARGE;
}

void RequestHandler::processChunkedData(int client_sockfd, const string &data, int epoll_fd)
{

    checkMaxBodySize();
    ChunkedUploadState &state = chunked_uploads[client_sockfd];
    state.partial_request += data;

    while (true)
    {
        // 1. Check for chunk size with proper hex validation
        size_t pos = state.partial_request.find("\r\n");
        if (pos == string::npos)
        {
            return; // Need more data
        }

        // 2. Parse chunk size with strict hex validation
        string chunk_size_str = state.partial_request.substr(0, pos);
        size_t chunk_size = 0;
        try
        {
            std::stringstream ss;
            ss << std::hex << chunk_size_str;
            ss >> chunk_size;
            if (ss.fail())
                throw BAD_REQUEST;
        }
        catch (...)
        {
            throw BAD_REQUEST;
        }

        // 3. Verify we have the complete chunk
        size_t chunk_header_size = pos + 2;                           // includes \r\n
        size_t chunk_total_size = chunk_header_size + chunk_size + 2; // +2 for trailing \r\n

        if (state.partial_request.length() < chunk_total_size)
            return; // Need more data

        // 4. Process chunk
        if (chunk_size == 0)
        {
            // Last chunk
            if (state.partial_request.substr(chunk_header_size).compare(0, 2, "\r\n") == 0)
            {
                // Valid end
                state.output_file.close();
                chunked_uploads.erase(client_sockfd);
                // state.total_size = 0;
                responses_info[client_sockfd] = ServerUtils::ressourceToResponse("", CREATED);
                modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
                return;
            }
        }
        // 5. get content length
        string contentLenghtStr;
        try
        {
            contentLenghtStr = request.getHeader("content-length").empty() ? "0" : request.getHeader("content-length");
        }
        catch (const std::exception &e)
        {
            contentLenghtStr = "0";
        }

        stringstream ss(contentLenghtStr);
        size_t contentLength;
        ss >> contentLength;
        const char *chunk_data = state.partial_request.data() + chunk_header_size;
        if (state.partial_request.length() >= contentLength)
        {
            state.output_file.write(chunk_data, contentLength);
            state.total_size += contentLength;
        }
        else
        {
            state.output_file.write(chunk_data, chunk_size);
            state.total_size += chunk_size;
        }
        // 6.if total size is eq content length should end writing data
        if (state.total_size >= contentLength)
        {
            state.output_file.close();
            state.total_size = 0;
            chunked_uploads.erase(client_sockfd);
            responses_info[client_sockfd] = ServerUtils::ressourceToResponse("", CREATED);
            modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
            return;
        }
        // 7.Remove processed chunk
        state.partial_request = state.partial_request.substr(chunk_total_size);
    }
}

void RequestHandler::processPostData(int client_sockfd, const string &data, int epoll_fd)
{
    checkMaxBodySize();
    ChunkedUploadState &state = chunked_uploads[client_sockfd];
    state.partial_request += data;
    string contentLenghtStr;
    try
    {
        contentLenghtStr = request.getHeader("content-length").empty() ? "0" : request.getHeader("content-length");
    }
    catch (const std::exception &e)
    {
        contentLenghtStr = "0";
    }

    stringstream ss(contentLenghtStr);
    size_t contentLenght;
    ss >> contentLenght;

    if (state.total_size >= contentLenght)
    {
        state.output_file.close();
        chunked_uploads.erase(client_sockfd);
        responses_info[client_sockfd] = ServerUtils::ressourceToResponse("", CREATED);
        modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
    }
    else
    {
        const char *post_data = state.partial_request.data();
        state.output_file.write(post_data, state.partial_request.length());

        state.total_size += state.partial_request.length();

        if (state.total_size >= contentLenght)
        {
            state.output_file.close();
            chunked_uploads.erase(client_sockfd);

            responses_info[client_sockfd] = ServerUtils::ressourceToResponse("", CREATED);
            modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
        }
        state.partial_request.clear();
    }
}

ostream &operator<<(ostream &os, const Request &request)
{
    os << "------------- Method: ---------\n " << request.getMethod() << endl;
    os << "------------- URI: ----------\n"
       << request.getPath() << endl;
    os << "------------- Version: ---------\n " << request.getVersion() << endl;
    os << "------------- Headers: ----------\n"
       << endl;
    map<string, string>::const_iterator it;
    for (it = request.getHeaders().begin(); it != request.getHeaders().end(); ++it)
    {
        os << it->first << ": " << it->second << endl;
    }
    os << "---------------Body:----------------\n"
       << request.getBody() << endl;
    return os;
}
