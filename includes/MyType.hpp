#pragma once 
#include <iostream>
#include <vector>
#include <map>

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


enum File_Type
{
    DIRECTORY,
    REGULAR,
    NOT_EXIST,
    UNDEFINED
};

enum State {
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

    ResponseInfos() : status(OK), statusMessage(MSG_OK), body("") {}
    ResponseInfos(int s, const string &sm, const string &b) : status(s), statusMessage(sm), body(b) {}
    void setHeaders(const map<string, string> &h) { headers = h; }
    void addHeader(const string &key, const string &value) {
        headers[key] = value;
    }
    void setBody(const string &b) { body = b; }
    int getStatus() const { return status; }
    string getStatusMessage() const { return statusMessage; }
    const map<string, string> &getHeaders() const { return headers; }
    string getBody() const { return body; }
    void setStatus(int s) { status = s; }
    void setStatusMessage(const string &sm) { statusMessage = sm; }
};