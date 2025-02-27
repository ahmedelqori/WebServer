/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpParser.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aes-sarg <aes-sarg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/13 20:23:18 by aes-sarg          #+#    #+#             */
/*   Updated: 2025/02/27 14:40:50 by aes-sarg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/HttpParser.hpp"

HttpParser::HttpParser() : state(REQUEST_LINE) {}

Request HttpParser::parse(const string &data, int size)
{

    string line;
    bool hasCRLF = false;
    for (int i = 0; i < size; ++i)
    {
        if (data[i] == '\r' && state != BODY)
        {
            if (i > 0 && state == REQUEST_LINE && data[i - 1] && data[i - 1] == ' ')
                throw BAD_REQUEST;
            if (i + 1 < size && data[i + 1] == '\n')
            {
                i++;
                if (data[i + 1] == ' ')
                    throw BAD_REQUEST;
                if (!line.empty())
                {
                    processLine(line, i);
                    line.clear();
                }
                else if (state == HEADER)
                {
                    state = BODY;
                    hasCRLF = true;
                }
            }
            else
                throw BAD_REQUEST;
        }
        else
            line += data[i];
    }

    if (!hasCRLF && state == HEADER)
        throw BAD_REQUEST;
    if (!line.empty())
        processLine(line, 0);

    validateHeaders();

    Request request;
    request.setMethod(method);
    request.setPath(uri);
    request.setQueryParams(query_params);
    request.setDecodedPath(uri);
    request.setVersion(version);
    request.setHeaders(headers);
    request.setBody(body);
    return request;
}

void HttpParser::processLine(const string &line, int position)
{
    switch (state)
    {
    case REQUEST_LINE:
        parseRequestLine(line, position);
        break;
    case HEADER:
        parseHeader(line);
        break;
    case BODY:
        body += line;
        break;
    }
}

bool HttpParser::isChunkedData()
{
    return (headers.count(TRANSFER_ENCODING) > 0 && headers[TRANSFER_ENCODING] == CHUNKED);
}

void HttpParser::parseRequestLine(const string &line, int i)
{
    (void)i;
    stringstream ss(line);
    string last;
    int j = 0;
    if (line[j] <= 32)
        throw BAD_REQUEST;
    while (j < i)
    {
        if ((line[j] == ' ' && line[j + 1] && line[j + 1] == ' ') || (line[j] >= 9 && line[j] <= 13))
            throw BAD_REQUEST;
        j++;
    }

    ss >> method >> uri >> version >> last;

    if (method.empty() || uri.empty() || version.empty() || !last.empty())
    {
        throw BAD_REQUEST;
    }
    if (uri[0] != '/')
        throw BAD_REQUEST;
    if (version != HTTP_VERSION)
    {
        throw VERSION_NOT_SUPPORTED;
    }
    parseHttpUrl(uri);

    validateMethod(method);
    uri = validatePath(uri);

    if (uri.find("?") != string::npos)
    {
        size_t separator = uri.find("?");
        string path = uri.substr(0, separator);
        string query = uri.substr(separator + 1);
        query_params = parseParams(query);
        uri = path;
    }
    state = HEADER;
}

void HttpParser::parseHttpUrl(string &url)
{

    string ipWithPort, path;

    size_t pos = url.find("://");
    if (pos != string::npos)
    {
        pos += 3;
    }
    else
    {
        pos = 0;
    }

    size_t pathPos = url.find("/", pos);
    if (pathPos != string::npos)
    {
        ipWithPort = url.substr(pos, pathPos - pos);
        path = url.substr(pathPos);
    }
    else
    {
        ipWithPort = url.substr(pos);
        path = "/";
    }
    url = path;
}

static void lowerString(string &str)
{
    for (size_t i = 0; i < str.size(); i++)
        str[i] = tolower(str[i]);
}

static void validateWhiteSpaces(string str)
{
    int i = 0;
    while (str[i])
    {
        if (str[i] == ' ')
            throw BAD_REQUEST;
        i++;
    }
    
}

void HttpParser::parseHeader(const string &line)
{
    if (line.empty())
    {
        state = BODY;
        return;
    }

    size_t separator = line.find(':');
    if (separator == string::npos)
    {
        throw BAD_REQUEST;
    }

    string name = line.substr(0, separator);
    string value = line.substr(separator + 1);

    if ((!value.empty() && value[0] != ' ') || value[1] == ' ')
        throw BAD_REQUEST;
    if (value.size() < 2)
        throw BAD_REQUEST;

    if (name.find('\n') != string::npos)
        throw BAD_REQUEST;
    validateWhiteSpaces(name);

    lowerString(name);
    trim(value);

    headers[name] = value;
}

void HttpParser::parseBody(const string &body)
{

    if (!body.empty() && method == GET)
        return;
}

void HttpParser::trim(string &str)
{
    size_t start = str.find_first_not_of(" \t");
    if (start != string::npos)
    {
        str = str.substr(start);
    }
    size_t end = str.find_last_not_of(" \t");
    if (end != string::npos)
    {
        str = str.substr(0, end + 1);
    }
}

void HttpParser::validateHeaders()
{
    if (!headers.count(HOST))
        throw BAD_REQUEST;
}

void HttpParser::validateMethod(const std::string &method)
{

    if (method != POST && method != GET && method != DELETE)
        throw NOT_ALLOWED;
}

bool HttpParser::isHexDigit(char c)
{
    return isxdigit(static_cast<unsigned char>(c));
}

char HttpParser::hexToChar(char high, char low)
{
    int highVal = isdigit(high) ? high - '0' : toupper(high) - 'A' + 10;
    int lowVal = isdigit(low) ? low - '0' : toupper(low) - 'A' + 10;
    return static_cast<char>((highVal << 4) | lowVal);
}

bool HttpParser::isBadUri(const std::string &uri)
{

    static const std::string unreserved = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
    static const std::string gen_delims = ":/?#[]@";
    static const std::string sub_delims = "!$&'()*+,;=";
    static const std::string reserved = gen_delims + sub_delims;
    static const std::string allowed = unreserved + reserved;

    for (size_t i = 0; i < uri.length(); ++i)
    {
        char c = uri[i];

        if (c == '%')
        {
            if (i + 2 >= uri.length() ||
                !isxdigit(static_cast<unsigned char>(uri[i + 1])) ||
                !isxdigit(static_cast<unsigned char>(uri[i + 2])))
            {
                return true; 
            }
            i += 2; 
            continue;
        }

      
        if (c < 32 || c > 126)
        {
            return true;
        }

       
        if (allowed.find(c) == std::string::npos)
        {
            return true;
        }

       
        if (c == '#')
        {
         
            for (size_t j = i + 1; j < uri.length(); ++j)
            {
                char fc = uri[j];
                if (fc < 32 || fc > 126 || (fc != '#' && allowed.find(fc) == std::string::npos))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

std::string HttpParser::validatePath(const std::string &path)
{
    if (path.empty())
    {
        throw BAD_REQUEST;
    }

    if (path[0] != '/')
    {
        throw BAD_REQUEST;
    }

    if (path.length() > 2048)
    {
        throw URI_TOO_LONG;
    }


    if (isBadUri(path))
    {
        throw BAD_REQUEST;
    }


    if (isBadUriTraversal(path))
        throw BAD_REQUEST;


    std::string decoded;
    for (size_t i = 0; i < path.size(); i++)
    {
        if (path[i] == '%')
        {
            if (i + 2 >= path.size())
            {
                throw BAD_REQUEST;
            }

            if (!isHexDigit(path[i + 1]) || !isHexDigit(path[i + 2]))
            {
                throw BAD_REQUEST;
            }

            char decodedChar = hexToChar(path[i + 1], path[i + 2]);

            // Check if decoded character is valid
            if (decodedChar < 32 || decodedChar > 126)
            {
                throw BAD_REQUEST;
            }

            decoded += decodedChar;
            i += 2;
        }
        else
        {
            decoded += path[i];
        }
    }

    return decoded;
}

bool HttpParser::isBadUriTraversal(const std::string &uri)
{
    std::string::size_type pos = 0;
    std::string::size_type prevPos = 0;

    while ((pos = uri.find('/', prevPos)) != std::string::npos)
    {
        if (pos == prevPos)
        {
            prevPos = pos + 1;
            continue;
        }

        std::string segment = uri.substr(prevPos, pos - prevPos);

       
        if (segment == ".." ||
            segment == "." ||
            segment.find("../") != std::string::npos ||
            segment.find("./") != std::string::npos)
        {
            return true;
        }

        prevPos = pos + 1;
    }


    if (prevPos < uri.length())
    {
        std::string lastSegment = uri.substr(prevPos);
        if (lastSegment == ".." ||
            lastSegment == "." ||
            lastSegment.find("../") != std::string::npos ||
            lastSegment.find("./") != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

map<string, string> HttpParser::parseParams(const string &query)
{
    map<string, string> params;
    size_t start = 0;
    size_t end = 0;

    while (end != string::npos)
    {
        end = query.find('&', start);
        string pair = query.substr(start, end - start);

        size_t separator = pair.find('=');
        if (separator != string::npos)
        {
            string key = pair.substr(0, separator);
            string value = pair.substr(separator + 1);
            params[key] = value;
        }
        else
        {
            params[pair] = "";
        }

        start = end + 1;
    }

    return params;
}

string HttpParser::getMethod() const
{
    return method;
}

string HttpParser::getUri() const
{
    return uri;
}

string HttpParser::getVersion() const
{
    return version;
}

map<string, string> HttpParser::getHeaders() const
{
    return headers;
}

string HttpParser::getBody() const
{
    return body;
}
