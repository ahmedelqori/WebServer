/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aes-sarg <aes-sarg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/13 20:43:44 by aes-sarg          #+#    #+#             */
/*   Updated: 2025/02/25 22:26:57 by aes-sarg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/RequestHandler.hpp"
#include "../includes/Cgi.hpp"
#include <csignal>

RequestHandler::RequestHandler() : requestStates()
{
}

void RequestHandler::cleanupConnection(int epoll_fd, int fd)
{
    if (requestStates.find(fd) != requestStates.end())
        requestStates.erase(fd);
    if (responses_info.find(fd) != responses_info.end())
        responses_info.erase(fd);
    if (chunked_uploads.find(fd) != chunked_uploads.end())
        chunked_uploads.erase(fd);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
}

void RequestHandler::handleWriteEvent(int epoll_fd, int current_fd)
{
    if (responses_info.find(current_fd) == responses_info.end())
    {
        cleanupConnection(epoll_fd, current_fd);
        return;
    }
    if (responses_info[current_fd].isCgi == true)
    {
        int status;
        pid_t ret = waitpid(responses_info[current_fd].cgiPid, &status, WNOHANG);
        time_t now = time(NULL);

        if (now - responses_info[current_fd].cgiStartTime >= CGI_TIMEOUT)
        {

            kill(responses_info[current_fd].cgiPid, SIGKILL);
            waitpid(responses_info[current_fd].cgiPid, &status, 0);
            responses_info[current_fd].isCgi = false;
            if (responses_info[current_fd].headers.empty())
            {
                if (hasErrorPage(CGI_TIMEOUT1, current_fd))
                    responses_info[current_fd] = ServerUtils::serveFile(getErrorPage(CGI_TIMEOUT1, current_fd), CGI_TIMEOUT1);
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
            string output = cgiInstance.getResponse(responses_info[current_fd].cgiOutputFile);
            ResponseInfos parsedResponse = cgiInstance.parseOutput(output);
            responses_info[current_fd] = parsedResponse;
            responses_info[current_fd].isCgi = false;
            if (responses_info[current_fd].headers.empty())
            {
                if (hasErrorPage(BAD_GATEWAY, current_fd))
                    responses_info[current_fd] = ServerUtils::serveFile(getErrorPage(BAD_GATEWAY, current_fd), BAD_GATEWAY);
                else
                    responses_info[current_fd] = ServerUtils::ressourceToResponse(ServerUtils::generateErrorPage(BAD_GATEWAY), BAD_GATEWAY);
            }
        }
    }

    if (!responses_info[current_fd].headers.empty())
    {

        Response responseHeaders;
        responseHeaders.setStatus(responses_info[current_fd].status, responses_info[current_fd].statusMessage);
        for (map<string, string>::const_iterator it = responses_info[current_fd].headers.begin(); it != responses_info[current_fd].headers.end(); ++it)
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
        responses_info[current_fd].headers.clear();
        responses_info[current_fd].bytes_written = 0;
        return;
    }

    if (!responses_info[current_fd].body.empty())
    {

        ssize_t bytes_sent = send(current_fd, responses_info[current_fd].body.c_str(), responses_info[current_fd].body.length(), 0);
        if (bytes_sent <= 0)
        {

            cleanupConnection(epoll_fd, current_fd);
            return;
        }
        responses_info[current_fd].body = responses_info[current_fd].body.substr(bytes_sent);
        if (responses_info[current_fd].body.empty())
        {

            cleanupConnection(epoll_fd, current_fd);
        }
    }

    if (responses_info[current_fd].filePath != "")
    {
        ifstream fileStream(responses_info[current_fd].filePath.c_str(), ios::in | ios::binary);
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
    return requestStates.find(client_sockfd) == requestStates.end(); 
}

void RequestHandler::handleRequest(int client_sockfd, string req, int bytes_received, int epoll_fd, vector<ServerConfig> config)
{
 

    try
    {
        
        if (isNewClient(client_sockfd))
        {
            cout << "new user: " << client_sockfd << endl;
            requestStates[client_sockfd].servers_config = config;
            requestStates[client_sockfd].partial_request += req;
            requestStates[client_sockfd].total_size += bytes_received;
            cout << "validating CRLF: " << requestStates[client_sockfd].validCRLF << endl;
            if (!requestStates[client_sockfd].validCRLF)
            {
                if (requestStates[client_sockfd].partial_request.find(CRLF_CRLF) == string::npos)
                {
                    cout << "More data needed for CRLF" << endl;
                    return;
                }
                else
                {
                    requestStates[client_sockfd].validCRLF = true;
                }
            }

            requestStates[client_sockfd].request = requestStates[client_sockfd].parser.parse(requestStates[client_sockfd].partial_request, requestStates[client_sockfd].total_size);
            requestStates[client_sockfd].request.client_sockfd = client_sockfd;
            if (isChunkedRequest(requestStates[client_sockfd].request))
            {
                if (requestStates[client_sockfd].request.hasHeader(CONTENT_LENGTH))
                    throw BAD_REQUEST;
                string url = requestStates[client_sockfd].request.getDecodedPath();
                if (!getFinalUrl(url, client_sockfd))
                    throw NOT_FOUND;

                LocationConfig location;
                if (this->matchLocation(location, requestStates[client_sockfd].request.getDecodedPath(), requestStates[client_sockfd].request))
                {

                    if (!ServerUtils::isMethodAllowed(requestStates[client_sockfd].request.getMethod(), location.getMethods()))
                        throw NOT_ALLOWED;
                    if (location.getUploadDir().empty())
                        throw UNAUTHORIZED;

                    ChunkedUploadState state;
                    state.headers_parsed = true;
                    state.content_remaining = 0;

                    state.upload_path = location.getRoot() + requestStates[client_sockfd].request.getDecodedPath() + location.getUploadDir() + "/" + ServerUtils::generateUniqueString() +
                                        ServerUtils::getFileExtention(requestStates[client_sockfd].request.getHeader(CONTENT_TYPE));

                    state.output_file.open(state.upload_path.c_str(), std::ios::binary);

                    if (!state.output_file.is_open())
                        throw NOT_FOUND;
                    chunked_uploads[client_sockfd] = state;

                    processChunkedData(client_sockfd, requestStates[client_sockfd].request.getBody(), epoll_fd);
                }
            }
            else if (isPostMethod(requestStates[client_sockfd].request))
            {
                // Handle POST method, differentiating it from GET
                handlePostRequest(client_sockfd, epoll_fd);
            }

            else
            {
                // Handle other methods
                responses_info[client_sockfd] = processRequest(requestStates[client_sockfd].request);
                modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
            }
        }
        else
        {
            if (chunked_uploads.find(client_sockfd) == chunked_uploads.end())
            {
                requestStates[client_sockfd].partial_request += req;
                requestStates[client_sockfd].total_size += bytes_received;
                cout << "validating CRLF: " << requestStates[client_sockfd].validCRLF << endl;
                if (!requestStates[client_sockfd].validCRLF)
                {
                    if (requestStates[client_sockfd].partial_request.find(CRLF_CRLF) == string::npos)
                    {
                        cout << "More data needed for CRLF 2" << endl;
                        return;
                    }
                    else
                    {
                        requestStates[client_sockfd].validCRLF = true;
                    }
                }

                requestStates[client_sockfd].request = requestStates[client_sockfd].parser.parse(requestStates[client_sockfd].partial_request, requestStates[client_sockfd].total_size);
                requestStates[client_sockfd].request.client_sockfd = client_sockfd;
                if (isChunkedRequest(requestStates[client_sockfd].request))
                {
                    if (requestStates[client_sockfd].request.hasHeader(CONTENT_LENGTH))
                        throw BAD_REQUEST;
                    string url = requestStates[client_sockfd].request.getDecodedPath();
                    if (!getFinalUrl(url, client_sockfd))
                        throw NOT_FOUND;

                    LocationConfig location;
                    if (this->matchLocation(location, requestStates[client_sockfd].request.getDecodedPath(), requestStates[client_sockfd].request))
                    {

                        if (!ServerUtils::isMethodAllowed(requestStates[client_sockfd].request.getMethod(), location.getMethods()))
                            throw NOT_ALLOWED;
                        if (location.getUploadDir().empty())
                            throw UNAUTHORIZED;

                        ChunkedUploadState state;
                        state.headers_parsed = true;
                        state.content_remaining = 0;

                        state.upload_path = location.getRoot() + requestStates[client_sockfd].request.getDecodedPath() + location.getUploadDir() + "/" + ServerUtils::generateUniqueString() +
                                            ServerUtils::getFileExtention(requestStates[client_sockfd].request.getHeader(CONTENT_TYPE));

                        state.output_file.open(state.upload_path.c_str(), std::ios::binary);

                        if (!state.output_file.is_open())
                            throw NOT_FOUND;
                        chunked_uploads[client_sockfd] = state;

                        processChunkedData(client_sockfd, requestStates[client_sockfd].request.getBody(), epoll_fd);
                    }
                }
                else if (isPostMethod(requestStates[client_sockfd].request))
                {
                    // Handle POST method, differentiating it from GET
                    handlePostRequest(client_sockfd, epoll_fd);
                }

                else
                {

                    responses_info[client_sockfd] = processRequest(requestStates[client_sockfd].request);
                    modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
                }
            }

            else if (isChunkedRequest(requestStates[client_sockfd].request))
            {
                processChunkedData(client_sockfd, req, epoll_fd);
            }
            else if (isPostMethod(requestStates[client_sockfd].request))
            {

                processPostData(client_sockfd, req, epoll_fd);
            }

            else
            {

                responses_info[client_sockfd] = processRequest(requestStates[client_sockfd].request);
                modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
            }
        }
    }
    catch (int code)
    {

        handleError(client_sockfd, epoll_fd, code);
    }
    catch (exception &e)
    {

        responses_info[client_sockfd] = ServerUtils::ressourceToResponse(
            Request::generateErrorPage(INTERNAL_SERVER_ERROR),
            INTERNAL_SERVER_ERROR);
        modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
    }
}

void RequestHandler::handlePostRequest(int client_sockfd, int epoll_fd)
{
    LocationConfig location;
    string url = requestStates[client_sockfd].request.getDecodedPath();

    if (!getFinalUrl(url, client_sockfd))
        throw NOT_FOUND;

    if (this->matchLocation(location, requestStates[client_sockfd].request.getDecodedPath(), requestStates[client_sockfd].request))
    {
        if (!ServerUtils::isMethodAllowed(requestStates[client_sockfd].request.getMethod(), location.getMethods()))
            throw NOT_ALLOWED;

        if (is_CgiRequest(url, location.getCgiExtension()))
        {
            CGI cgi;
            ResponseInfos response = cgi.execute(requestStates[client_sockfd].request, url, location.getCgiExtension(), location.getRoot());
            responses_info[client_sockfd] = response;
            modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
            return;
        }

        if (location.getUploadDir().empty())
        {
            cout << "Location: " << location.getPath() << endl;
            cout << "Upload dir: " << location.getUploadDir() << endl;
            throw UNAUTHORIZED;
        }

        ChunkedUploadState state;
        state.headers_parsed = true;
        state.content_remaining = 0;
        state.total_size = 0;

        state.upload_path = location.getRoot() + requestStates[client_sockfd].request.getDecodedPath() + location.getUploadDir() + "/" +
                            ServerUtils::generateUniqueString() + ServerUtils::getFileExtention(requestStates[client_sockfd].request.getHeader(CONTENT_TYPE));

        state.output_file.open(state.upload_path.c_str(), std::ios::binary);
        if (!state.output_file.is_open())
            throw NOT_FOUND;

        chunked_uploads[client_sockfd] = state;
        processPostData(client_sockfd, requestStates[client_sockfd].request.getBody(), epoll_fd);
    }
}

void RequestHandler::handleGetRequest(int client_sockfd, int epoll_fd)
{
    LocationConfig location;
    string url = requestStates[client_sockfd].request.getDecodedPath();

    if (!getFinalUrl(url, client_sockfd))
        throw NOT_FOUND;

    if (this->matchLocation(location, requestStates[client_sockfd].request.getDecodedPath(), requestStates[client_sockfd].request))
    {
        if (!ServerUtils::isMethodAllowed(requestStates[client_sockfd].request.getMethod(), location.getMethods()))
            throw NOT_ALLOWED;

        responses_info[client_sockfd] = processRequest(requestStates[client_sockfd].request);
        modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
    }
}

void RequestHandler::handleError(int client_sockfd, int epoll_fd, int code)
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

    if (hasErrorPage(code, client_sockfd))
    {
        responses_info[client_sockfd] = ServerUtils::serveFile(getErrorPage(code, client_sockfd), code);
        modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
    }
    else
    {
        responses_info[client_sockfd] = ServerUtils::ressourceToResponse(
            ServerUtils::generateErrorPage(code), code);
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

bool RequestHandler::getFinalUrl(string &url, int fd)
{

    LocationConfig loc;
    if (this->matchLocation(loc, url, requestStates[fd].request))
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
            requestStates[fd].request.setDecodedPath(loc.getRedirectionPath());
            url = requestStates[fd].request.getDecodedPath();
            getFinalUrl(url, fd);
        }
        lastLocations.clear();
        return true;
    }
    if (lastLocations.size() > 0)
        lastLocations.clear();
    return false;
}
string RequestHandler::getErrorPage(int code, int client_sockfd)
{
    map<string, string> errors_pages = getServer(requestStates[client_sockfd].servers_config , requestStates[client_sockfd].request.getHeader(HOST)).getErrorPages();
    return errors_pages[itoa(code)];
}
bool RequestHandler::hasErrorPage(int code, int client_sockfd)
{
    map<string, string> errors_pages = getServer(requestStates[client_sockfd].servers_config , requestStates[client_sockfd].request.getHeader(HOST)).getErrorPages();
    string errorPagePath = errors_pages.find(itoa(code)) != errors_pages.end() ? errors_pages[itoa(code)] : ServerUtils::generateErrorPage(code);
    if (access(errorPagePath.c_str(), F_OK | R_OK) == 0)
        return true;
    return false;
}

ResponseInfos RequestHandler::processRequest(const Request &request)
{

    if (request.getMethod() == GET)
        return handleGet(request.client_sockfd);
    else if (request.getMethod() == DELETE)
        return handleDelete(request.client_sockfd);
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
            if (indexPath[0] == '.' || indexPath[0] == '/')
                throw NOT_FOUND;
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

ResponseInfos RequestHandler::handleGet(int client_sockfd)
{

    string url = requestStates[client_sockfd].request.getDecodedPath();
    LocationConfig bestMatch;
    RessourceInfo ressource;

    if (!matchLocation(bestMatch, url, requestStates[client_sockfd].request))
    {
        string f_path = bestMatch.getRoot() + url;
        ressource.autoindex = bestMatch.getDirectoryListing();
        ressource.redirect = "";
        ressource.path = f_path;
        ressource.cgi_infos = bestMatch.getCgiExtension();
        ressource.errors_pages = getServer(requestStates[client_sockfd].servers_config , requestStates[client_sockfd].request.getHeader(HOST)).getErrorPages();
        ressource.root = bestMatch.getRoot();
        ressource.url = url;

        if (is_CgiRequest(url, bestMatch.getCgiExtension()))
        {
            try
            {
                CGI cgi;
                ResponseInfos response;
                response = cgi.execute(requestStates[client_sockfd].request, url, bestMatch.getCgiExtension(), bestMatch.getRoot());

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

        return serveRessourceOrFail(ressource, client_sockfd);
    }

    string fullPath = bestMatch.getRoot() + url;

    ressource.autoindex = bestMatch.getDirectoryListing();
    ressource.indexFile = bestMatch.getIndexFile();
    ressource.redirect = bestMatch.getRedirectionPath();
    ressource.path = fullPath;
    ressource.cgi_infos = bestMatch.getCgiExtension();
    ressource.errors_pages = getServer(requestStates[client_sockfd].servers_config , requestStates[client_sockfd].request.getHeader(HOST)).getErrorPages();
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
            if (ressource.indexFile[0] == '.' || ressource.indexFile[0] == '/')
                throw NOT_FOUND;
            url = url + ressource.indexFile;
        }
    }

    if (is_CgiRequest(url, bestMatch.getCgiExtension()))
    {
        try
        {
            CGI cgi;
            ResponseInfos response;
            response = cgi.execute(requestStates[client_sockfd].request, url, bestMatch.getCgiExtension(), bestMatch.getRoot());
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

    if (!ServerUtils::isMethodAllowed(requestStates[client_sockfd].request.getMethod(), bestMatch.getMethods()))
        return ServerUtils::ressourceToResponse(Request::generateErrorPage(NOT_ALLOWED), NOT_ALLOWED);
    return serveRessourceOrFail(ressource, client_sockfd);
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

ResponseInfos RequestHandler::handleDelete(int client_sockfd)
{

    LocationConfig bestMatch;
    string url = requestStates[client_sockfd].request.getDecodedPath();
    if (!getFinalUrl(url, requestStates[client_sockfd].request.client_sockfd))
        throw NOT_FOUND;

    if (matchLocation(bestMatch, requestStates[client_sockfd].request.getDecodedPath(), requestStates[client_sockfd].request))
    {
        if (!ServerUtils::isMethodAllowed(requestStates[client_sockfd].request.getMethod(), bestMatch.getMethods()))
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
        return deleteOrFail(bestMatch.getRoot() + requestStates[client_sockfd].request.getDecodedPath());
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

    (void)request;
    vector<LocationConfig> locs = getServer(requestStates[request.client_sockfd].servers_config , request.getHeader(HOST)).getLocations();
    LocationConfig bestMatch;
    size_t bestMatchLength = 0;
    bool found = false;

    for (size_t i = 0; i < locs.size(); i++)
    {
        const string &path = locs[i].getPath();

        if (url.find(path) != string::npos)
        {
            size_t pathLength = path.length();
            if (pathLength > bestMatchLength)
            {
                char nextChar = url[pathLength];
                if (nextChar == '/' || path == "/" || nextChar == '\0')
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

ResponseInfos RequestHandler::serveRessourceOrFail(RessourceInfo ressource, int client_sockfd)
{
    map<string, string> errorPagePaths = getServer(requestStates[client_sockfd].servers_config , requestStates[client_sockfd].request.getHeader(HOST)).getErrorPages();
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

void RequestHandler::checkMaxBodySize(int client_sockfd)
{
    size_t maxBodySize = getServer(requestStates[client_sockfd].servers_config , requestStates[client_sockfd].request.getHeader(HOST)).getClientMaxBodySize();
    string contentLenghtStr = requestStates[client_sockfd].request.getHeader(CONTENT_LENGTH).empty() ? "0" : requestStates[client_sockfd].request.getHeader(CONTENT_LENGTH);

    stringstream ss(contentLenghtStr);
    size_t contentLenght;
    ss >> contentLenght;
    if (contentLenght > maxBodySize)
        throw PAYLOAD_TOO_LARGE;
}

void RequestHandler::processChunkedData(int client_sockfd, const string &data, int epoll_fd)
{

    checkMaxBodySize(client_sockfd);
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

    checkMaxBodySize(client_sockfd);

    ChunkedUploadState &state = chunked_uploads[client_sockfd];
    state.partial_request += data;
    string contentLenghtStr;
    try
    {
        contentLenghtStr = requestStates[client_sockfd].request.getHeader(CONTENT_LENGTH).empty() ? "0" : requestStates[client_sockfd].request.getHeader(CONTENT_LENGTH);
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
