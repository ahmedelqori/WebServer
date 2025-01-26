/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/13 20:23:35 by aes-sarg          #+#    #+#             */
/*   Updated: 2025/01/19 21:27:36 by ael-qori         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/ServerUtils.hpp"

ServerUtils::ServerUtils() {}

File_Type ServerUtils::checkResource(const std::string &fullPath)
{
    struct stat pathStat;
    if (stat(fullPath.c_str(), &pathStat) != 0)
        return NOT_EXIST;
    else
    {
        if (S_ISDIR(pathStat.st_mode))
            return DIRECTORY;
        else if (S_ISREG(pathStat.st_mode))
            return REGULAR;
        else
            return UNDEFINED;
    }
}

static std::size_t getFileSize(const std::string& filePath) {
    std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary); // Open the file in binary mode
    if (!file) {
        std::cerr << "Error: Could not open file " << filePath << std::endl;
        return 0; // Return 0 if the file could not be opened
    }

    file.seekg(0, std::ios::end); // Move the file pointer to the end
    std::size_t fileSize = file.tellg(); // Get the position of the pointer (file size)
    file.close(); // Close the file

    return fileSize;
}

ResponseInfos ServerUtils::serveFile(const string &filePath, int code)
{

    if (access(filePath.c_str(), R_OK) != 0)
        return ServerUtils::ressourceToResponse(generateErrorPage(FORBIDEN), FORBIDEN);

    ResponseInfos response;
    response.filePath = filePath;
    response.status = code;
    response.statusMessage = Request::generateStatusMsg(code);
    stringstream ss;
    ss << filePath.length();
    response.headers["Content-Length"] = getFileSize(filePath);
    if (filePath.find_last_of('.') != string::npos)
    {
        string ext = filePath.substr(filePath.find_last_of('.'));
        // response.headers["Connection"] = "keep-alive";
        response.headers["Content-Type"] = getMimeType(ext);
    }
    return response;
}

ResponseInfos ServerUtils::serverRootOrRedirect(RessourceInfo ressource)
{

    if ((ressource.url[ressource.url.length() - 1] != '/' && ressource.url != "/") || !ressource.redirect.empty())
    {
        string redirectUrl = (!ressource.redirect.empty() ? ressource.redirect + "/" : ressource.url + "/");
        return handleRedirect(redirectUrl, REDIRECTED);
    }
    if (!ressource.indexFile.empty())
    {

        string indexPath;
        if (ressource.autoindex)
            indexPath = ressource.root + "/" + ressource.indexFile;
        else
            indexPath = ressource.root + "/" + ressource.url + '/' + ressource.indexFile;
        // cout << "index is " << indexPath << endl;
        struct stat indexStat;
        if (stat(indexPath.c_str(), &indexStat) == 0)
        {
            return ServerUtils::serveFile(indexPath, OK);
        }
    }
    // cout << "Hi 0" << endl;
    if (ressource.autoindex)
        return ServerUtils::generateDirectoryListing(ressource.root + ressource.url);
    return ServerUtils::ressourceToResponse(generateErrorPage(FORBIDEN), FORBIDEN);
}

ResponseInfos ServerUtils::handleRedirect(const string &redirectUrl, int statusCode)
{
    stringstream redirectResponse;
    redirectResponse << "<html><body><h1>" << statusCode << " Redirect</h1><p>Redirecting to: <a href=\"" << redirectUrl << "\">" << redirectUrl << "</a></p></body></html>";

    ResponseInfos infos;
    infos.body = redirectResponse.str();
    infos.headers["Location"] = redirectUrl;
    infos.status = statusCode;
    infos.statusMessage = "Moved permanently";

    return infos;
}

ResponseInfos ServerUtils::generateDirectoryListing(const string &dirPath)
{

    DIR *dir = opendir(dirPath.c_str());
    if (!dir)
    {
        return ressourceToResponse(ServerUtils::generateErrorPage(FORBIDEN), FORBIDEN);
    }

    struct dirent *entry;
    stringstream dirContent;

    dirContent << "<html><body><h1>Directory Listing for " << dirPath << "</h1><ul>";

    while ((entry = readdir(dir)) != NULL)
        dirContent << "<li><a href=\"" << entry->d_name << "\">" << entry->d_name << "</a></li>";

    dirContent << "</ul></body></html>";
    closedir(dir);
    return ressourceToResponse(dirContent.str(), OK);
}

string ServerUtils::getFileExtention(const string type)
{
    std::map<std::string, std::string> mimeTypes;
    mimeTypes["text/html"] = ".html";
    mimeTypes["text/html"] = ".htm";
    mimeTypes["text/css"] = ".css";
    mimeTypes["application/javascript"] = ".js";
    mimeTypes["image/png"] = ".png";
    mimeTypes["image/jpeg"] = ".jpg";
    mimeTypes["image/jpeg"] = ".jpeg";
    mimeTypes["image/gif"] = ".gif";
    mimeTypes["image/svg+xml"] = ".svg";
    mimeTypes["image/x-icon"] = ".ico";
    mimeTypes["audio/mpeg"] = ".mp3";
    mimeTypes["audio/wav"] = ".wav";
    mimeTypes["audio/ogg"] = ".ogg";
    mimeTypes["video/mp4"] = ".mp4";
    mimeTypes["video/webm"] = ".webm";
    mimeTypes["text/plain"] = ".txt";
    mimeTypes["application/json"] = ".json";
    mimeTypes["application/xml"] = ".xml";
    mimeTypes["application/pdf"] = ".pdf";
    mimeTypes["application/zip"] = ".zip";

    if (mimeTypes.find(type) != mimeTypes.end())
        return mimeTypes[type];
    else
        return ".txt";
}

string ServerUtils::getMimeType(const std::string &filePath)
{
    std::map<std::string, std::string> mimeTypes;
    mimeTypes[".html"] = "text/html";
    mimeTypes[".htm"] = "text/html";
    mimeTypes[".css"] = "text/css";
    mimeTypes[".js"] = "application/javascript";
    mimeTypes[".png"] = "image/png";
    mimeTypes[".jpg"] = "image/jpeg";
    mimeTypes[".jpeg"] = "image/jpeg";
    mimeTypes[".gif"] = "image/gif";
    mimeTypes[".svg"] = "image/svg+xml";
    mimeTypes[".ico"] = "image/x-icon";
    mimeTypes[".mp3"] = "audio/mpeg";
    mimeTypes[".wav"] = "audio/wav";
    mimeTypes[".ogg"] = "audio/ogg";
    mimeTypes[".mp4"] = "video/mp4";
    mimeTypes[".webm"] = "video/webm";
    mimeTypes[".txt"] = "text/plain";
    mimeTypes[".json"] = "application/json";
    mimeTypes[".xml"] = "application/xml";
    mimeTypes[".pdf"] = "application/pdf";
    mimeTypes[".zip"] = "application/zip";

    size_t extPos = filePath.find_last_of('.');
    if (extPos != std::string::npos)
    {
        std::string extension = filePath.substr(extPos);
        if (mimeTypes.find(extension) != mimeTypes.end())
        {
            return mimeTypes[extension];
        }
    }
    return "application/octet-stream"; // Default MIME type for binary files
}

ResponseInfos ServerUtils::ressourceToResponse(string ressource, int code)
{
    ResponseInfos response_infos;
    response_infos.body = ressource;
    response_infos.status = code;
    response_infos.statusMessage = Request::generateStatusMsg(code);
    stringstream ss;
    ss << ressource.length();
    response_infos.headers["Content-Length"] = ss.str();

    return response_infos;
}

string ServerUtils::generateErrorPage(int statusCode)
{
    cout << "Hello from generateErrorPage" << endl;
    stringstream errorPage;
    errorPage << "<html><body><h1>" << statusCode << " " << "</h1></body></html>";
    return errorPage.str();
}

bool ServerUtils::isMethodAllowed(const std::string &method, const std::vector<std::string> &allowMethods)
{
    vector<string>::const_iterator it = allowMethods.begin();
    while (it != allowMethods.end())
    {
        if (*it == method)
        {
            return true;
        }
        it++;
    }
    return false;
}

string ServerUtils::generateUniqueString()
{
    static unsigned long counter = 0;
    stringstream ss;
    ss << std::hex << time(NULL) << counter++;
    return ss.str();
}

ostream &operator<<(ostream &os, const ResponseInfos &response)
{
    os << "Status: \n"
       << response.status << " " << response.statusMessage << endl;
    os << "Headers: \n"
       << endl;
    map<string, string>::const_iterator it = response.headers.begin();
    while (it != response.headers.end())
    {
        os << it->first << ": " << it->second << endl;
        it++;
    }

    os << "Body: \n"
       << endl;
    os << response.body << endl;
    return os;
}