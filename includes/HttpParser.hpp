#pragma once
#include <string>
#include <map>
#include "Request.hpp"
#include "MyType.hpp"
#include <iostream>
#include <sstream>

class HttpParser
{
public:
    HttpParser();
    Request parse(const string &data, int);
    string getMethod() const;
    string getUri() const;
    string getVersion() const;
    map<string, string> getHeaders() const;
    void parseHttpUrl(string &url);
    string getBody() const;

private:
    void parseRequestLine(const string &line, int);
    void parseHeader(const string &line);
    void parseBody(const string &body);
    void processLine(const string &line, int);
    void validateHeaders();
    string validatePath(const string &path);
    void validateMethod(const string &method);
    bool isHexDigit(char c);
    bool isBadUri(const string &uri);
    bool isChunkedData();
    char hexToChar(char high, char low);
    bool isBadUriTraversal(const string &uri);
    map<string, string> parseParams(const string &query);
    void trim(string &str);

    State state;
    string method;
    string uri;
    string version;
    map<string, string> headers;
    map<string, string> query_params;
    string body;
};
