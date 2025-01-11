#include "Server.hpp"

using namespace std;

Server::Server(int port, Config &config) : server_socket(port), server_config(config)
{
    // Initialize epoll events vector with a reasonable size
    events.resize(MAX_EVENTS);
}
string Server::readRequest(int client_sockfd)
{
    char buffer[1024] = {0};
    string request = "";
    int valread = read(client_sockfd, buffer, 1024);
    if (valread < 0)
    {
        cerr << "Error reading from socket" << endl;
        return "";
    }
    request.append(buffer, valread);
    return request;
}

void Server::start()
{
    server_socket.bindSocket();
    server_socket.listenSocket();
    cout << "Server is listening on port " << server_socket.getPort() << "..." << endl;

    // Create epoll instance
    int epoll_fd = epoll_create(1); // Use epoll_create instead of epoll_create1 for C++98
    if (epoll_fd == -1)
    {
        perror("epoll_create");
        return;
    }

    // Register server socket with epoll
    struct epoll_event ev;
    ev.events = EPOLLIN; // Remove EPOLLHUP and EPOLLERR as they are implicit
    ev.data.fd = server_socket.getSocketFd();
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket.getSocketFd(), &ev) == -1)
    {
        perror("epoll_ctl");
        close(epoll_fd);
        return;
    }

    while (true)
    {
        int num_events = epoll_wait(epoll_fd, &events[0], events.size(), -1);
        if (num_events == -1)
        {
            // if (errno == EINTR)
            //     continue; // Handle interrupted system call
            cerr << "epoll_wait error" << endl;
            break;
        }

        for (int i = 0; i < num_events; ++i)
        {
            handleEpollEvent(epoll_fd, events[i]);
        }
    }

    close(epoll_fd);
}

void Server::handleEpollEvent(int epoll_fd, const epoll_event &event)
{
    if (event.data.fd == server_socket.getSocketFd())
    {
        // Accept new connection
        int client_fd = server_socket.acceptConnection();
        if (client_fd < 0)
            return;

        // Add to epoll with edge-triggered mode
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0)
        {
            close(client_fd);
            return;
        }
    }
    else
    {
        // Handle client socket
        if (event.events & EPOLLIN)
        {
            // Read all available data
            string request_data;
            char buffer[4096];

            ssize_t bytes_read = read(event.data.fd, buffer, sizeof(buffer));
            if (bytes_read < 0)
            {
                // if (errno == EAGAIN || errno == EWOULDBLOCK)
                // {
                //     break; // No more data
                // }
                cleanupConnection(epoll_fd, event.data.fd);
                return;
            }
            if (bytes_read == 0)
            {
                cleanupConnection(epoll_fd, event.data.fd);
                return;
            }
            request_data.append(buffer, bytes_read);

            if (!request_data.empty())
            {
                handleRequest(event.data.fd, request_data, epoll_fd);
            }
        }

        if (event.events & EPOLLOUT)
        {
            handleWriteEvent(epoll_fd, event.data.fd);
        }
    }
}

