/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aes-sarg <aes-sarg@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/13 20:43:44 by aes-sarg          #+#    #+#             */
/*   Updated: 2025/01/15 23:42:36 by aes-sarg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/RequestHandler.hpp"

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

    // Set up response
    response.setStatus(response_info.status, response_info.statusMessage);
    map<string,string>::const_iterator head = response_info.headers.begin();
    while (head != response_info.headers.end())
    {
        cout << "header-> " << head->first << " : " << head->second << endl;
        response.addHeader(head->first,head->second);
        head++;
    }
    
    response.setBody(response_info.body);

    string response_str = response.getResponse();
    ssize_t bytes_written = write(current_fd, response_str.c_str(), response_str.length());

    if (bytes_written == -1)
    {

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
        perror("epoll_ctl mod");
        throw INTERNAL_SERVER_ERROR;
    }
}
void RequestHandler::handleRequest(int client_sockfd, string req, int epoll_fd)
{

    try
    {
        if (chunked_uploads.find(client_sockfd) == chunked_uploads.end())
        {
            HttpParser parser;
            request = parser.parse(req);

            if (request.getMethod() == POST &&
                request.hasHeader("transfer-encoding") &&
                request.getHeader("transfer-encoding") == "chunked")
            {

                // Initialize chunked upload state
                ChunkedUploadState state;
                state.headers_parsed = true;
                state.content_remaining = 0;
                state.upload_path = "uploads/" + ServerUtils::generateUniqueString();
                state.output_file.open(state.upload_path.c_str(), std::ios::binary);
                cout << "FILE OPENED " << endl;

                if (!state.output_file.is_open())
                {
                    cerr << "Failed to open output file" << endl;
                    throw INTERNAL_SERVER_ERROR;
                }

                chunked_uploads[client_sockfd] = state;
                cout << "After file opened " << endl;
                processChunkedData(client_sockfd, request.getBody(), epoll_fd);
                // modifyEpollEvent(epoll_fd, client_sockfd, EPOLLIN);
            }
            else
            {
                cout << "Processing non-chunked data" << endl;
                responses_info[client_sockfd] = processRequest(request);
                modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
            }
        }
        else
        {
            cout << "Processing chunked data" << req << endl;
            processChunkedData(client_sockfd, req, epoll_fd);
        }
    }
    catch (int code)
    {
        cout << "Error code: " << code << endl;

        // Clean up any partial uploads
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

        responses_info[client_sockfd] = ServerUtils::ressourceToResponse(
            Request::generateErrorPage(code),
            code);
        modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
    }
    catch (exception &e)
    {
        cerr << "Unexpected error: " << e.what() << endl;

        // Handle unexpected errors similarly to known error codes
        responses_info[client_sockfd] = ServerUtils::ressourceToResponse(
            Request::generateErrorPage(INTERNAL_SERVER_ERROR),
            INTERNAL_SERVER_ERROR);
        modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
    }
}

ResponseInfos RequestHandler::processRequest(const Request &request)
{

    if (request.getMethod() == GET)
    {
        return handleGet(request);
    }

    else if (request.getMethod() == POST)
    {

        return handlePost(request);
    }
    else if (request.getMethod() == DELETE)
    {
        return handleDelete(request);
    }
    else
    {

        return ServerUtils::ressourceToResponse(ServerUtils::generateErrorPage(NOT_EXIST), NOT_EXIST);
    }
}

ResponseInfos RequestHandler::handleGet(const Request &request)
{
    string url = request.getDecodedPath();
    LocationConfig bestMatch;
    RessourceInfo ressource;
     cout << "LOCATION FOUND  000" << bestMatch.getPath() << endl;
    if (!matchLocation(bestMatch, url, request))
    {
        // cout << "Location not found" << endl;
        string f_path = "www" + url;
        // cout << "full path is : " << f_path << endl;
        ressource.autoindex = false;
        ressource.indexFile = bestMatch.getIndexFile();
        ressource.redirect = "";
        ressource.path = f_path;
        ressource.root = "www";
        ressource.url = url;
        return serveRessourceOrFail(ressource);
    }

    cout << "LOCATION FOUND " << bestMatch.getPath() << endl;

    string fullPath = bestMatch.getRoot() + url;
    ressource.autoindex = bestMatch.getDirectoryListing();
    ressource.indexFile = bestMatch.getIndexFile();
    ressource.redirect = bestMatch.getRedirectionPath();
    cout << ressource.redirect << "   hello " << endl; 
    ressource.path = fullPath;
    ressource.root = bestMatch.getRoot();
    ressource.url = url;

    if (ServerUtils::isMethodAllowed(request.getMethod(), bestMatch.getMethods()))
        return serveRessourceOrFail(ressource);
    else
        return ServerUtils::ressourceToResponse(Request::generateErrorPage(NOT_ALLOWED), NOT_ALLOWED);
}

ResponseInfos RequestHandler::handlePost(const Request &request)
{

    return ServerUtils::ressourceToResponse(ServerUtils::generateErrorPage(NOT_FOUND), NOT_FOUND);
}

ResponseInfos RequestHandler::handleDelete(const Request &request)
{
    // map<string, Location> locs = server_config.getLocations();
    // string url = request.getDecodedPath();

    // Location bestMatch;
    // size_t bestMatchLength = 0;
    // bool found = false;

    // for (const auto &loc : locs)
    // {
    //     const string &locationPath = loc.first;
    //     if (url.find(locationPath) == 0 && (url == locationPath || url[locationPath.length()] == '/'))
    //     {
    //         if (locationPath.length() > bestMatchLength)
    //         {
    //             found = true;
    //             bestMatch = loc.second;
    //             bestMatchLength = locationPath.length();
    //         }
    //     }
    // }

    // if (found)
    // {
    //     return ServerUtils::ressourceToResponse("<html><body><h1>DELETE Operation Successful on " + bestMatch.root + "</h1></body></html>", CREATED);
    // }

    return ServerUtils::ressourceToResponse(ServerUtils::generateErrorPage(NOT_FOUND), NOT_FOUND);
}

static ServerConfig getServer(ConfigParser configParser, std::string host)
{

    std::vector<ServerConfig> currentServers = configParser.servers;
    cout << "Hsot : " << host << endl;
    int i = INDEX;
    while (++i < currentServers.size())
    {
        stringstream server(currentServers[i].getHost() ,ios_base::app | ios_base::out); 
        server << ':';
        server << currentServers[i].getPort();
        cout << "server name: " << server.str() << endl;
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
        // Check if URL starts with this location path
        if (url.find(locs[i].getPath()) == 0)
        {

            if (locs[i].getPath().length() > bestMatchLength)
            {
                // std::cout << "Found better match: " << locationPath << std::endl;
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

void RequestHandler::processChunkedData(int client_sockfd, const string &data, int epoll_fd)
{
    ChunkedUploadState &state = chunked_uploads[client_sockfd];
    state.partial_request += data;

    while (true)
    {
        // 1. Check for chunk size with proper hex validation
        size_t pos = state.partial_request.find("\r\n");
        if (pos == string::npos)
        {
            cout << "Need more data" << endl;
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
        {
            return; // Need more data
        }

        // 4. Process chunk
        if (chunk_size == 0)
        {
            // Last chunk
            if (state.partial_request.substr(chunk_header_size).compare(0, 2, "\r\n") == 0)
            {
                // Valid end
                state.output_file.close();
                chunked_uploads.erase(client_sockfd);
                responses_info[client_sockfd] = ServerUtils::ressourceToResponse("Upload Complete", CREATED);
                modifyEpollEvent(epoll_fd, client_sockfd, EPOLLOUT);
                return;
            }
        }

        // Write chunk data
        const char *chunk_data = state.partial_request.data() + chunk_header_size;
        state.output_file.write(chunk_data, chunk_size);

        // Remove processed chunk
        state.partial_request = state.partial_request.substr(chunk_total_size);
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
