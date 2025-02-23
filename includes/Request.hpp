#pragma once

#include <string>
#include <map>

class Request
{
private:
    std::string method;
    std::string path;
    std::string decoded_path;
    std::string version;
    std::string host;
    std::map<std::string, std::string> headers;
    std::string body;
    std::map<std::string, std::string> query_params;

public:
    Request();
    int buffer_size;
    const std::string &getMethod() const;
    const std::string &getPath() const;
    const std::string &getDecodedPath() const;
    const std::string &getVersion() const;
    const std::map<std::string, std::string> &getHeaders() const;
    const std::string &getHeader(const std::string &key) const;
    bool hasHeader(const std::string &key) const;
    const std::string &getBody() const;
    void clearRequest();
    const std::map<std::string, std::string> &getQueryParams() const;
    static std::string generateErrorPage(const int code);
    static std::string generateStatusMsg(const int code);
 

    void setMethod(const std::string &method);
    void setPath(const std::string &path);
    void setDecodedPath(const std::string &decoded_path);
    void setVersion(const std::string &version);
    void setHeaders(const std::map<std::string, std::string> &headers);
    void setBody(const std::string &body);
    void setQueryParams(const std::map<std::string, std::string> &query_params);

   
};

