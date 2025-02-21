/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aes-sarg <aes-sarg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/13 20:23:35 by aes-sarg          #+#    #+#             */
/*   Updated: 2025/02/21 12:11:02 by aes-sarg         ###   ########.fr       */
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

static std::size_t getFileSize(const std::string &filePath)
{
    std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
    if (!file)
    {
        return 0;
    }
    file.seekg(0, std::ios::end);
    std::size_t fileSize = file.tellg();
    file.close();
    return fileSize;
}

ResponseInfos ServerUtils::serveFile(const string &filePath, int code)
{

    if (access(filePath.c_str(), F_OK | R_OK) != 0)
        throw FORBIDEN;

    ResponseInfos response;
    response.filePath = filePath;
    response.status = code;
    response.statusMessage = Request::generateStatusMsg(code);
    stringstream ss;
    string s;
    ss << getFileSize(filePath);
    ss >> s;
    response.headers["Content-Length"] = s;
    if (filePath.find_last_of('.') != string::npos)
    {
        string ext = filePath.substr(filePath.find_last_of('.'));
        response.headers["Content-Type"] = getMimeType(ext);
    }
    return response;
}

ResponseInfos ServerUtils::handleRedirect(const string &redirectUrl, int statusCode)
{
    stringstream redirectResponse;
    redirectResponse << "<html><body><h1>" << statusCode << " Redirect</h1><p>Redirecting to: <a href=\"" << redirectUrl << "\">" << redirectUrl << "</a></p></body></html>";

    ResponseInfos infos;
    infos.body = redirectResponse.str();
    infos.headers["Location"] = redirectUrl;
    cout  << "Redirect URL: " << redirectUrl << endl;
    infos.status = statusCode;
    infos.statusMessage = "Moved permanently";
    return infos;
}

ResponseInfos ServerUtils::generateDirectoryListing(const string &dirPath)
{

    DIR *dir = opendir(dirPath.c_str());
    if (!dir)
    {
        throw FORBIDEN;
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
    return "application/octet-stream";
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
    response_infos.headers["Content-Type"] = "text/html";

    return response_infos;
}

string ServerUtils::generateErrorPage(int statusCode)
{
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
