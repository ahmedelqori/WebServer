/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbentahi <mbentahi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/28 13:36:03 by mbentahi          #+#    #+#             */
/*   Updated: 2025/01/05 19:04:04 by mbentahi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Cgi.hpp"

CGI::CGI(const string &workDir, const string &upDir) : workingDir(workDir), uploadDir(upDir), childPid(0)
{
	outputPipe[0] = -1;
	outputPipe[1] = -1;
	inputPipe[0] = -1;
	inputPipe[1] = -1;

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
	map<string, string> header;
	istringstream stream(output);
	string line;

	while (getline(stream, line))
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
	return header;
}
void CGI::setupEnvironment(const HttpRequest &req)
{
	env["REDIRECT_STATUS"] = "200";
	env["REQUEST_METHOD"] = req.getMethod();
	env["SCRIPT_NAME"] = req.getScriptPath();
	env["SCRIPT_FILENAME"] = req.getScriptPath();
	env["SERVER_PROTOCOL"] = "HTTP/1.1";
	env["SERVER_SOFTWARE"] = "YourServer/1.0";
	if (req.getMethod() == "GET")
	{
		cout << "get used\n";
		env["QUERY_STRING"] = req.getQueryString();
	}
	else if (req.getMethod() == "POST")
	{
		env["CONTENT_LENGTH"] = req.getContentLength();
		env["CONTENT_TYPE"] = req.getContentType();
	}
}

ResponseInfos CGI::execute(const string &script, const string &requestBody)
{
	if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1)
	{
		throw CGIException("Error: CGI: Pipe failed");
	}

	if ((childPid = fork()) == -1)
	{
		throw CGIException("Error: CGI: Fork failed");
	}
	if (!childPid)
	{
		if (dup2(inputPipe[0], STDIN_FILENO) == -1 || dup2(outputPipe[1], STDOUT_FILENO) == -1)
			throw CGIException("Error: CGI: Dup2 failed");
		close(inputPipe[0]);
		close(inputPipe[1]);
		close(outputPipe[0]);
		close(outputPipe[1]);

		string path = script;
		if (access(path.c_str(), F_OK) == -1)
			throw CGIException("Error: CGI: Script not found");
		char *argv[] = {strdup(path.c_str()), NULL};
		char **envp = new char *[env.size() + 1];
		size_t i = 0;
		for (map<string, string>::iterator it = env.begin(); it != env.end(); ++it, ++i)
		{
			string envVar = it->first + "=" + it->second;
			envp[i] = strdup(envVar.c_str());
		}
		envp[i] = NULL;
		execve("/usr/bin/php-cgi", argv, envp);
		for (size_t j = 0; envp[j] != NULL; ++j)
		{
			free(envp[j]);
		}
		delete[] envp;
		throw CGIException("Error: CGI: Execve failed");
	}
	else
	{
		close(inputPipe[0]);
		close(outputPipe[1]);
		if (!requestBody.empty())
			write(inputPipe[1], requestBody.c_str(), requestBody.length());
		close(inputPipe[1]);
	}
	return ResponseInfos();
}

string CGI::getResponse()
{
	string response;
	char buffer[4096];
	ssize_t bytesRead;

	fcntl(outputPipe[0], O_NONBLOCK);

	struct timeval tv;
	tv.tv_sec = 30;
	tv.tv_usec = 0;

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(outputPipe[0], &readfds);

	while (select(outputPipe[0] + 1, &readfds, NULL, NULL, &tv) > 0)
	{
		bytesRead = read(outputPipe[0], buffer, sizeof(buffer));
		if (bytesRead > 0)
			response.append(buffer, bytesRead);
		else if (bytesRead == 0)
			break;
	}
	ofstream outFile("cgi_output.txt");
	if (outFile.is_open())
	{
		outFile << response;
		outFile.close();
	}
	else
	{
		cerr << "Error: Unable to open file for writing CGI response" << endl;
	}
	return response;
}

// Main function to test CGI class
int main()
{
	try
	{
		// Set up working and upload directories (adjust as needed)
		string workingDir = "./"; // Assuming your CGI scripts are in './scripts'
		string uploadDir = "./";  // Adjust upload directory path

		// Create CGI object
		CGI cgi(workingDir, uploadDir);

		// Create a mock HTTP request (POST for upload)
		HttpRequest request("POST", "/home/mbentahi/Desktop/WebServer/HTTP-RESPONSE/upload.cgi");

		// Set up the CGI environment
		request.setContentLength("20");
		request.setContentType("application/octet-stream");
		cgi.setupEnvironment(request);

		// Execute CGI script with request body (upload data)
		string script = "/home/mbentahi/Desktop/WebServer/HTTP-RESPONSE/upload.cgi";
		string requestBody = "This is test upload data";

		ResponseInfos responseInfos = cgi.execute(script, requestBody);
		if (responseInfos.getStatus() != OK)
			throw CGIException("Error: CGI execution failed");

		// Process the upload

		// Get the response from the CGI execution
		string response = cgi.getResponse();
		// cout << "CGI Response: " << response << endl;
		

		map<string, string> header = cgi.createHeader(response);
		responseInfos.setHeaders(header);
		cout << "Status: " << responseInfos.getStatus() << endl;
		cout << "Status Message: " << responseInfos.getStatusMessage() << endl;
		cout << "Headers: " << endl;
		for (map<string, string>::iterator it = header.begin(); it != header.end(); ++it)
		{
			cout << it->first << ": " << it->second << endl;
		}
		
	}
	catch (const CGIException &e)
	{
		cerr << "CGI Error :" << e.what() << endl;
	}
	catch (const exception &e)
	{
		cerr << "Unexpected Error :" << e.what() << endl;
	}
	return 0;
}