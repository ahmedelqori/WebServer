/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbentahi <mbentahi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/28 13:36:03 by mbentahi          #+#    #+#             */
/*   Updated: 2025/02/23 19:09:58 by mbentahi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Cgi.hpp"
#include <algorithm>
#include <string>

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

	for (size_t i = 0; i < 2; i++)
	{
		if (inputPipe[i] != -1)
			close(inputPipe[i]);
		if (outputPipe[i] != -1)
			close(outputPipe[i]);
		if (stderrPipe[i] != -1)
			close(stderrPipe[i]);
	}
}

string to_string1(int n)
{
	stringstream ss;
	ss << n;
	return ss.str();
}

map<string, string> splitQueryString(string query)
{
	map<string, string> result;
	size_t pos = 0;
	while (pos < query.size())
	{
		size_t end = query.find('&', pos);
		if (end == string::npos)
			end = query.size();
		size_t eq = query.find('=', pos);
		if (eq == string::npos || eq > end)
		{
			pos = end + 1;
			continue;
		}
		result[query.substr(pos, eq - pos)] = query.substr(eq + 1, end - eq - 1);
		pos = end + 1;
	}
	return result;
}

string extentionExtractor(string path)
{
	size_t pos = path.find_last_of(".");
	if (pos == string::npos)
		return "";
	return path.substr(pos);
}


void CGI::setupEnvironment(const Request &req, string root, string cgi, string path)
{
	cout << "Setting up environment variables for CGI script" << endl;

	env["REQUEST_METHOD"] = req.getMethod();
	env["SERVER_PROTOCOL"] = req.getVersion();
	env["SERVER_SOFTWARE"] = "Webserv/1.0";
	env["GATEWAY_INTERFACE"] = "CGI/1.1";
	env["SERVER_NAME"] = "localhost";
	if (req.getMethod() == "POST")
	{
		env["CONTENT_LENGTH"] = to_string1(req.getBody().size());
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
	if (req.getMethod() == "POST")
	{
		queryString += req.getBody();
		map<string, string> postParams = splitQueryString(req.getBody());
		for (map<string, string>::const_iterator it = postParams.begin(); it != postParams.end(); ++it) {
			env[it->first] = it->second;
		}
		for (map<string, string>::const_iterator it = postParams.begin(); it != postParams.end(); ++it) {
			cout << "POST PARAMS: " << it->first << " = " << it->second << endl;
		}
	}
	cout << "Query string: " << queryString << endl;
	env["QUERY_STRING"] = queryString;
	env["SCRIPT_NAME"] = path;

	env["PATH_INFO"] = env["SCRIPT_NAME"];
	(void)root;
	string pathtranslated = cgi;
	cout << "Path translated: " << pathtranslated << endl;
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
	cout << "CGI Environment Variables:" << endl;
	// for (const auto &pair : env)
	// {
	// 	cout << pair.first << "=" << pair.second << endl;
	// }
	cout << "End of CGI Environment Variables" << endl;
}

string generateRandomName()
{
	string name = "tmp_";
	time_t t;
	srand((unsigned)time(&t));
	for (int i = 0; i < 10; i++)
	{
		name += 'a' + rand() % 26;
	}
	return name;
}

ResponseInfos CGI::execute(const Request request, string &cgi, map<string, string> cgi_info, string root)
{
	ResponseInfos response;

	string extention = extentionExtractor(cgi);
	string cgi_path = cgi_info[extention];
	string path = cgi;
	int pid;
	cgi = string(root) + string(cgi);
	setupEnvironment(request, root, cgi, path);
	
	outputFile = "/tmp/output" + generateRandomName();
	inputFile = "/tmp/" + generateRandomName();

	    // Open input file before forking to ensure it exists
    fstream input(inputFile.c_str(), ios::out | ios::binary | ios::trunc);
    if (!input.is_open())
    {
        cerr << "Error: Unable to open file for CGI input: " << inputFile << endl;
        throw CGIException("Error: CGI: Unable to open file for writing CGI input");
    }

    if (request.getMethod() == "POST" && !request.getBody().empty())
    {
        input << request.getBody();
        input.flush(); // Ensure data is fully written before exec
    }
    input.close();
	
	if ((pid = fork()) == -1)	throw CGIException("Error: CGI: Fork failed");
	if (!pid)
	{
		freopen(outputFile.c_str(), "w+", stdout);
		freopen(inputFile.c_str(), "w+", stdin);
		
				vector<string> envStrings;
		for (map<string, string>::const_iterator it = env.begin(); it != env.end(); ++it)
		{
			envStrings.push_back(it->first + "=" + it->second);
		}

		char **envp = new char *[envStrings.size() + 1];
		for (size_t i = 0; i < envStrings.size(); i++)
		{
			envp[i] = const_cast<char*>(envStrings[i].c_str());
		}
		envp[envStrings.size()] = NULL;

		char *argv[] = {
			const_cast<char*>(cgi_path.c_str()),
			const_cast<char*>(cgi.c_str()),
			NULL
		};

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
		// if (request.getMethod() == "POST" && !request.getBody().empty())
		// {
		// 	cout << "Writing POST body to CGI input file" << endl;
		// 	fstream input(inputFile.c_str(), ios::out | ios::binary);
		// 	if (!input.is_open())
		// 	{
		// 		cerr << "Error: Unable to open file for writing CGI input: " << inputFile << endl;
		// 		throw CGIException("Error: CGI: Unable to open file for writing CGI input");
		// 	}
		// 	input << request.getBody();
		// 	input.close();
		// }
    	response.isCgi = 1;                 
    	response.cgiPid = pid;               
    	response.cgiStartTime = time(NULL);  
    	response.cgiOutputFile = outputFile;
    	response.cgiInputFile = inputFile;
	}
	cout << "CGI execution complete" << endl;
	cout << response << endl;
	return response;
}



string CGI::getResponse(string output)
{
    string response;
    char buffer[4096];
    ssize_t bytesRead;

    ifstream inFile(output.c_str(), ios::in | ios::binary);
    if (!inFile.is_open())
    {
        cerr << "Error: Unable to open file for reading CGI response: " << outputFile << endl;
		if (remove(output.c_str()) != 0 || remove(inputFile.c_str()) != 0 || remove(outputFile.c_str()) != 0)
        	cerr << "Error removing inoutput file: " << strerror(errno) << endl;
        return response;
    }

    while ((bytesRead = inFile.readsome(buffer, sizeof(buffer)))){
        response.append(buffer, bytesRead);
    }

    inFile.close();
 

    return response;
}

ResponseInfos CGI::parseOutput(string output)
{
	ResponseInfos response;
	cout << "response string : " << output << endl;
	size_t headerEnd = output.find("\r\n\r\n");

	// if (output.find("PHP Warning") != string::npos || output.find("PHP Error") != string::npos)
	// {
	// 	response.setStatus(FORBIDEN);
	// 	response.setStatusMessage("PHP Error: " + output.substr(0, output.find('\n')));
	// }
	if (headerEnd == string::npos)
		headerEnd = output.find("\n\n");
	if (headerEnd != string::npos)
	{
		string headers = output.substr(0, headerEnd);
		string body = output.substr(headerEnd + (output[headerEnd] == '\r' ? 4 : 2));
		istringstream headerStream(headers);
		string line;

		while (getline(headerStream, line))
		{
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
