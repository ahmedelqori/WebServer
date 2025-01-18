/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aes-sarg <aes-sarg@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/28 13:36:03 by mbentahi          #+#    #+#             */
/*   Updated: 2025/01/18 23:11:43 by aes-sarg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Cgi.hpp"
#include <algorithm>

CGI::CGI() : workingDir(""), uploadDir(""), childPid(0)
{
	outputPipe[0] = -1;
	outputPipe[1] = -1;
	inputPipe[0] = -1;
	inputPipe[1] = -1;
}

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

// map<string, string> CGI::createHeader(string output)
// {
// 	map<string, string> header;
// 	istringstream stream(output);
// 	string line;

// 	while (getline(stream, line))
// 	{
// 		if (line.empty())
// 			break;
// 		string::size_type pos = line.find(":");
// 		if (pos != string::npos)
// 		{
// 			string key = line.substr(0, pos);
// 			string value = line.substr(pos + 1);
// 			header[key] = value;
// 		}
// 	}
// 	return header;
// }
void CGI::setupEnvironment(const Request &req)
{
	cout << "Setting up environment variables for CGI script" << endl;

	// Basic CGI variables
	env["REQUEST_METHOD"] = req.getMethod();
	env["SERVER_PROTOCOL"] = req.getVersion();
	env["CONTENT_LENGTH"] = to_string(req.getBody().size());

	// Parse query string from path
	size_t questionMarkPos = req.getPath().find('?');
	if (questionMarkPos != string::npos)
	{
		env["SCRIPT_NAME"] = req.getPath().substr(0, questionMarkPos);
		env["QUERY_STRING"] = req.getPath().substr(questionMarkPos + 1);
	}
	else
	{
		env["SCRIPT_NAME"] = req.getPath();
		env["QUERY_STRING"] = "";
	}
	// env["QUERY_STRING"] = "param1=value1&param2=value2";
	// Add path information
	env["PATH_INFO"] = env["SCRIPT_NAME"];
	env["PATH_TRANSLATED"] = "/home/sultane/Desktop/WebServer/www/post.php"; // You'll need to implement getServerRoot()
	env["SCRIPT_FILENAME"] = env["PATH_TRANSLATED"];

	// Add redirect status for PHP-CGI
	env["REDIRECT_STATUS"] = "200";

	// Process headers
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

	// Debug output
	cout << "CGI Environment Variables:" << endl;
	for (const auto &pair : env)
	{
		cout << pair.first << "=" << pair.second << endl;
	}
	cout << "End of CGI Environment Variables" << endl;
}

// ResponseInfos CGI::execute(const string &script, const string &cgi, const string &requestBody)
// {
// 	if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1)
// 	{
// 		throw CGIException("Error: CGI: Pipe failed");
// 	}

// 	if ((childPid = fork()) == -1)
// 	{
// 		throw CGIException("Error: CGI: Fork failed");
// 	}
// 	if (!childPid)
// 	{
// 		if (dup2(inputPipe[0], STDIN_FILENO) == -1 || dup2(outputPipe[1], STDOUT_FILENO) == -1)
// 			throw CGIException("Error: CGI: Dup2 failed");
// 		close(inputPipe[0]);
// 		close(inputPipe[1]);
// 		close(outputPipe[0]);
// 		close(outputPipe[1]);

// 		string path = script;
// 		if (access(path.c_str(), F_OK) == -1)
// 			throw CGIException("Error: CGI: Script not found");
// 		char *argv[] = {strdup(path.c_str()), NULL};
// 		char **envp = new char *[env.size() + 1];
// 		size_t i = 0;
// 		for (map<string, string>::iterator it = env.begin(); it != env.end(); ++it, ++i)
// 		{
// 			string envVar = it->first + "=" + it->second;
// 			envp[i] = strdup(envVar.c_str());
// 		}
// 		envp[i] = NULL;
// 		execve(cgi.c_str(), argv, envp);
// 		for (size_t j = 0; envp[j] != NULL; ++j)
// 		{
// 			free(envp[j]);
// 		}
// 		delete[] envp;
// 		throw CGIException("Error: CGI: Execve failed");
// 	}
// 	else
// 	{
// 		close(inputPipe[0]);
// 		close(outputPipe[1]);
// 		if (requestBody.empty())
// 		{
// 			close(inputPipe[1]);
// 		}
// 		else
// 		{
// 			ssize_t bytesWritten = write(inputPipe[1], requestBody.c_str(), requestBody.size());
// 			if (bytesWritten == -1)
// 			{
// 				throw CGIException("Error: CGI: Write failed");
// 			}
// 		}
// 		close(inputPipe[1]);
// 	}
// 	return ResponseInfos();
// }

ResponseInfos CGI::execute(const Request request, const string &cgi)
{
	if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1)
	{
		throw CGIException("Error: CGI: Pipe failed");
	}

	if ((childPid = fork()) == -1)
	{
		throw CGIException("Error: CGI: Fork failed");
	}

	setupEnvironment(request);
	if (!childPid)
	{
		// Child process
		if (dup2(inputPipe[0], STDIN_FILENO) == -1 || dup2(outputPipe[1], STDOUT_FILENO) == -1)
			throw CGIException("Error: CGI: Dup2 failed");

		close(inputPipe[0]);
		close(inputPipe[1]);
		close(outputPipe[0]);
		close(outputPipe[1]);

		// Set up args for Python script
		char *argv[] = {
			(char *)cgi.c_str(),					  // Python interpreter path
			(char *)request.getDecodedPath().c_str(), // Script path
			NULL};

		// Convert environment variables to char* array
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

		execve("/usr/bin/php-cgi", argv, envp);

		// Clean up if execve fails
		for (size_t i = 0; envp[i] != NULL; i++)
		{
			free(envp[i]);
		}
		delete[] envp;

		exit(1); // Exit if execve fails
	}
	else
	{
		// Parent process
		close(inputPipe[0]);
		close(outputPipe[1]);
		if (!request.getBody().empty()) {
            ssize_t written = write(inputPipe[1], request.getBody().c_str(), request.getBody().size());
            if (written == -1) {
                cerr << "Failed to write POST data to CGI script" << endl;
            } else {
                cout << "Wrote " << written << " bytes of POST data to CGI script" << endl;
            }
        }

		close(inputPipe[1]);
		// Wait for child process
		cout << "Waiting for child process to finish" << endl;
		int status;
		waitpid(childPid, &status, 0);

		// Check if process exited normally
		if (WIFEXITED(status))
		{
			int exitStatus = WEXITSTATUS(status);
			if (exitStatus != 0)
			{
				cout << "exitStatus: " << exitStatus << endl;
				ResponseInfos response;
				response.setStatus(INTERNAL_SERVER_ERROR); // Set 500 status
				response.setStatusMessage(MSG_INTERNAL_SERVER_ERROR);
				return response;
			}
		}
	}

	ResponseInfos response;
	response.setStatus(OK); // 200 status for successful execution
	response.setStatusMessage("OK");
	return response;
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

ResponseInfos CGI::parseOutput(int outputPipe)
{
	ResponseInfos response;
	string output;
	char buffer[1024];
	ssize_t bytesRead;

	// Read all output from pipe
	while ((bytesRead = read(outputPipe, buffer, sizeof(buffer) - 1)) > 0)
	{
		buffer[bytesRead] = '\0';
		output += buffer;
	}

	// Find the separator between headers and body (double newline)
	size_t headerEnd = output.find("\r\n\r\n");
	if (headerEnd == string::npos)
		headerEnd = output.find("\n\n");

	if (headerEnd != string::npos)
	{
		// Extract headers
		string headers = output.substr(0, headerEnd);
		string body = output.substr(headerEnd + (output[headerEnd] == '\r' ? 4 : 2));

		// Parse headers
		size_t pos = 0;
		size_t nextPos;
		while ((nextPos = headers.find('\n', pos)) != string::npos)
		{
			string line = headers.substr(pos, nextPos - pos);
			if (!line.empty() && line[line.length() - 1] == '\r')
				line = line.substr(0, line.length() - 1);

			// Find the colon in the header
			size_t colon = line.find(':');
			if (colon != string::npos)
			{
				string key = line.substr(0, colon);
				string value = line.substr(colon + 1);

				// Trim whitespace
				while (!value.empty() && isspace(value[0]))
					value = value.substr(1);

				response.addHeader(key, value);
			}
			pos = nextPos + 1;
		}

		// Set the body
		response.setBody(body);
	}
	else
	{
		// No headers found, treat everything as body
		response.setBody(output);
	}

	return response;
}

// // Main function to test CGI class
// int main()
// {
// 	try
// 	{
// 		// Set up working and upload directories (adjust as needed)
// 		string workingDir = "./"; // Assuming your CGI scripts are in './scripts'
// 		string uploadDir = "./";  // Adjust upload directory path

// 		// Create CGI object
// 		CGI cgi(workingDir, uploadDir);

// 		// Create a mock HTTP request (POST for upload)
// 		HttpRequest request("POST", "/home/mbentahi/Desktop/WebServer/HTTP-RESPONSE/upload.cgi");

// 		// Set up the CGI environment
// 		request.setContentLength("20");
// 		request.setContentType("application/octet-stream");

// 		// Execute CGI script with request body (test data)
// 		string script = "/home/mbentahi/Desktop/WebServer/HTTP-RESPONSE/upload.cgi";
// 		string cgi_script = "/usr/bin/php-cgi";
// 		string script_type;
// 		cout << "Enter script type (e.g., php, python): ";
// 		cin >> script_type;
// 		if (script_type == "php")
// 		{
// 			cgi_script = "/usr/bin/php-cgi";
// 		}
// 		else if (script_type == "python")
// 		{
// 			cgi_script = "/usr/bin/python3";
// 		}
// 		string requestBody = "cool job"; // URL-encoded format;
// 		cgi.setupEnvironment(request, requestBody);

// 		ResponseInfos responseInfos = cgi.execute(script, cgi_script, requestBody);
// 		// if (responseInfos.getStatus() != OK)
// 		// 	throw CGIException("Error: CGI execution failed");

// 		// Process the upload

// 		// Get the response from the CGI execution
// 		string response = cgi.getResponse();
// 		// cout << "CGI Response: " << response << endl;

// 		map<string, string> header = cgi.createHeader(response);
// 		responseInfos.setHeaders(header);
// 		cout << "Status: " << responseInfos.getStatus() << endl;
// 		cout << "Status Message: " << responseInfos.getStatusMessage() << endl;
// 		cout << "Headers: " << endl;
// 		for (map<string, string>::iterator it = header.begin(); it != header.end(); ++it)
// 		{
// 			cout << it->first << ": " << it->second << endl;
// 		}
// 	}
// 	catch (const CGIException &e)
// 	{
// 		cerr << "CGI Error :" << e.what() << endl;
// 	}
// 	catch (const exception &e)
// 	{
// 		cerr << "Unexpected Error :" << e.what() << endl;
// 	}
// 	return 0;
// }