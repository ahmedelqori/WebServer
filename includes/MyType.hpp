#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <fstream>

using namespace std;

#define BAD_REQUEST 400
#define OK 200
#define CREATED 201
#define INTERNAL_SERVER_ERROR 500
#define FORBIDEN 403
#define REDIRECTED 301
#define NOT_ALLOWED 405
#define NOT_FOUND 404
#define URI_TOO_LONG 414
#define BAD_GATEWAY 502
#define VERSION_NOT_SUPPORTED 505
#define PAYLOAD_TOO_LARGE 413
#define NO_CONTENT 204
#define CRLF_CRLF "\r\n\r\n"

#define NOT_FOUND_CODE "404"
#define HOST "host"
#define CONTENT_LENGTH "content-length"
#define TRANSFER_ENCODING "transfer-encoding"
#define CONTENT_TYPE "content-type"
#define CHUNKED "chunked"
#define HTTP_VERSION "HTTP/1.1"


#define MSG_BAD_REQUEST "Bad Request"
#define MSG_OK "OK"
#define MSG_CREATED "Created"
#define MSG_INTERNAL_SERVER_ERROR "Internal Server Error"
#define MSG_FORBIDDEN "Forbidden"
#define MSG_REDIRECTED "Redirected"
#define MSG_NOT_ALLOWED "Method Not Allowed"
#define MSG_NOT_FOUND "Not Found"

#define GET "GET"
#define POST "POST"
#define DELETE "DELETE"

#define READ_BUFFER_SIZE 8192

enum File_Type
{
    DIRECTORY,
    REGULAR,
    NOT_EXIST,
    UNDEFINED
};

enum State
{
    REQUEST_LINE,
    HEADER,
    BODY
};

struct Location
{
    string root;
    vector<string> allow_methods;
    bool autoindex;
    vector<string> index_files;
    size_t client_max_body_size;
    string upload_dir;
    string redirect;
    int redirectCode;
    Location() : autoindex(false), client_max_body_size(0) {}
};

struct RessourceInfo
{
    string path;
    string url;
    string root;
    map<string,string> errors_pages;
    map<string,string>cgi_infos;
    string indexFile;
    string redirect;
    bool autoindex;
};

struct ResponseInfos
{
    int status;
    string statusMessage;
    map<string, string> headers;
    string location;
    string body;
    string response_str;
    size_t bytes_written;
    size_t bytes_sent;
    string filePath;
    string response;

    ResponseInfos() : status(OK), statusMessage(MSG_OK), body(""),bytes_written(0),response("") {}
    ResponseInfos(int s, const string &sm, const string &b) : status(s), statusMessage(sm), body(b) {}
    ResponseInfos &operator=(const ResponseInfos &r)
    {
        if (this != &r)
        {
            status = r.status;
            statusMessage = r.statusMessage;
            headers = r.headers;
            filePath = r.filePath;
            location = r.location;
            body = r.body;
        }
        return *this;
    }
    void setHeaders(const map<string, string> &h) { headers = h; }
    void addHeader(const string &key, const string &value)
    {
        headers[key] = value;
    }
    void setBody(const string &b) { body = b; }
    int getStatus() const { return status; }
    string getStatusMessage() const { return statusMessage; }
    const map<string, string> &getHeaders() const { return headers; }
    string getBody() const { return body; }
    void setStatus(int s) { status = s; }
    void setStatusMessage(const string &sm) { statusMessage = sm; }
    void parseHeaders(const string &response);
    string getHeader(const string &key) const
    {

        map<string, string>::const_iterator it = headers.find(key);

        return (it != headers.end()) ? it->second : "";
    }
    bool hasHeader(const string& key) const { return headers.find(key) != headers.end(); }
};
