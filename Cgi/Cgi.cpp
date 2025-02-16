/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aes-sarg <aes-sarg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/28 13:36:03 by mbentahi          #+#    #+#             */
/*   Updated: 2025/02/16 18:13:36 by aes-sarg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Cgi.hpp"
#include <algorithm>
#include <string>
#include <sys/time.h>

CGI::CGI() : workingDir(""), uploadDir(""), childPid(0)
{
	memset(outputPipe, -1, sizeof(outputPipe));
	memset(inputPipe, -1, sizeof(inputPipe));
	memset(stderrPipe, -1, sizeof(stderrPipe));
}

CGI::CGI(const string &workDir, const string &upDir) : workingDir(workDir), uploadDir(upDir), childPid(0)
{
	memset(outputPipe, -1, sizeof(outputPipe));
	memset(inputPipe, -1, sizeof(inputPipe));
	memset(stderrPipe, -1, sizeof(stderrPipe));

	if (workDir.empty() || upDir.empty())
		throw CGIException("Error: CGI: Invalid working or upload directory");
	if (access(workDir.c_str(), F_OK) == -1)
		throw CGIException("Error: CGI: Working directory does not exist");
	if (access(upDir.c_str(), F_OK) == -1)
		throw CGIException("Error: CGI: Upload directory does not exist");
}

CGI::~CGI()
{
	if (childPid > 0)
	{
		kill(childPid, SIGKILL);
		waitpid(childPid, NULL, 0);
	}
}
void CGI::processUpload(const string &uploadPath)
{
	ofstream uploadFile(uploadPath.c_str(), ios::out | ios::binary);

	if (!uploadFile)
		return;
	char buffer[8192];
	ssize_t bytesRead;

	cout << "uploadPath: " << uploadPath << endl;
	while ((bytesRead = read(outputPipe[0], buffer, sizeof(buffer))) > 0)
	{
		cout << "bytesRead: " << bytesRead << endl;
		cout << "buffer: " << buffer << endl;
		uploadFile.write(buffer, bytesRead);
	}
	uploadFile.close();
}

map<string, string> CGI::createHeader(string output)
{
	(void)output;
	map<string, string> header;
	string line;
	ifstream file("cgi_output.txt");
	if (file.is_open())
	{
		while (getline(file, line))
		{
			if (line.empty())
				break;
			string::size_type pos = line.find(":");
			if (pos != string::npos)
			{
				string key = line.substr(0, pos);
				string value = line.substr(pos + 1);
				header[key] = value;
			}
		}
		file.close();
	}
	else
	{
		throw CGIException("Error: Unable to open file for reading CGI response");
	}
	return header;
}

string to_string(int n)
{
	stringstream ss;
	ss << n;
	return ss.str();
}

void CGI::setupEnvironment(const Request &req)
{
	cout << "Setting up environment variables for CGI script" << endl;

	env["REQUEST_METHOD"] = req.getMethod();
	env["SERVER_PROTOCOL"] = req.getVersion();
	env["SERVER_SOFTWARE"] = "Webserv/1.0";
	env["GATEWAY_INTERFACE"] = "CGI/1.1";
	env["SERVER_NAME"] = "localhost";
	if (req.getMethod() == "POST")
	{
		env["CONTENT_LENGTH"] = to_string(req.getBody().size());
		const map<string, string> &headers = req.getHeaders();
		map<string, string>::const_iterator contentType = headers.find("Content-Type");
		if (contentType != headers.end())
		{
			env["CONTENT_TYPE"] = contentType->second;
		}
	}
	size_t questionMarkPos = req.getPath().find('?');
	env["QUERY_STRING"] = req.getPath().substr(questionMarkPos + 1);

	string queryString;
	map<string, string> queryParams = req.getQueryParams();
	for (map<string, string>::const_iterator it = queryParams.begin(); it != queryParams.end(); ++it)
	{
		if (!queryString.empty())
		{
			queryString += "&";
		}
		queryString += it->first + "=" + it->second;
	}
	env["QUERY_STRING"] = queryString;
	env["SCRIPT_NAME"] = req.getPath();

	env["PATH_INFO"] = env["SCRIPT_NAME"];
	string pathtranslated = "www" + req.getPath();
	env["PATH_TRANSLATED"] = pathtranslated;
	env["SCRIPT_FILENAME"] = env["PATH_TRANSLATED"];

	env["REDIRECT_STATUS"] = "200";

	map<string, string>::const_iterator it1 = req.getHeaders().begin();
	if (it1 != req.getHeaders().end())
	{
		while (it1 != req.getHeaders().end())
		{
			string headerName = it1->first;
			transform(headerName.begin(), headerName.end(), headerName.begin(), ::toupper);
			if (headerName != "CONTENT_LENGTH" && headerName != "CONTENT_TYPE")
			{
				headerName = "HTTP_" + headerName;
			}
			env[headerName] = it1->second;
			it1++;
		}
	}

	map<string, string>::const_iterator cookieIt = req.getHeaders().find("Cookie");
	if (cookieIt != req.getHeaders().end())
	{
		map<string, string> cookies = parseCookies(cookieIt->second);
		string cookieStr;
		for (map<string, string>::const_iterator it = cookies.begin(); it != cookies.end(); ++it)
		{
			if (!cookieStr.empty())
				cookieStr += "; ";
			cookieStr += it->first + "=" + it->second;
		}
		env["HTTP_COOKIE"] = cookieStr;
	}
	// Debug output
	// cout << "CGI Environment Variables:" << endl;
	// for (const auto &pair : env)
	// {
	// 	cout << pair.first << "=" << pair.second << endl;
	// }
	cout << "End of CGI Environment Variables" << endl;
}

string extentionExtractor(string path)
{
	size_t pos = path.find_last_of(".");
	if (pos == string::npos)
		return "";
	return path.substr(pos);
}

ResponseInfos CGI::execute(const Request request, const string &cgi, map<string, string> cgi_info, string root)
{
	string extention = extentionExtractor(cgi);
	string cgi_path = cgi_info[extention];

	setupEnvironment(request);

	pid_t parentPid = getpid();
	struct timeval tv;
	gettimeofday(&tv, NULL);
	long long timestamp = tv.tv_sec * 1000000LL + tv.tv_usec;
	string uniqueSuffix = to_string(parentPid) + "_" + to_string(timestamp);
	inputFileName = "cgi_input_" + uniqueSuffix + ".txt";
	outputFileName = "cgi_output_" + uniqueSuffix + ".txt";
	errorFileName = "cgi_error_" + uniqueSuffix + ".txt";

	if ((childPid = fork()) == -1)
	{
		throw CGIException("Error: CGI: Fork failed");
	}
	if (!childPid)
	{
		FILE *inputFile = freopen(inputFileName.c_str(), "w+b", stdin);
		FILE *outputFile = freopen(outputFileName.c_str(), "w+b", stdout);
		FILE *errorFile = freopen(errorFileName.c_str(), "w+b", stderr);

		if (!inputFile || !outputFile || !errorFile)
		{
			perror("freopen failed");
			exit(1);
		}

		if (!request.getBody().empty())
		{
			fwrite(request.getBody().c_str(), 1, request.getBody().size(), stdin);
			fflush(stdin);
		}

		vector<string> envStrings;
		for (map<string, string>::const_iterator it = env.begin(); it != env.end(); ++it)
		{
			envStrings.push_back(it->first + "=" + it->second);
		}

		char **envp = new char *[envStrings.size() + 1];
		for (size_t i = 0; i < envStrings.size(); i++)
		{
			envp[i] = strdup(envStrings[i].c_str());
		}
		envp[envStrings.size()] = NULL;

		char *argv[] = {
			strdup(cgi_path.c_str()),
			(char *)cgi.c_str(),
			NULL};

		execve(cgi_path.c_str(), argv, envp);
		perror("execve failed");
		for (size_t i = 0; envp[i] != NULL; i++)
		{
			free(envp[i]);
		}
		delete[] envp;

		exit(1);
	}
	else
	{
		int status;
		waitpid(childPid, &status, 0);

		if (WIFEXITED(status))
		{
			int exitStatus = WEXITSTATUS(status);
			if (exitStatus != 0)
			{
				cout << "exitStatus: " << exitStatus << endl;
				ResponseInfos response;
				response.setStatus(INTERNAL_SERVER_ERROR);
				response.setStatusMessage(MSG_INTERNAL_SERVER_ERROR);
				return response;
			}
		}
	}

	cout << "Parsing CGI output" << endl;
	ResponseInfos response;

	try
	{
		string output = getResponse();
		cout << "Output: " << output << endl;
		response = parseOutput(output);
	}
	catch (const exception &e)
	{
		response.setStatus(INTERNAL_SERVER_ERROR);
		response.setBody("CGI Processing Error");
	}
	return response;
}

string CGI::getResponse()
{
	string response;
	char buffer[4096];
	ssize_t bytesRead;

	FILE *errorFile = fopen(errorFileName.c_str(), "rb");
	if (errorFile)
	{
		while ((bytesRead = fread(buffer, 1, sizeof(buffer), errorFile)) > 0)
		{
			response.append(buffer, bytesRead);
		}
		fclose(errorFile);
	}
	else
	{
		cerr << "Error: Unable to open error file: " << errorFileName << endl;
	}

	response.append("\n");

	FILE *outputFile = fopen(outputFileName.c_str(), "rb");
	if (outputFile)
	{
		while ((bytesRead = fread(buffer, 1, sizeof(buffer), outputFile)) > 0)
		{
			response.append(buffer, bytesRead);
			normalOutput.append(buffer, bytesRead);
		}
		fclose(outputFile);
	}
	else
	{
		cerr << "Error: Unable to open output file: " << outputFileName << endl;
	}

	ofstream outFile(outputFileName.c_str());
	if (outFile.is_open())
	{
		outFile << response;
		outFile.close();
	}
	else
	{
		cerr << "Error: Unable to open file for writing CGI response" << endl;
	}

	remove(inputFileName.c_str());
	remove(errorFileName.c_str());
	remove(outputFileName.c_str());

	return response;
}

ResponseInfos CGI::parseOutput(string output)
{
	ResponseInfos response;
	cout << "response string : " << output << endl;
	size_t headerEnd = output.find("\r\n\r\n");

	if (output.find("PHP Warning") != string::npos || output.find("PHP Error") != string::npos)
	{
		response.setStatus(FORBIDEN);
		response.setStatusMessage("PHP Error: " + output.substr(0, output.find('\n')));
	}
	if (headerEnd == string::npos)
		headerEnd = output.find("\n\n");
	if (headerEnd != string::npos)
	{
		string headers = output.substr(0, headerEnd);
		string body = output.substr(headerEnd + (output[headerEnd] == '\r' ? 4 : 2));
		istringstream headerStream(headers);
		string line;

		while (getline(headerStream, line)) {
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);
			size_t colon = line.find(':');
			if (colon != string::npos)
			{
				string key = line.substr(0, colon);
				string value = line.substr(colon + 1);
				value.erase(0, value.find_first_not_of(" \t"));
				if (key == "Set-Cookie")
					response.addHeader("Set-Cookie", value);
				else
					response.addHeader(key, value);
			}
		}
		response.setBody(body);
	}
	else
		response.setBody(output);
	ofstream logFile("cgi.log", ios::app);
	logFile << "CGI Response Status: " << response.getStatus() << endl;
	logFile << "CGI Response Headers: " << endl;
	for (map<string, string>::const_iterator it = response.getHeaders().begin(); it != response.getHeaders().end(); ++it)
	{
		logFile << it->first << ": " << it->second << endl;
	}
	logFile << "CGI Response Body Length: " << response.getBody().length() << endl;
	logFile.close();
	return response;
}

map<string, string> CGI::parseCookies(const string &cookieHeader)
{
	map<string, string> cookies;
	size_t start = 0;
	size_t end;

	while ((end = cookieHeader.find(';', start)) != string::npos)
	{
		string cookie = cookieHeader.substr(start, end - start);
		size_t equal = cookie.find('=');
		if (equal != string::npos)
		{
			string key = cookie.substr(0, equal);
			string value = cookie.substr(equal + 1);
			cookies[key] = value;
		}
		start = end + 1;
	}
	return cookies;
}