/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbentahi <mbentahi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/28 16:05:18 by mbentahi          #+#    #+#             */
/*   Updated: 2025/01/05 16:04:19 by mbentahi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>

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
#define VERSION_NOT_SUPPORTED 505

#define MSG_BAD_REQUEST "Bad Request"
#define MSG_OK "OK"
#define MSG_CREATED "Created"
#define MSG_INTERNAL_SERVER_ERROR "Internal Server Error"
#define MSG_FORBIDDEN "Forbidden"
#define MSG_REDIRECTED "Redirected"
#define MSG_NOT_ALLOWED "Method Not Allowed"
#define MSG_NOT_FOUND "Not Found"

class HttpRequest
{
private:
    string method;
    string path;
    string queryString;
    map<string, string> headers;
    string body;

public:
    HttpRequest() {}
    HttpRequest(const string &m, const string &p) : method(m), path(p) {}
    // Getters
    string getMethod() const { return method; }
    string getScriptPath() const { return path; }
    string getPathInfo() const; // Extract path after script
    string getQueryString() const { return queryString; }
    string getHeader(const string &name) const;
    string getContentLength() const;
    string getContentType() const;
    string getServerPort() const { return getHeader("SERVER_PORT"); }
    const map<string, string> &getHeaders() const { return headers; }

    // Setters
    void setMethod(const string &m) { method = m; }
    void setPath(const string &p) { path = p; }
    void setQueryString(const string &qs) { queryString = qs; }
    void setBody(const string &b) { body = b; }
    void setContentLength(const string &cl) { addHeader("CONTENT_LENGTH", cl); }
    void setContentType(const string &ct) { addHeader("CONTENT_TYPE", ct); }
    void addHeader(const string &name, const string &value)
    {
        headers[name] = value;
    }
};

struct ResponseInfos
{
    int status;
    string statusMessage;
    map<string, string> headers;
    string body;

    ResponseInfos() : status(OK), statusMessage(MSG_OK), body("") {}
    ResponseInfos(int s, const string &sm, const string &b) : status(s), statusMessage(sm), body(b) {}
    void setHeaders(const map<string, string> &h) { headers = h; }
    int getStatus() const { return status; }
    string getStatusMessage() const { return statusMessage; }
    const map<string, string> &getHeaders() const { return headers; }
};