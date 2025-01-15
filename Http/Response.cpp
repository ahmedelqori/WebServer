/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aes-sarg <aes-sarg@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/13 20:23:30 by aes-sarg          #+#    #+#             */
/*   Updated: 2025/01/15 20:22:02 by aes-sarg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Response.hpp"


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
    map<string,string>::const_iterator it = headers.begin();
    while (it != headers.end())
    {
        ss << it->first << ": " << it->second << "\r\n";
        it++; 
    }
    ss << "\r\n"
       << body;
    // cout << "hh\r\n" << ss.str() << "hh" << endl;
    return ss.str();
}
