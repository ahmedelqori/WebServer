/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbentahi <mbentahi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/28 13:36:03 by mbentahi          #+#    #+#             */
/*   Updated: 2025/01/06 12:26:43 by mbentahi         ###   ########.fr       */
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
void CGI::setupEnvironment(const HttpRequest &req,const string &requestBody)
{
	env.clear();  // Clear any existing environment variables
    (void)requestBody;
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_SOFTWARE"] = "YourServer/1.0";
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["REQUEST_METHOD"] = "POST";
    env["SCRIPT_FILENAME"] = req.getScriptPath();
    env["SCRIPT_NAME"] = req.getScriptPath();
    env["PATH_INFO"] = "";
    env["PATH_TRANSLATED"] = req.getScriptPath();
    env["CONTENT_TYPE"] = "application/x-www-form-urlencoded";
    env["CONTENT_LENGTH"] = req.getContentLength();
    env["REDIRECT_STATUS"] = "200";

    // Add any query string if present
    if (!req.getQueryString().empty()) {
        env["QUERY_STRING"] = req.getQueryString();
    }
	// env["REDIRECT_STATUS"] = "200";
	// env["REQUEST_METHOD"] = req.getMethod();
	// env["SCRIPT_NAME"] = req.getScriptPath();
	// env["SCRIPT_FILENAME"] = req.getScriptPath();
	// env["SERVER_PROTOCOL"] = "HTTP/1.1";
	// env["SERVER_SOFTWARE"] = "YourServer/1.0";
	// if (req.getMethod() == "GET")
	// {
	// 	cout << "get used\n";
	// 	env["QUERY_STRING"] = req.getQueryString();
	// }
	// else if (req.getMethod() == "POST")
	// {
	// 	env["CONTENT_LENGTH"] = requestBody.size();
	// 	env["CONTENT_TYPE"] = req.getContentType();
	// }
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

ResponseInfos CGI::execute(const string &script, const string &cgi, const string &requestBody)
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
        // Child process
        if (dup2(inputPipe[0], STDIN_FILENO) == -1 || dup2(outputPipe[1], STDOUT_FILENO) == -1)
            throw CGIException("Error: CGI: Dup2 failed");
            
        close(inputPipe[0]);
        close(inputPipe[1]);
        close(outputPipe[0]);
        close(outputPipe[1]);

        // Set up args for Python script
        char *argv[] = {
            (char*)cgi.c_str(),  // Python interpreter path
            (char*)script.c_str(),  // Script path
            NULL
        };

        // Convert environment variables to char* array
        vector<string> envStrings;
		for (map<string, string>::const_iterator it = env.begin(); it != env.end(); ++it) {
			envStrings.push_back(it->first + "=" + it->second);
		}
        char **envp = new char*[envStrings.size() + 1];
        for (size_t i = 0; i < envStrings.size(); i++) {
            envp[i] = strdup(envStrings[i].c_str());
        }
        envp[envStrings.size()] = NULL;

        execve(argv[0], argv, envp);
        
        // Clean up if execve fails
        for (size_t i = 0; envp[i] != NULL; i++) {
            free(envp[i]);
        }
        delete[] envp;
        
        exit(1);  // Exit if execve fails
    }
    else
    {
        // Parent process
        close(inputPipe[0]);
        close(outputPipe[1]);

        if (!requestBody.empty())
        {
            // Format the POST data properly
            string formattedBody = "data=" + requestBody;
            write(inputPipe[1], formattedBody.c_str(), formattedBody.length());
        }
        
        close(inputPipe[1]);

        // Wait for child process
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
                response.setStatus(INTERNAL_SERVER_ERROR);  // Set 500 status
                response.setStatusMessage(MSG_INTERNAL_SERVER_ERROR);
                return response;
            }
        }
    }
    
    ResponseInfos response;
    response.setStatus(OK);  // 200 status for successful execution
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

		// Execute CGI script with request body (test data)
		string script = "/home/mbentahi/Desktop/WebServer/HTTP-RESPONSE/upload.cgi";
		string cgi_script = "/usr/bin/php-cgi";
		string script_type;
		cout << "Enter script type (e.g., php, python): ";
		cin >> script_type;
		if (script_type == "php")
		{
			cgi_script = "/usr/bin/php-cgi";
		}
		else if (script_type == "python")
		{
			cgi_script = "/usr/bin/python3";
		}
		string requestBody = "cool job";  // URL-encoded format;
		cgi.setupEnvironment(request, requestBody);

		ResponseInfos responseInfos = cgi.execute(script, cgi_script, requestBody);
		// if (responseInfos.getStatus() != OK)
		// 	throw CGIException("Error: CGI execution failed");

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