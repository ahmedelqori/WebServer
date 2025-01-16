#pragma once
#include "MyType.hpp"
#include "Response.hpp"
#include "Request.hpp"
#include <string>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <vector>
#include <map>

class ServerUtils
{
public:
    ServerUtils();
    static string getMimeType(const std::string &filePath);
    static File_Type checkResource(const std::string &fullPath);
    static ResponseInfos serverRootOrRedirect(RessourceInfo ressource);
    static string generateErrorPage(int statusCode);
    static string getFileExtention(const string type);
    static ResponseInfos ressourceToResponse(string ressource,int statusCode);
    static bool isMethodAllowed(const std::string &method, const std::vector<std::string> &allowMethods);
    static ResponseInfos serveFile(const std::string& filePath,int code);
    static string generateUniqueString();
    static ResponseInfos generateDirectoryListing(const string &dirPath);
    static ResponseInfos handleRedirect(const string &redirectUrl, int statusCode);

};

ostream &operator<<(ostream &os, const ResponseInfos &response);
