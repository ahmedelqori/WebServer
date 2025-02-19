/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aes-sarg <aes-sarg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/27 10:02:26 by ael-qori          #+#    #+#             */
/*   Updated: 2025/02/18 21:06:56 by aes-sarg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/ConfigParser.hpp"

ServerConfig::ServerConfig():       locationIndex(0){};
ConfigParser::ConfigParser():       current(0), index(0), currentServerState(HTTP),currentErrorPages(ERROR),currentLocationState(PATH){};
LocationConfig::LocationConfig()    {};


LocationConfig::LocationConfig(const LocationConfig &L)
{
    path = L.getPath();
    root = L.getRoot();
    methods = L.getMethods();
    indexFile = L.getIndexFile();
    cgiExtension = L.getCgiExtension();
    redirectionPath = L.getRedirectionPath();
    redirectionCode = L.getRedirectionCode();
    directoryListing = L.getDirectoryListing();
}
LocationConfig &LocationConfig::operator=(const LocationConfig &L)
{
    if (this == &L) return *this;
    path = L.getPath();
    root = L.getRoot();
    methods = L.getMethods();
    indexFile = L.getIndexFile();
    cgiExtension = L.getCgiExtension();
    redirectionPath = L.getRedirectionPath();
    redirectionCode = L.getRedirectionCode();
    directoryListing = L.getDirectoryListing();
    return *this;
}

void    ConfigParser::parseFile(const char *file_path)
{
    std::ifstream file;
    file.open(file_path, std::ifstream::in);
    if (!file.is_open()) Error(2, ERR_INPUT, ERR_FILE_NOT_EXIST);
    this->fileToVector(file);
    this->checkClosedParenthesis();
    this->parse();
}

void    ConfigParser::fileToVector(std::ifstream &file)
{
    std::string     line;
    std::string     pureLine;
    while(std::getline(file, line)) this->fileContent.push_back(line);
}

void    ConfigParser::deleteEmptyLines()
{
    size_t index = INDEX;
    while (++index < this->fileContent.size()) if (this->fileContent[index] == "")  this->fileContent.erase(this->fileContent.begin() + index--);
}

void    ConfigParser::checkClosedParenthesis()
{
    std::vector<std::string>    parenthesis;
    size_t                      index, i, tmp;

    for(index = 0; index < this->fileContent.size(); index++)
    {
        tmp = 0;
        for (i = 0; i < this->fileContent[index].size(); ++i)
        {
            if (this->fileContent[index][i] == O_PAR[0] && (tmp = 1)) parenthesis.push_back(O_PAR);
            if (this->fileContent[index][i] == C_PAR[0] && (tmp = 1) )
            {
                if(parenthesis.size() && parenthesis[parenthesis.size() - 1] == O_PAR)    parenthesis.pop_back();
                else parenthesis.push_back(O_PAR);
            }
        }
        this->fileContent[index] = trim(this->fileContent[index]);
        if (tmp && this->fileContent[index].size() != 1) Error(3, ERR_SYNTAX, LINE, itoa(index + 1).c_str());
    }
    if (parenthesis.size() != 0)  Error(2, ERR_SYNTAX, ERR_MIS_PAR);
    this->deleteEmptyLines();
}


void    ConfigParser::parse()
{
    while (this->index < static_cast<int>(this->fileContent.size()) && currentServerState != DONE)
    {
        switch (currentServerState)
        {
            case    HTTP                    :   this->handleHttpState()                 ; break;
            case    SERVER                  :   this->handleServerState()               ; break;
            case    HOST_PORT               :   this->handleHostPortState()             ; break;
            case    SERVER_NAME             :   this->handleServerNameState()           ; break;
            case    ERROR_PAGES             :   this->parseErrorPages()                 ; break;
            case    CLIENT_MAX_BODY_SIZE    :   this->handleClientMaxBodySizeState()    ; break;
            case    LOCATIONS               :   this->parseLocations()                  ; break;
            default                         :                                             break;
        }
    }
    if (currentServerState == DONE && this->index + 1 != static_cast<int>(this->fileContent.size())) Error(2, ERR_SYNTAX, W_HTTP);
}

void    ConfigParser::handleHttpState()
{
    static bool httpStart = false;
    if (!httpStart)
    {
        if (this->fileContent[this->index++] != W_HTTP) Error(2, ERR_SYNTAX, W_HTTP);
        if (this->fileContent[this->index++] != O_PAR) Error(3, ERR_SYNTAX, ERR_OPEN_PAR, W_HTTP);
        this->currentServerState = SERVER;
        httpStart = true;
    }
    else
    {
        if (this->fileContent[++this->index] != C_PAR && static_cast<int>(this->fileContent.size()) == this->index ) Error(3, ERR_SYNTAX, ERR_CLOSE_PAR, W_HTTP);
        else if (this->fileContent[this->index] != C_PAR) this->currentServerState = SERVER;
        else  (this->currentServerState = DONE);
    }
}

void    ConfigParser::handleServerState()
{
    static bool serverStart = false;
    if (!serverStart)
    { 
        this->servers.push_back(ServerConfig());
        if (this->fileContent[this->index++] != W_SERVER) Error(2, ERR_SYNTAX ,W_SERVER);
        if (this->fileContent[this->index++] != O_PAR) Error(3, ERR_SYNTAX, ERR_OPEN_PAR, W_SERVER);
        (serverStart = true, this->currentServerState = HOST_PORT);
    }
    else
    {
        serverStart = false;
        if (this->fileContent[this->index] != C_PAR) Error(3, ERR_SYNTAX, ERR_CLOSE_PAR, W_SERVER);
        else (this->current++ ,this->currentServerState = HTTP);
    }       
}
void    ConfigParser::handleHostPortState()
{
    std::vector<std::string> hostPort;
    std::vector<std::string> ArrayPorts;
    int                      index = INDEX;

    if (!this->servers[this->current].getHost().empty()) Error(3, ERR_SYNTAX, ERR_DUPLICATED, W_HOST_PORT);
    hostPort = splitString(this->fileContent[this->index++], WHITE_SPACES);
    if (hostPort[0] != W_HOST_PORT || hostPort.size() != 2) Error(4, ERR_SYNTAX, W_HOST_PORT, W_SERVER, itoa(this->current).c_str());
    hostPort = splitString(hostPort[1], ":");
    if (hostPort.size() != 2) Error(4, ERR_SYNTAX, W_HOST_PORT, W_SERVER, itoa(this->current).c_str());
    if (is_ipaddress(hostPort[0], INDEX) == false) Error(2, ERR_SYNTAX, W_HOST);
    ArrayPorts = splitString(hostPort[1], ",");
    while (++index < static_cast<int>(ArrayPorts.size()))
        if (is_number(ArrayPorts[index], INDEX) == false || atoi(ArrayPorts[index].c_str()) < 0 || atoi(ArrayPorts[index].c_str()) > 65535) Error(2, ERR_SYNTAX, W_PORT);
        else this->servers[this->current].setPorts(atoi(ArrayPorts[index].c_str()));
    this->servers[this->current].setHost(hostPort[0]);
    this->servers[this->current].setPort(atoi(ArrayPorts[0].c_str()));
    this->currentServerState = SERVER_NAME;
}

void    ConfigParser::handleServerNameState()
{
    std::vector<std::string> server_names;
    size_t                   i = 0;
    if (!this->servers[this->current].getServerNames().empty()) Error(3, ERR_SYNTAX, ERR_DUPLICATED, W_SERVER_NAMES);
    server_names = splitString(this->fileContent[this->index++], WHITE_SPACES);
    if (server_names[0] != W_SERVER_NAMES || server_names.size() < 2) Error(3, ERR_SYNTAX, W_SERVER_NAMES, W_SERVER, itoa(this->current).c_str());
    while (++i < server_names.size())
    {
        if (is_hostname(server_names[i], INDEX) == false)
            Error(2,ERR_SYNTAX, W_SERVER_NAMES);
        this->servers[this->current].setServerNames(server_names[i]);
    }
    this->currentServerState = ERROR_PAGES;
}

void    ConfigParser::parseErrorPages()
{
    while(this->index < static_cast<int>(this->fileContent.size()) && currentErrorPages != DONE_ERROR_PAGES)
    {
        switch (currentErrorPages)
        {
            case ERROR:         this->handleErrorPagesState(); break;
            case ERROR_FILE:    this->handleErrorFileState(); break;
            default: break;
        }
    }
    currentErrorPages = ERROR;
    currentServerState = CLIENT_MAX_BODY_SIZE;
}

void    ConfigParser::handleErrorPagesState()
{
    static bool errorPagesState = false;
    if (!errorPagesState)
    {
        if (this->fileContent[this->index++] != W_ERRORS_PAGES) Error(2, ERR_SYNTAX, W_ERRORS_PAGES);
        if (this->fileContent[this->index++] != O_PAR) Error(3, ERR_SYNTAX, ERR_OPEN_PAR, W_ERRORS_PAGES);
        (this->currentErrorPages = ERROR ,errorPagesState = true);
    }
    else
    {
        if (this->fileContent[this->index] != C_PAR) this->currentErrorPages = ERROR_FILE;
        else (this->index++, this->currentErrorPages = DONE_ERROR_PAGES, this->currentServerState = LOCATIONS, errorPagesState = false);
    }
}
void    ConfigParser::handleErrorFileState()
{
    std::vector<std::string> error_page;
    error_page = splitString(this->fileContent[this->index++], WHITE_SPACES);
    if (error_page[0] != W_ERROR|| error_page.size() != 3) Error(4, ERR_SYNTAX, W_ERROR, W_SERVER,  itoa(this->current).c_str());
    if (is_statuscode(error_page[1], INDEX) == false) Error(3, ERR_SYNTAX, W_ERROR, ERR_INVALID_STATUS_CODE);
    this->servers[this->current].setErrorPages(error_page[1], error_page[2]);
    this->currentErrorPages = ERROR;
}

void    ConfigParser::handleClientMaxBodySizeState()
{
    std::vector<std::string> clientMaxBodySize;
    std::vector<std::string> Array;
    unsigned long long       res;
    std::string              tmp;

    clientMaxBodySize = splitString(this->fileContent[this->index++], WHITE_SPACES);
    if (clientMaxBodySize[0] != W_CLIENT_MAX_BODY_SIZE || clientMaxBodySize.size() != 2)
        Error(4, ERR_SYNTAX, W_CLIENT_MAX_BODY_SIZE, W_SERVER, itoa(this->current).c_str());
    if (is_valid_size(clientMaxBodySize[1], INDEX) == false) Error(4, ERR_SYNTAX, W_CLIENT_MAX_BODY_SIZE, W_SERVER, itoa(this->current).c_str());
    (tmp = clientMaxBodySize[1][clientMaxBodySize[1].size() - 1], clientMaxBodySize[1][clientMaxBodySize[1].size() - 1] = ',', clientMaxBodySize[1] += tmp);
    (Array = splitString(clientMaxBodySize[1], ","), res = (unsigned long long)atoi(Array[0].c_str()) * parse_size(Array[1]));
    this->servers[this->current].setClientMaxBodySize(res);
    this->currentServerState = LOCATIONS; 
}

void    ConfigParser::parseLocations()
{
    while (this->index < static_cast<int>(this->fileContent.size()) && currentLocationState != END_LOCATION)
    {
        switch (currentLocationState)
        {
            case PATH           :       this->handlePathLocationState()     ;       break; 
            case METHODS        :       this->handleMethodsState()          ;       break;
            case ROOT           :       this->handleRootState()             ;       break;
            case REDIRECTION    :       this->handleRedirectionState()      ;       break;
            case AUTO_INDEX     :       this->handleDirectoryListingState() ;       break;
            case INDEX_FILE     :       this->handleIndexFileState()        ;       break;
            case CGI_EXT        :       this->handleCgiExtension()          ;       break;
            default             :                                                   break;
        }
    }
    currentLocationState = PATH;
}

void    ConfigParser::handlePathLocationState()
{
    int                             index = this->servers[this->current].locationIndex;
    static bool                     locationPageState = false;
    std::vector<std::string>        Array;

    if (!locationPageState)
    { 
        this->servers[this->current].setLocations(LocationConfig());
        Array = splitString(this->fileContent[this->index], WHITE_SPACES);
        if (Array.size() != 2) Error(4, ERR_SYNTAX, W_LOCATION, W_SERVER, itoa(this->current).c_str());
        if (Array[0] != W_LOCATION) Error(2, ERR_SYNTAX ,W_LOCATION);
        this->servers[this->current].getLocations()[index].setPath(Array[1]);
        if (this->fileContent[++this->index] != O_PAR) Error(3, ERR_SYNTAX, ERR_OPEN_PAR, W_LOCATION);
        (this->index++, locationPageState = true, this->currentLocationState = METHODS);
    }
    else
    {
        locationPageState = false;
        if (this->fileContent[this->index] != C_PAR) Error(3, ERR_SYNTAX, ERR_CLOSE_PAR, W_LOCATION);
        (this->index++, this->servers[this->current].locationIndex++,this->currentLocationState = END_LOCATION, this->currentServerState = SERVER);
        if (this->fileContent[this->index] != C_PAR)
            this->currentServerState = LOCATIONS;
    }       
}

void    ConfigParser::handleMethodsState()
{
    int                         index = this->servers[this->current].locationIndex;
    size_t                      i;
    std::vector<std::string>    Array;

    i = 1;
    if(!this->servers[this->current].getLocations()[index].getMethods().empty()) Error(3, ERR_SYNTAX, ERR_DUPLICATED, W_METHODS);
    Array = splitString(this->fileContent[this->index++], WHITE_SPACES);
    if (Array[0] != W_METHODS) {(this->currentLocationState = ROOT, this->index--); return;};
    while (i < Array.size())
    {
        if (Array[i] != GET && Array[i] != POST && Array[i] != DELETE) Error(3, ERR_SYNTAX, ERR_UNKNOWN_METHOD, Array[i].c_str()); 
        if (is_duplicated(this->servers[this->current].getLocations()[index].getMethods(), Array[i], INDEX) == false) Error(3, ERR_SYNTAX, ERR_DUPLICATED, Array[i].c_str());
        this->servers[this->current].getLocations()[index].setMethods(Array[i]);
        i++;
    }
    this->currentLocationState = ROOT;
}

void    ConfigParser::handleRootState()
{
    int                         index = this->servers[this->current].locationIndex;
    std::vector<std::string>    Array;
    
    if (!this->servers[this->current].getLocations()[index].getRoot().empty())Error(3, ERR_SYNTAX, ERR_DUPLICATED, W_ROOT);
    Array = splitString(this->fileContent[this->index++], WHITE_SPACES);
    if (Array[0] != W_ROOT) {(this->currentLocationState = REDIRECTION, this->index--); return ;};
    if (Array.size() != 2) Error(2, ERR_SYNTAX, W_ROOT);
    this->servers[this->current].getLocations()[index].setRoot(Array[1]);
    this->currentLocationState = REDIRECTION;
}

void    ConfigParser::handleRedirectionState()
{
    int                         index = this->servers[this->current].locationIndex;
    std::vector<std::string>    Array;

    if (!this->servers[this->current].getLocations()[index].getRedirectionPath().empty()) Error(3, ERR_SYNTAX, ERR_DUPLICATED, W_REDIRECTION);
    Array = splitString(this->fileContent[this->index++], WHITE_SPACES);
    
    if (Array[0] != W_REDIRECTION)  {(this->currentLocationState = AUTO_INDEX, this->index--); return ;};
    if (Array.size() != 3) Error(2, ERR_SYNTAX, W_REDIRECTION);
    if (!is_number(Array[1], INDEX) || !is_statuscode(Array[1], INDEX))
        Error(2, ERR_SYNTAX, W_REDIRECTION);
    this->servers[this->current].getLocations()[index].setRedirectionCode(atoi(Array[1].c_str()));
    this->servers[this->current].getLocations()[index].setRedirectionPath(Array[2]);
    this->currentLocationState = AUTO_INDEX;
} 

void    ConfigParser::handleDirectoryListingState()
{
    int                         index = this->servers[this->current].locationIndex;
    std::vector<std::string>    Array;
    
    Array = splitString(this->fileContent[this->index++], WHITE_SPACES);
    if (Array[0] != W_AUTO_INDEX) {(this->currentLocationState = INDEX_FILE, this->index--); return ;};
    if (Array.size() != 2) Error(2, ERR_SYNTAX, W_AUTO_INDEX);
    if (Array[1] != ON && Array[1] != OFF) Error(3, ERR_SYNTAX, ON, OFF);
    if (Array[1] == ON) this->servers[this->current].getLocations()[index].setDirectoryListing(true);
    else this->servers[this->current].getLocations()[index].setDirectoryListing(false);
    this->currentLocationState = INDEX_FILE;
}

void    ConfigParser::handleIndexFileState()
{
    int                         index  = this->servers[this->current].locationIndex;
    std::vector<std::string>    Array;

    if (!this->servers[this->current].getLocations()[index].getIndexFile().empty()) Error(3,ERR_SYNTAX, ERR_DUPLICATED, W_INDEX);
    Array = splitString(this->fileContent[this->index++], WHITE_SPACES);
    if (Array[0] != W_INDEX) {(this->currentLocationState = CGI_EXT, this->index--); return ;};
    if (Array.size() != 2 || Array[0] != W_INDEX) Error(2, ERR_SYNTAX, W_INDEX);
    this->servers[this->current].getLocations()[index].setIndexFile(Array[1]);
    this->currentLocationState = CGI_EXT;
}

void   ConfigParser::handleCgiExtension()
{
    int                         index  = this->servers[this->current].locationIndex;
    std::vector<std::string>    Array;

    Array = splitString(this->fileContent[this->index++], WHITE_SPACES);
    if (Array[0] != W_CGI) {(this->currentLocationState = PATH, this->index--); return ;};
    if (Array.size() != 3) Error(2, ERR_SYNTAX, W_CGI);
    this->servers[this->current].getLocations()[index].setCgiExtension(Array[1], Array[2]);
}


int                                 ServerConfig::getPort() const                                       {    return this->port;}
std::vector<int>                    ServerConfig::getPorts() const                                      {    return this->ports;};
std::string                         ServerConfig::getHost() const                                       {    return this->host;}
std::vector<LocationConfig>         &ServerConfig::getLocations()                                       {    return this->locations;}
std::map<std::string, std::string>  ServerConfig::getErrorPages() const                                 {    return this->errorPages;}
std::vector<std::string>            ServerConfig::getServerNames() const                                {    return this->serverNames;}
unsigned long long                  ServerConfig::getClientMaxBodySize() const                          {    return this->clientMaxBodySize; }
            
std::string                         LocationConfig::getPath() const                                     {    return this->path;}
std::string                         LocationConfig::getRoot() const                                     {    return this->root;}
std::vector<std::string>            LocationConfig::getMethods() const                                  {    return this->methods;}
std::string                         LocationConfig::getIndexFile() const                                {    return this->indexFile;}
std::string                         LocationConfig::getRedirectionPath() const                          {    return this->redirectionPath;}
int                                 LocationConfig::getRedirectionCode() const                          {    return this->redirectionCode;}
bool                                LocationConfig::getDirectoryListing() const                         {    return this->directoryListing;}
std::map<std::string, std::string>  LocationConfig::getCgiExtension() const                             {    return this->cgiExtension;};


void                                ServerConfig::setPort(int port)                                     {    this->port = port;}
void                                ServerConfig::setPorts(int port)                                    {    this->ports.push_back(port);}
void                                ServerConfig::setHost(std::string &host)                            {    this->host = host;}
void                                ServerConfig::setLocations(LocationConfig location)                 {    this->locations.push_back(location);}
void                                ServerConfig::setClientMaxBodySize(unsigned long long size)               {    this->clientMaxBodySize = size;}
void                                ServerConfig::setServerNames(std::string &server_name)              {    this->serverNames.push_back(server_name);}
void                                ServerConfig::setErrorPages(std::string &key, std::string &value)   {    this->errorPages.insert(std::make_pair(key, value));}

void                                LocationConfig::setPath(std::string path)                           {    this->path = path;}
void                                LocationConfig::setRoot(std::string root)                           {    this->root = root;}
void                                LocationConfig::setMethods(std::string key)                         {    this->methods.push_back(key);}
void                                LocationConfig::setDirectoryListing(bool b)                         {    this->directoryListing = b;}
void                                LocationConfig::setIndexFile(std::string index)                     {    this->indexFile = index;}
void                                LocationConfig::setRedirectionCode(int statusCode)                  {    this->redirectionCode = statusCode;}
void                                LocationConfig::setRedirectionPath(std::string path)                {    this->redirectionPath = path;}
void                                LocationConfig::setCgiExtension(std::string ext, std::string path)  {    this->cgiExtension.insert(std::make_pair(ext, path));}