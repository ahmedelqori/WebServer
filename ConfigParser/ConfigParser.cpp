/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/27 10:02:26 by ael-qori          #+#    #+#             */
/*   Updated: 2024/12/27 14:46:50 by ael-qori         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"

std::string itoa(size_t n)
{
    std::ostringstream oss;

    oss << n;
    return oss.str();
}
std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) 
        return "";
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}


std::vector<std::string> splitString(const std::string& input, const std::string& delimiters)
{
    std::vector<std::string> tokens;
    std::string token;
    size_t start = 0;
    
    for (size_t i = 0; i < input.length(); ++i) {
        if (delimiters.find(input[i]) != std::string::npos) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += input[i];
        }
    }
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}
void    Error(int count, ...)
{
    int     i = -1;
    va_list args;
    va_start(args, count);
    std::string error;
    char    *tmp;
    
    count--;
    tmp = (char *)va_arg(args, char *);
    if (tmp == NULL)    
        return ;
    error += BOLDRED + std::string(tmp) + RESET;
    while (++i <  count)
    {
        tmp = (char *)va_arg(args, char *);
        if (tmp == NULL) return ;
        error += " " + std::string(tmp);
    }
    throw std::runtime_error(error);
}

bool is_number(std::string str, int index)
{
    while (str[++index]) if (!isdigit(str[index])) return false;
    return true;
}

bool is_ipaddress(std::string str, int index)
{
    std::vector<std::string> Array;
    int countDots = 0;
    int byte;

    while (str[++index]) if (str[index] == '.') countDots++;
    if (countDots != 3) return false;
    Array = splitString(str, ".");
    if (Array.size() != 4 && ((index = INDEX) || true)) return false;
    index = INDEX;
    while (++index < Array.size() && ((byte = atoi(Array[index].c_str()) )|| true))
        if(Array[index] != itoa(byte) || byte < 0 || byte > 255) return false;
    return true;
}

bool is_statuscode(std::string str, int index)
{
    int status;

    status = atoi(str.c_str());
    if (itoa(status) != str) return false;
    if (status < 100 || status > 999) return false;
    return true;
}

int main(int ac, char **av)
{
    ConfigParser    configParser;
    
    try {
        if (ac != 2) Error(2, ERR_INPUT, ERR_ARGS);
        configParser.parseFile(av[1]);
    } catch (const std::exception& e) {
        std::cerr << std::endl <<e.what() << std::endl << std::endl;
        return 1; 
    }   
}

ConfigParser::ConfigParser():index(0), current(0), currentServerState(HTTP),currentErrorPages(ERROR){};

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
    int index = -1;
    while (++index < this->fileContent.size()) if (this->fileContent[index] == "")  this->fileContent.erase(this->fileContent.begin() + index--);
}

void    ConfigParser::checkClosedParenthesis()
{
    std::vector<std::string> parenthesis;
    int index , i, tmp;
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
    while (this->index < this->fileContent.size() && currentServerState != DONE)
    {
        switch (currentServerState)
        {
            case    HTTP        :   this->handleHttpState();            break;
            case    SERVER      :   this->handleServerState();          break;
            case    HOST_PORT   :   this->handleHostPortState();        break;
            case    SERVER_NAME :   this->handleServerNameState();      break;
            case    ERROR_PAGES :   this->parseErrorPages();            break;
            default         :                                 break;
        }
    }
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
        if (this->fileContent[++this->index] != C_PAR && this->fileContent.size() == this->index + 1) Error(3, ERR_SYNTAX, ERR_CLOSE_PAR, W_HTTP);
        else if (this->fileContent[this->index] != C_PAR) this->currentServerState = SERVER;
        else  this->currentServerState = DONE;
    }
}

void    ConfigParser::handleServerState()
{
    static bool serverStart = false;
    this->servers.push_back(ServerConfig());
    if (!serverStart)
    { 
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
    
    if (!this->servers[this->current].getHost().empty()) Error(3, ERR_SYNTAX, ERR_DUPLICATED, W_HOST_PORT);
    hostPort = splitString(this->fileContent[this->index++], WHITE_SPACES);
    if (hostPort[0] != W_HOST_PORT || hostPort.size() != 2) Error(4, ERR_SYNTAX, W_HOST_PORT, W_SERVER, itoa(this->current).c_str());
    hostPort = splitString(hostPort[1], ":");
    if (hostPort.size() != 2) Error(4, ERR_SYNTAX, W_HOST_PORT, W_SERVER, itoa(this->current).c_str());
    if (is_ipaddress(hostPort[0], INDEX) == false) Error(2, ERR_SYNTAX, W_HOST);
    if (is_number(hostPort[1], INDEX) == false) Error(2, ERR_SYNTAX, W_PORT);
    this->servers[this->current].setHost(hostPort[0]);
    this->servers[this->current].setPort(atoi(hostPort[1].c_str()));
    this->currentServerState = SERVER_NAME;
}

void    ConfigParser::handleServerNameState()
{
    std::vector<std::string> server_names;
    int                      i = 0;
    if (!this->servers[this->current].getServerNames().empty()) Error(3, ERR_SYNTAX, ERR_DUPLICATED, W_SERVER_NAMES);
    server_names = splitString(this->fileContent[this->index++], WHITE_SPACES);
    if (server_names[0] != W_SERVER_NAMES || server_names.size() < 2) Error(3, ERR_SYNTAX, W_SERVER_NAMES, W_SERVER, itoa(this->current).c_str());
    while (++i < server_names.size()) this->servers[this->current].setServerNames(server_names[i]);
    this->currentServerState = ERROR_PAGES;
}

void    ConfigParser::parseErrorPages()
{
    while(this->index < this->fileContent.size() && currentErrorPages != DONE_ERROR_PAGES)
    {
        switch (currentErrorPages)
        {
            case ERROR:         this->handleErrorPagesState(); break;
            case ERROR_FILE:    this->handleErrorFileState(); break;
            default: break;
        }
    }
    currentErrorPages = ERROR;
    currentServerState = SERVER;
}

void    ConfigParser::handleErrorPagesState()
{
    static bool errorPagesState = false;
    if (!errorPagesState)
    {
        if (this->fileContent[this->index++] != W_ERRORS_PAGES) Error(2, ERR_SYNTAX, W_ERRORS_PAGES);
        if (this->fileContent[this->index++] != O_PAR) Error(3, ERR_SYNTAX, ERR_OPEN_PAR, W_ERRORS_PAGES);
        (this->currentErrorPages = ERROR_FILE ,errorPagesState = true);
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

//////////////////////////////////////////////////////////
std::string                         ServerConfig::getClientMaxBodySize() const
{
    return this->clientMaxBodySize;
}
int                                 ServerConfig::getPort() const
{
    return this->port;   
}
std::string                         ServerConfig::getHost() const
{
    return this->host;
}
std::vector<std::string>            ServerConfig::getServerNames() const
{
    return this->serverNames;
}
std::vector<LocationConfig>         &ServerConfig::getLocations()
{
    return this->locations;
}
std::map<std::string, std::string>  ServerConfig::getErrorPages() const
{
    return this->errorPages;
}
void                                ServerConfig::setClientMaxBodySize(std::string &size)
{
    this->clientMaxBodySize = size;
}
void                                ServerConfig::setPort(int port)
{
    this->port = port;   
}
void                                ServerConfig::setHost(std::string &host)
{
    this->host = host;   
}
void                                ServerConfig::setServerNames(std::string &server_name)
{
    this->serverNames.push_back(server_name);
}
void                                ServerConfig::setLocations(LocationConfig location)
{
    this->locations.push_back(location);
}
void                                ServerConfig::setErrorPages(std::string &key, std::string &value)
{
    this->errorPages.insert(std::make_pair(key, value));   
}
