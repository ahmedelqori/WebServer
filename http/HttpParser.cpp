#include "HttpParser.hpp"
#include <iostream>
#include <sstream>
#include <set>

HttpParser::HttpParser() : state(REQUEST_LINE) {}

Request HttpParser::parse(const string &data)
{

cout << "Parsing request" << data << endl;
    
    string line;
    // size_t pos = data.find("\r\n\r\n");
    for (size_t i = 0; i < data.size() ; ++i)
    {
        if (data[i] == '\r' && state != BODY)
        {
            if (i + 1 < data.size() && data[i + 1] == '\n')
            {
                i++;
                processLine(line);
                line.clear();
            }
            else
            {
                cout << "400 here 1 " << line << endl;
                throw BAD_REQUEST;
            }
        }
        else if (data[i] == '\n' && state != BODY)
        {
            processLine(line);
            line.clear();
        }
        else
        {
            line += data[i];
        }
    }

    // if (!(line.empty() && line.find("\r\n\r\n") && state == BODY))
    //     throw BAD_REQUEST;
        

    if (!line.empty())
    {
        processLine(line);
    }

    validateHeaders();

    if (state == BODY && method == "POST")
    {
        parseBody(this->body);
    }
    
    Request request;
    request.setMethod(method);
    request.setPath(uri);
   
    request.setDecodedPath(uri);
    request.setVersion(version);
    request.setHeaders(headers);
    request.setBody(body);
    return request;
}
void HttpParser::processLine(const string &line)
{
    switch (state)
    {
    case REQUEST_LINE:
        parseRequestLine(line);
        break;
    case HEADER:
        parseHeader(line);
        break;
    case BODY:
        body += line;
        break;
    }
}

void HttpParser::parseRequestLine(const string &line)
{
    stringstream ss(line);
    string last;
    ss >> method >> uri >> version >> last;

    if (method.empty() || uri.empty() || version.empty() || !last.empty())
    {
        std::cerr << "Invalid request line: " << line << std::endl;
        throw BAD_REQUEST;
    }
    if (version != "HTTP/1.1")
    {
        throw VERSION_NOT_SUPPORTED;
    }
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
        std::cerr << "Invalid header: " << line << std::endl;
        throw BAD_REQUEST;
    }

    string name = line.substr(0, separator);
    string value = line.substr(separator + 1);

     trim(name);
    trim(value);

    headers[name] = value;
}

void HttpParser::parseBody(const string &body)
{

    // if (headers.count("Content-Length") > 0)
    // {
    //     int content_length = stoi(headers["Content-Length"]);
    //     if (body.length() < content_length)
    //     {
    //         throw BAD_REQUEST;
    //     }
    //     query_params = parseParams(body);
    // }
    // else
    // {
      
    //     // if (body.find("\r\n\n") != string::npos)
    //     // {
    //     //     throw BAD_REQUEST;
    //     // }
    // }

    return;
}

void HttpParser::trim(string &str)
{
    // Remove leading whitespace
    size_t start = str.find_first_not_of(" \t");
    if (start != string::npos)
    {
        str = str.substr(start);
    }
    // Remove trailing whitespace
    size_t end = str.find_last_not_of(" \t");
    if (end != string::npos)
    {
        str = str.substr(0, end + 1);
    }
}

void HttpParser::validateHeaders()
{
    if (!headers.count("Host"))
    {
         cout << "400 here hosty "  << endl;
        throw BAD_REQUEST;
    }
}

void HttpParser::validateMethod(const std::string &method)
{
    static const std::set<std::string> allowedMethods = {"GET", "POST", "DELETE"};
    if (allowedMethods.find(method) == allowedMethods.end())
    {
        throw NOT_ALLOWED;
    }
}

bool HttpParser::isHexDigit(char c)
{
    return std::isxdigit(static_cast<unsigned char>(c));
}

char HttpParser::hexToChar(char high, char low)
{
    int highVal = std::isdigit(high) ? high - '0' : std::toupper(high) - 'A' + 10;
    int lowVal = std::isdigit(low) ? low - '0' : std::toupper(low) - 'A' + 10;
    return static_cast<char>((highVal << 4) | lowVal);
}

bool HttpParser::isBadUri(const std::string &uri)
{
    // RFC 3986 allowed characters in URLs
    static const std::string unreserved = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
    static const std::string gen_delims = ":/?#[]@";
    static const std::string sub_delims = "!$&'()*+,;=";
    static const std::string reserved = gen_delims + sub_delims;
    static const std::string allowed = unreserved + reserved;

    for (size_t i = 0; i < uri.length(); ++i)
    {
        char c = uri[i];

        // Allow percent-encoded characters
        if (c == '%')
        {
            if (i + 2 >= uri.length() ||
                !isxdigit(static_cast<unsigned char>(uri[i + 1])) ||
                !isxdigit(static_cast<unsigned char>(uri[i + 2])))
            {
                return true; // Invalid percent encoding
            }
            i += 2; // Skip the two hex digits
            continue;
        }

        // Check for control characters and non-ASCII characters
        if (c < 32 || c > 126)
        {
            return true;
        }

        // Check if character is in allowed set
        if (allowed.find(c) == std::string::npos)
        {
            return true;
        }

        // Special handling for fragment identifier (#)
        if (c == '#')
        {
            // Fragment must be the last part of the URI
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

    if (path.length() > 84)
    {
        throw URI_TOO_LONG;
    }

    // Check for bad URI characters
    if (isBadUri(path))
    {
        std::cout << "Is bad uri " << std::endl;
        throw BAD_REQUEST;
    }

    // Check for path traversal
    if (isBadUriTraversal(path))
    {
        std::cout << "Is Traversal uri " << std::endl;

        throw BAD_REQUEST;
    }

    // Decode percent-encoded characters
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

// Updated isBadUriTraversal function
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

        // Check for path traversal attempts
        if (segment == ".." ||
            segment == "." ||
            segment.find("../") != std::string::npos ||
            segment.find("./") != std::string::npos)
        {
            return true;
        }

        prevPos = pos + 1;
    }

    // Check the last segment
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
