#include "Response.hpp"


Response::Response() : status_code(200), status_message("OK") {}

void Response::setStatus(int code, const string &message)
{
    status_code = code;
    status_message = message;
}

void Response::addHeader(const string &name, const string &value)
{
    headers[name] = value;
}

void Response::setBody(const string &body_content)
{
    body = body_content;
}

string Response::getResponse() const
{
    stringstream ss;
    ss << "HTTP/1.1 " << status_code << " " << status_message << "\r\n";
    for (const auto &header : headers)
    {
        ss << header.first << ": " << header.second << "\r\n";
    }
    ss << "\r\n"
       << body;
    // cout << "hh\r\n" << ss.str() << "hh" << endl;
    return ss.str();
}
