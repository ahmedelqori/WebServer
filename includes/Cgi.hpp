/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbentahi <mbentahi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/28 11:26:31 by mbentahi          #+#    #+#             */
/*   Updated: 2025/02/27 10:41:26 by mbentahi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
#define CGI_HPP

#include "MyType.hpp"
#include "Request.hpp"
#include "RequestHandler.hpp"
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <string.h>
#include <fstream>
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
#include <sstream>

#define CGI_TIMEOUT 5

class CGI
{
private:
	string workingDir;
	string uploadDir;
	map<string, string> env;

	pid_t childPid;

public:
	int counter;
	string inputFile;
	string outputFile;
	string normalOutput;
	void handleChunkedTransfer();
	void processUpload(const string &uploadPath);
	void setupEnvironment(const Request &req,string root, string cgi,string);
	ResponseInfos parseOutput(string output);
	string getErrorResponse();
	CGI(const string &workDir, const string &upDir);
	CGI();
	~CGI();

	ResponseInfos execute(const Request request,  string &cgi, map<string , string> cgi_info,string root);
	string getResponse(string);
	map<string, string> createHeader(string output);
	map<string, string> parseCookies(const string& cookieHeader);
};

class CGIException : public exception
{
private:
	string message;

public:
	CGIException(const string &msg) : message(msg) {}

	virtual ~CGIException() throw() {} // Match the base class specification

	virtual const char *what() const throw() { return message.c_str(); }
};

#endif
