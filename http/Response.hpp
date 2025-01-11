#pragma once

#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include "../utils/MyType.hpp"

using namespace std;

class Response {
public:
    Response();
    void setStatus(int status_code, const string& status_message);
    void addHeader(const string& name, const string& value);
    void setBody(const string& body);
    string getResponse() const;

private:
    int status_code;
    string status_message;
    map<string, string> headers;
    string body;
};