void Server::handleWriteEvent(int epoll_fd, int current_fd)
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
    response.addHeader("Content-Type", "text/html");
    if (response_info.status == REDIRECTED)
    {
        response.addHeader("Location", request.getDecodedPath() + "/");
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

void Server::cleanupConnection(int epoll_fd, int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    responses_info.erase(fd);
    chunked_uploads.erase(fd);
}

void Server::processChunkedData(int client_sockfd, const string &data, int epoll_fd)
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
            chunk_size = std::stoul(chunk_size_str, nullptr, 16);
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

// Helper function to modify epoll events
void Server::modifyEpollEvent(int epoll_fd, int fd, uint32_t events)
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
void Server::handleRequest(int client_sockfd, string req, int epoll_fd)
{

    try
    {
        if (chunked_uploads.find(client_sockfd) == chunked_uploads.end())
        {
            cout << "New request coming " << endl;

            HttpParser parser;
            request = parser.parse(req);


            cout << "\n--------------body -----------\n" << request.getBody() << endl;

            if (request.getMethod() == "POST" &&
                request.hasHeader("Transfer-Encoding") &&
                request.getHeader("Transfer-Encoding") == "chunked")
            {

                cout << "Starting chunked upload" << endl;

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

ResponseInfos Server::processRequest(const Request &request)
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

ResponseInfos Server::handleGet(const Request &request)
{
    string url = request.getDecodedPath();
    Location bestMatch;
    RessourceInfo ressource;
    if (!matchLocation(bestMatch, url))
    {
        string f_path = server_config.getRoot() + url;
        ressource.autoindex = false;
        ressource.indexFiles = server_config.getIndexFiles();
        ressource.path = f_path;
        ressource.root = server_config.getRoot();
        ressource.url = url;
        return serveRessourceOrFail(ressource);
    }

    string fullPath = bestMatch.root + url;
    ressource.autoindex = true;
    ressource.indexFiles = bestMatch.index_files;
    ressource.path = fullPath;
    ressource.root = bestMatch.root;
    ressource.url = url;

    if (ServerUtils::isMethodAllowed(request.getMethod(), bestMatch.allow_methods))
        return serveRessourceOrFail(ressource);
    else
        return ServerUtils::ressourceToResponse(Request::generateErrorPage(NOT_ALLOWED), NOT_ALLOWED);
}

ResponseInfos Server::handlePost(const Request &request)
{
    // std::cout << "Handling POST request......\n"
    //   << request.getBody() << std::endl;

    std::map<std::string, Location> locs = server_config.getLocations();
    std::string url = request.getDecodedPath();

    Location bestMatch;
    size_t bestMatchLength = 0;
    bool found = false;

    return ServerUtils::ressourceToResponse(ServerUtils::generateErrorPage(NOT_FOUND), NOT_FOUND);
}

ResponseInfos Server::handleDelete(const Request &request)
{
    map<string, Location> locs = server_config.getLocations();
    string url = request.getDecodedPath();

    Location bestMatch;
    size_t bestMatchLength = 0;
    bool found = false;

    for (const auto &loc : locs)
    {
        const string &locationPath = loc.first;
        if (url.find(locationPath) == 0 && (url == locationPath || url[locationPath.length()] == '/'))
        {
            if (locationPath.length() > bestMatchLength)
            {
                found = true;
                bestMatch = loc.second;
                bestMatchLength = locationPath.length();
            }
        }
    }

    if (found)
    {
        return ServerUtils::ressourceToResponse("<html><body><h1>DELETE Operation Successful on " + bestMatch.root + "</h1></body></html>", CREATED);
    }

    return ServerUtils::ressourceToResponse(ServerUtils::generateErrorPage(NOT_FOUND), NOT_FOUND);
}

bool Server::matchLocation(Location &loc, const string url)
{
    map<string, Location> locs = server_config.getLocations();
    map<string, Location>::const_iterator loc_it = locs.begin();
    Location bestMatch;
    size_t bestMatchLength = 0;
    bool found = false;

    for (; loc_it != locs.end(); loc_it++)
    {
        const string &locationPath = loc_it->first;

        // Check if URL starts with this location path
        if (url.find(locationPath) == 0)
        {

            if (locationPath.length() > bestMatchLength)
            {
                // std::cout << "Found better match: " << locationPath << std::endl;
                found = true;
                bestMatch = loc_it->second;

                bestMatchLength = locationPath.length();
            }
        }
    }
    loc = bestMatch;
    return found;
}

ResponseInfos Server::serveRessourceOrFail(RessourceInfo ressource)
{

    // cout << "serveRessourceOrFail " << ressource.path << endl;

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

ostream &operator<<(ostream &os, const Request &request)
{
    os << "-------------Method:---------\n " << request.getMethod() << endl;
    os << "-------------URI:----------\n"
       << request.getPath() << endl;
    os << "-------------Version:---------\n " << request.getVersion() << endl;
    os << "-------------Headers:----------\n"
       << endl;
    for (const auto &header : request.getHeaders())
    {
        os << header.first << ": " << header.second << endl;
    }
    os << "---------------Body:----------------\n"
       << request.getBody() << endl;
    return os;
}
