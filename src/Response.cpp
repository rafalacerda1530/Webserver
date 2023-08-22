#include "Response.hpp"


std::string itoa(int num) {
	std::ostringstream oss;
	oss << num;
	return oss.str();
}

std::string Response::checkLocationRoot(const std::vector<std::string>& webPathSegments, std::map <std::string, std::string>& location, Server &web) {
    std::string currentPath;
    std::string matchingLocationPath;

    currentPath += "root /";
    {
        std::map<std::string, std::string>::const_iterator locationPathIterator = location.find(currentPath);
        if (locationPathIterator != location.end()) {
            matchingLocationPath = locationPathIterator->second;
            web.locationPath = currentPath.substr(5, currentPath.length() - 5);
        }
    }
    for (std::vector<std::string>::const_iterator segmentIterator = webPathSegments.begin(); segmentIterator != webPathSegments.end(); ++segmentIterator) {
        currentPath += *segmentIterator;
        std::map<std::string, std::string>::const_iterator locationPathIterator = location.find(currentPath);
        if (locationPathIterator != location.end()) {
            matchingLocationPath = locationPathIterator->second;
            web.locationPath = currentPath.substr(5, currentPath.length() - 5);
        }
        currentPath += "/";
    }

    return (matchingLocationPath);
}

std::vector<std::string> Response::splitPath(const std::string& path, char delimiter) {
    std::vector<std::string> segments;
    std::stringstream ss(path);
    std::string segment;
    while (std::getline(ss, segment, delimiter)) {
        if (!segment.empty()) {
            segments.push_back(segment);
        }
    }
    return (segments);
}

std::string  Response::findLocationRoot(Server &web, std::string RequestPathResource){
    std::map<std::string, std::map<std::string, std::string> >::iterator outerIt;

    for (outerIt = web.locationMap.begin(); outerIt != web.locationMap.end(); ++outerIt){
        if (outerIt->first == "Server " + web.hostMessageReturn){
            web.pathSegments = splitPath(RequestPathResource, '/');
            web.locationRoot = checkLocationRoot(web.pathSegments, outerIt->second, web);
            if (web.locationRoot.empty()){
                std::map<std::string, std::map<std::string, std::string> >::iterator serverIt;
                for (serverIt = web.serverMap.begin(); serverIt != web.serverMap.end(); ++serverIt){
                    if (serverIt->first == "Server " + web.hostMessageReturn)
                        web.locationRoot = serverIt->second["root"];
                }
            }
            RequestPathResource = web.locationRoot + RequestPathResource.substr(web.locationPath.length(), RequestPathResource.length() - web.locationPath.length());
        }
    }
    return(RequestPathResource);
}

std::string  Response::getResponseFile(std::string responseRequestFilePath, Server &web, std::string RequestPathResource){
    addIndex(web, responseRequestFilePath);
    std::ifstream file(responseRequestFilePath.c_str());
    std::string content;
    std::string response;
    DIR* directory = opendir(responseRequestFilePath.c_str());

    if (file.is_open() && !directory){
        std::string line;
        while(std::getline(file, line)){
            content += line;
        }
        file.close();
    }
    else
    {
        std::map<std::string, std::map<std::string, std::string> >::iterator serverIt;
        for (serverIt = web.serverMap.begin(); serverIt != web.serverMap.end(); ++serverIt)
        {
            if (serverIt->first == "Server " + web.hostMessageReturn)
            {
                if (web.locationRoot != serverIt->second["root"])
                {
                    web.locationRoot = serverIt->second["root"];
                    responseRequestFilePath = serverIt->second["root"] + RequestPathResource;
                    closedir(directory);
                    return (getResponseFile(responseRequestFilePath, web, RequestPathResource));
                }
            }
        }
        web.pathSegments.clear();
        web.locationPath.clear();
        web.locationRoot.clear();
        closedir(directory);
        std::vector<std::string>().swap(web.pathSegments);
        return("Error 404");
    }
    response = createResponseMessage(content);
    web.pathSegments.clear();
    web.locationPath.clear();
    web.locationRoot.clear();
    closedir(directory);
    std::vector<std::string>().swap(web.pathSegments);
    return (response);
}

std::string  Response::responseRequest(Server &web, std::string RequestPathResource){
    std::string fullRequestPathResource;

    fullRequestPathResource = findLocationRoot(web, RequestPathResource);
    std::map<std::string, std::string> keyValueMap;
    std::string response;

    response = getResponseFile(fullRequestPathResource, web, RequestPathResource);
    return(response);
}

void Response::addIndex(Server &web, std::string &path){
    std::string currentPath;
    std::string matchingLocationPath;
    std::string fileToReturn = "index.html";
    std::string temp;

    if (web.locationRoot != web.getItemFromServerMap(web, "Server " + web.hostMessageReturn, "root")){
        temp = web.getItemFromLocationMap(web, "Server " + web.hostMessageReturn, "index " + web.locationPath);
        if (temp != "wrong")
            fileToReturn = temp;
    }
    else{
        fileToReturn = web.getItemFromServerMap(web, "Server " + web.hostMessageReturn, "index");
        if (fileToReturn == "wrong")
            fileToReturn = "index.html";
    }
    std::vector<std::string> files = splitPath(fileToReturn, ' ');
    DIR* directory = opendir(path.c_str());
    if(directory){
        struct dirent* entry;
        while ((entry = readdir(directory)) != NULL){
            for (std::vector<std::string>::const_iterator filesIterator = files.begin(); filesIterator != files.end(); ++filesIterator) {
                if (*filesIterator == entry->d_name){
                    path = path + "/" + entry->d_name;
                    closedir(directory);
                    return ;
                }
            }
        }
        closedir(directory);
        return ;
    }
    else{
        closedir(directory);
        return ;
    }
}

std::string  Response::createResponseMessage(std::string body){
    std::string body_size = itoa(body.size() + 1);
    std::string response = "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/html\r\n"
                            "Content-Length: " + body_size + "\r\n"
                            "\r\n"
                            + body + "\n";
    return(response);
}