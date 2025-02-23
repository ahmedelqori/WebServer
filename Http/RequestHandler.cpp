/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aes-sarg <aes-sarg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/13 20:43:44 by aes-sarg          #+#    #+#             */
/*   Updated: 2025/02/23 20:38:02 by aes-sarg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/RequestHandler.hpp"
#include "../includes/Cgi.hpp"
#include <csignal>

RequestHandler::RequestHandler()
{
}

void RequestHandler::cleanupConnection(int epoll_fd, int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    request.clearRequest();
    validCRLF = false;
    reqBuffer.clear();
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
    ResponseInfos &response_info = responses_info[current_fd];
    if (responses_info[current_fd].isCgi == true)
    {

        int status;
        pid_t ret = waitpid(response_info.cgiPid, &status, WNOHANG);
        time_t now = time(NULL);

        if (now - response_info.cgiStartTime >= CGI_TIMEOUT)
        {

            kill(response_info.cgiPid, SIGKILL);
            waitpid(response_info.cgiPid, &status, 0);
            response_info.isCgi = false;
            if (response_info.headers.empty())
            {
                if (hasErrorPage(CGI_TIMEOUT1))
                    responses_info[current_fd] = ServerUtils::serveFile(getErrorPage(CGI_TIMEOUT1), CGI_TIMEOUT1);
                else
                    responses_info[current_fd] = ServerUtils::ressourceToResponse(ServerUtils::generateErrorPage(CGI_TIMEOUT1), CGI_TIMEOUT1);
            }
        }

        if (ret == 0)
        {

            return;
        }

        else if (ret > 0)
        {

            CGI cgiInstance;
            string output = cgiInstance.getResponse(response_info.cgiOutputFile);
            ResponseInfos parsedResponse = cgiInstance.parseOutput(output);
            response_info = parsedResponse;
            response_info.isCgi = false;
            if (response_info.headers.empty())
            {
                if (hasErrorPage(BAD_GATEWAY))
                    responses_info[current_fd] = ServerUtils::serveFile(getErrorPage(BAD_GATEWAY), BAD_GATEWAY);
                else
                    responses_info[current_fd] = ServerUtils::ressourceToResponse(ServerUtils::generateErrorPage(BAD_GATEWAY), BAD_GATEWAY);
            }
        }
    }

    if (!response_info.headers.empty())
    {

        Response responseHeaders;
        responseHeaders.setStatus(response_info.status, response_info.statusMessage);
        for (map<string, string>::const_iterator it = response_info.headers.begin(); it != response_info.headers.end(); ++it)
        {
            responseHeaders.addHeader(it->first, it->second);
        }

        string responseHeadersStr = responseHeaders.getResponse();

        ssize_t bytes_sent = send(current_fd, responseHeadersStr.c_str(), responseHeadersStr.length(), 0);
        if (bytes_sent <= 0)
        {

            cleanupConnection(epoll_fd, current_fd);
            return;
        }
        response_info.headers.clear();
        responses_info[current_fd].bytes_written = 0;
        return;
    }

    if (!response_info.body.empty())
    {

        ssize_t bytes_sent = send(current_fd, response_info.body.c_str(), response_info.body.length(), 0);
        if (bytes_sent <= 0)
        {

            cleanupConnection(epoll_fd, current_fd);
            return;
        }
        response_info.body = response_info.body.substr(bytes_sent);
        if (response_info.body.empty())
        {

            cleanupConnection(epoll_fd, current_fd);
        }
    }

    if (response_info.filePath != "")
    {
        ifstream fileStream(response_info.filePath.c_str(), ios::in | ios::binary);
        if (!fileStream.is_open())
        {
            cleanupConnection(epoll_fd, current_fd);
            return;
        }

        fileStream.seekg(responses_info[current_fd].bytes_written, ios::beg);
        char buffer[READ_BUFFER_SIZE];
        fileStream.read(buffer, READ_BUFFER_SIZE);
        ssize_t bytes_read = fileStream.gcount();
        if (bytes_read <= 0)
        {

            fileStream.close();
            cleanupConnection(epoll_fd, current_fd);
            return;
        }
        ssize_t bytes_sent = send(current_fd, buffer, bytes_read, 0);
        if (bytes_sent == -1)
        {

            fileStream.close();
            cleanupConnection(epoll_fd, current_fd);
            return;
        }
        responses_info[current_fd].bytes_written += bytes_sent;
    }
    else
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
            request.hasHeader(TRANSFER_ENCODING) &&
            request.hasHeader(CONTENT_TYPE) &&
            request.getHeader(TRANSFER_ENCODING) == CHUNKED);
}

static bool isPostMethod(Request request)
{
    return (request.getMethod() == POST);
}

bool RequestHandler::isNewClient(int client_sockfd)
{
    return chunked_uploads.find(client_sockfd) == chunked_uploads.end();
}
void RequestHandler::handleRequest(int client_sockfd, string req, int bytes_received, int epoll_fd, vector<ServerConfig> config)
{
    server_config = config;
    try
    {
        if (isNewClient(client_sockfd))
        {

            reqBuffer += req;
            if (!validCRLF)
            {
                if (reqBuffer.find(CRLF_CRLF) == string::npos)
                    return;
                else
                    validCRLF = true;
            }

            HttpParser parser;

            request = parser.parse(reqBuffer, bytes_received);

            reqBuffer.clear();
            if (isChunkedRequest(request))
            {
                if (request.hasHeader(CONTENT_LENGTH))
                    throw BAD_REQUEST;
                string url = request.getDecodedPath();
                if (!getFinalUrl(url))
                {
                    throw NOT_FOUND;
                }

                LocationConfig location;
                if (this->matchLocation(location, request.getDecodedPath(), request))
                {

                    ChunkedUploadState state;
                    state.headers_parsed = true;
                    state.content_remaining = 0;

                    state.upload_path = location.getRoot() + request.getDecodedPath() + ServerUtils::generateUniqueString() +
                                        ServerUtils::getFileExtention(request.getHeader(CONTENT_TYPE));
                    state.output_file.open(state.upload_path.c_str(), std::ios::binary);

                    if (!state.output_file.is_open())
                        throw NOT_FOUND;
                    chunked_uploads[client_sockfd] = state;
                    if (!ServerUtils::isMethodAllowed(request.getMethod(), location.getMethods()))
                        throw NOT_ALLOWED;
                    processChunkedData(client_sockfd, request.getBody(), epoll_fd);
                }
            }
            else if (isPostMethod(request))
            {
                LocationConfig location;

                string url = request.getDecodedPath();

                if (!getFinalUrl(url))
                {
                    throw NOT_FOUND;
                }

                if (this->matchLocation(location, request.getDecodedPath(), request))
                {

                    if (is_CgiRequest(url, location.getCgiExtension()))
                    {
                        CGI cgi;

                        ResponseInfos response;
                        response = cgi.execute(request, url, location.getCgiExtension(), location.getRoot());

                        responses_info[client_sockfd] = response;

                        modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
                        return;
                    }
                    ChunkedUploadState state;
                    state.headers_parsed = true;
                    state.content_remaining = 0;
                    state.total_size = 0;

                    state.upload_path = location.getRoot() + request.getDecodedPath() + "/" + ServerUtils::generateUniqueString() +
                                        ServerUtils::getFileExtention(request.getHeader(CONTENT_TYPE));
                    state.output_file.open(state.upload_path.c_str(), std::ios::binary);

                    if (!state.output_file.is_open())
                        throw NOT_FOUND;
                    chunked_uploads[client_sockfd] = state;
                    if (!ServerUtils::isMethodAllowed(request.getMethod(), location.getMethods()))
                        throw NOT_ALLOWED;
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
            {
                if (request.hasHeader(CONTENT_LENGTH))
                    throw BAD_REQUEST;
                processChunkedData(client_sockfd, req, epoll_fd);
            }
            else
                processPostData(client_sockfd, req, epoll_fd);
        }
    }
    catch (int code)
    {
        map<int, ChunkedUploadState>::iterator it = chunked_uploads.find(client_sockfd);
        if (it != chunked_uploads.end())
        {
            if (it->second.output_file.is_open())
            {
                it->second.output_file.close();
            }
            remove(it->second.upload_path.c_str());
            chunked_uploads.erase(it);
        }

        if (hasErrorPage(code))
        {
            responses_info[client_sockfd] = ServerUtils::serveFile(getErrorPage(code), code);
            modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
        }
        else
        {

            responses_info[client_sockfd] = ServerUtils::ressourceToResponse(
                ServerUtils::generateErrorPage(code), code);
            modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
        }
    }
    catch (exception &e)
    {
        responses_info[client_sockfd] = ServerUtils::ressourceToResponse(
            Request::generateErrorPage(INTERNAL_SERVER_ERROR),
            INTERNAL_SERVER_ERROR);
        modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
    }
}

bool RequestHandler::alreadyExist(string url)
{
    size_t i = 0;

    while (i < lastLocations.size())
    {
        if (lastLocations[i] == url)
            return true;
        i++;
    }
    return false;
}

bool RequestHandler::getFinalUrl(string &url)
{

    LocationConfig loc;
    if (this->matchLocation(loc, url, request))
    {
        if (lastLocations.size() > 0)
        {
            if (alreadyExist(url))
            {
                lastLocations.clear();
                return false;
            }
        }
        lastLocations.push_back(url);
        if (!loc.getRedirectionPath().empty())
        {
            request.setDecodedPath(loc.getRedirectionPath());
            url = request.getDecodedPath();
            getFinalUrl(url);
        }
        lastLocations.clear();
        return true;
    }
    if (lastLocations.size() > 0)
        lastLocations.clear();
    return false;
}
string RequestHandler::getErrorPage(int code)
{
    map<string, string> errors_pages = getServer(server_config, request.getHeader(HOST)).getErrorPages();
    return errors_pages[itoa(code)];
}
bool RequestHandler::hasErrorPage(int code)
{
    map<string, string> errors_pages = getServer(server_config, request.getHeader(HOST)).getErrorPages();
    string errorPagePath = errors_pages.find(itoa(code)) != errors_pages.end() ? errors_pages[itoa(code)] : ServerUtils::generateErrorPage(code);
    if (access(errorPagePath.c_str(), F_OK | R_OK) == 0)
        return true;
    return false;
}

ResponseInfos RequestHandler::processRequest(const Request &request)
{

    if (request.getMethod() == GET)
        return handleGet(request);
    else if (request.getMethod() == DELETE)
        return handleDelete(request);
    else
        return ServerUtils::ressourceToResponse(ServerUtils::generateErrorPage(NOT_EXIST), NOT_EXIST);
}

bool RequestHandler::is_CgiRequest(string url, map<string, string> cgiInfos)
{

    map<string, string>::const_iterator it = cgiInfos.begin();
    if (it == cgiInfos.end())
        return false;
    size_t pos = url.find_last_of(".");
    if (pos == string::npos)
        return false;
    string extention = url.substr(pos, url.length());
    map<string, string>::const_iterator pos2 = cgiInfos.find(extention);
    if (pos2 == cgiInfos.end())
        return false;
    return true;
}

ResponseInfos RequestHandler::serverRootOrRedirect(RessourceInfo ressource)
{
    if ((ressource.url[ressource.url.length() - 1] != '/' && ressource.url != "/") || !ressource.redirect.empty())
    {
        string redirectUrl = (!ressource.redirect.empty() ? ressource.redirect + "/" : ressource.url + "/");
        return ServerUtils::handleRedirect(redirectUrl, REDIRECTED);
    }
    if (!ressource.indexFile.empty())
    {

        string indexPath;
        if (ressource.autoindex)
            indexPath = ressource.root + "/" + ressource.indexFile;
        else
            indexPath = ressource.root + "/" + ressource.url + '/' + ressource.indexFile;

        struct stat indexStat;
        if (stat(indexPath.c_str(), &indexStat) == 0)
        {
            return ServerUtils::serveFile(indexPath, OK);
        }
    }
    if (ressource.autoindex)
        return ServerUtils::generateDirectoryListing(ressource.root + ressource.url);
    string errPAth = ressource.errors_pages.find(NOT_FOUND_CODE) != ressource.errors_pages.end() ? ressource.errors_pages[NOT_FOUND_CODE] : ServerUtils::generateErrorPage(NOT_FOUND);
    if (access(errPAth.c_str(), F_OK | R_OK) == 0)
    {
        return ServerUtils::serveFile(errPAth, NOT_FOUND);
    }
    return ServerUtils::ressourceToResponse(ServerUtils::generateErrorPage(NOT_FOUND), NOT_FOUND);
}

ResponseInfos RequestHandler::handleGet(const Request &request)
{

    string url = request.getDecodedPath();
    LocationConfig bestMatch;
    RessourceInfo ressource;

    if (!matchLocation(bestMatch, url, request))
    {
        string f_path = bestMatch.getRoot() + url;
        ressource.autoindex = bestMatch.getDirectoryListing();
        ressource.redirect = "";
        ressource.path = f_path;
        ressource.cgi_infos = bestMatch.getCgiExtension();
        ressource.errors_pages = getServer(server_config, request.getHeader(HOST)).getErrorPages();
        ressource.root = bestMatch.getRoot();
        ressource.url = url;

        if (is_CgiRequest(url, bestMatch.getCgiExtension()))
        {
            try
            {
                CGI cgi;
                ResponseInfos response;
                response = cgi.execute(request, url, bestMatch.getCgiExtension(), bestMatch.getRoot());

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
        }

        return serveRessourceOrFail(ressource);
    }

    string fullPath = bestMatch.getRoot() + url;

    ressource.autoindex = bestMatch.getDirectoryListing();
    ressource.indexFile = bestMatch.getIndexFile();
    ressource.redirect = bestMatch.getRedirectionPath();
    ressource.path = fullPath;
    ressource.cgi_infos = bestMatch.getCgiExtension();
    ressource.errors_pages = getServer(server_config, request.getHeader(HOST)).getErrorPages();
    ressource.root = bestMatch.getRoot();
    ressource.url = url;

    if (!ressource.indexFile.empty() && url.find_last_of("/") == url.length() - 1)
    {

        string indexPath;
        if (ressource.autoindex)
            indexPath = ressource.root + "/" + ressource.indexFile;
        else
            indexPath = ressource.root + "/" + ressource.url + '/' + ressource.indexFile;

        struct stat indexStat;
        if (stat(indexPath.c_str(), &indexStat) == 0)
        {
            url = url + ressource.indexFile;
        }
    }

    if (is_CgiRequest(url, bestMatch.getCgiExtension()))
    {
        try
        {
            CGI cgi;
            ResponseInfos response;
            response = cgi.execute(request, url, bestMatch.getCgiExtension(), bestMatch.getRoot());
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
    }

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
    string url = request.getDecodedPath();
    if (!getFinalUrl(url))
        throw NOT_FOUND;

    if (matchLocation(bestMatch, request.getDecodedPath(), request))
    {
        if (!ServerUtils::isMethodAllowed(request.getMethod(), bestMatch.getMethods()))
            return ServerUtils::ressourceToResponse(
                ServerUtils::generateErrorPage(NOT_ALLOWED),
                NOT_ALLOWED);
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

ServerConfig RequestHandler::getServer(vector<ServerConfig> servers, std::string host)
{
    size_t i = 0;

    while (i < servers.size())
    {
        size_t j = 0;
        while (j < servers[i].getServerNames().size())
        {
            if (!host.empty() && servers[i].getServerNames()[j] == host)
                return servers[i];
            j++;
        }
        i++;
    }

    return servers[0];
}

bool RequestHandler::matchLocation(LocationConfig &loc, const string url, const Request &request)
{

    cout << "IS FIRST TIME" << endl;
    (void)request;
    vector<LocationConfig> locs = getServer(server_config, request.getHeader(HOST)).getLocations();
    LocationConfig bestMatch;
    size_t bestMatchLength = 0;
    bool found = false;

    for (size_t i = 0; i < locs.size(); i++)
    {
        const string &path = locs[i].getPath();

        if (url.find(path) == 0)
        {
            size_t pathLength = path.length();
            if (pathLength > bestMatchLength)
            {
                char nextChar = url[pathLength];
               
                if (nextChar == '/' || nextChar == '\0')
                {
                    found = true;
                    bestMatch = locs[i];
                    bestMatchLength = pathLength;
                }
            }
        }
    }
    loc = bestMatch;
    return found;
}

ResponseInfos RequestHandler::serveRessourceOrFail(RessourceInfo ressource)
{
    map<string, string> errorPagePaths = getServer(server_config, request.getHeader(HOST)).getErrorPages();
    string errorPagePath = errorPagePaths.find(NOT_FOUND_CODE) != errorPagePaths.end() ? errorPagePaths[NOT_FOUND_CODE] : ServerUtils::generateErrorPage(NOT_FOUND);

    switch (ServerUtils::checkResource(ressource.path))
    {
    case DIRECTORY:
        return serverRootOrRedirect(ressource);
        break;
    case REGULAR:
        return ServerUtils::serveFile(ressource.path, OK);
        break;
    default:
        return ServerUtils::serveFile(errorPagePath, NOT_FOUND);
        break;
    }
}

void RequestHandler::checkMaxBodySize()
{
    size_t maxBodySize = getServer(server_config, request.getHeader(HOST)).getClientMaxBodySize();
    string contentLenghtStr = request.getHeader(CONTENT_LENGTH).empty() ? "0" : request.getHeader(CONTENT_LENGTH);

    stringstream ss(contentLenghtStr);
    size_t contentLenght;
    ss >> contentLenght;
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

        size_t pos = state.partial_request.find("\r\n");
        if (pos == string::npos)
        {
            return;
        }

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

        size_t chunk_header_size = pos + 2;
        size_t chunk_total_size = chunk_header_size + chunk_size + 2;

        if (state.partial_request.length() < chunk_total_size)
            return;

        if (chunk_size == 0)
        {

            if (state.partial_request.substr(chunk_header_size).compare(0, 2, "\r\n") == 0)
            {
                state.output_file.close();
                state.total_size = 0;
                responses_info[client_sockfd] = ServerUtils::ressourceToResponse("", CREATED);
                modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
                return;
            }
            else
                throw BAD_REQUEST;
        }

        const char *chunk_data = state.partial_request.data() + chunk_header_size;
        state.output_file.write(chunk_data, chunk_size);
        state.total_size += chunk_size;
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
        contentLenghtStr = request.getHeader(CONTENT_LENGTH).empty() ? "0" : request.getHeader(CONTENT_LENGTH);
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

            responses_info[client_sockfd] = ServerUtils::ressourceToResponse("", CREATED);
            modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
        }
        state.partial_request.clear();
    }
}
