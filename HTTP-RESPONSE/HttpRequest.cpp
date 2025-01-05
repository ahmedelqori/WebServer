/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbentahi <mbentahi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/28 16:06:53 by mbentahi          #+#    #+#             */
/*   Updated: 2025/01/04 22:47:44 by mbentahi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"

string HttpRequest::getPathInfo() const
{
	return "/usr/bin/php-cgi";
}

string HttpRequest::getHeader(const string &name) const
{
	map<string, string>::const_iterator it = headers.find(name);
	if (it == headers.end())
		return "";
	return it->second;
}

string HttpRequest::getContentLength() const
{
	return getHeader("CONTENT_LENGTH");
}

string HttpRequest::getContentType() const
{
	return getHeader("CONTENT_TYPE");
}