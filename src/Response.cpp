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


std::string returnContenType(std::string contentType){
    std::ifstream file("./utils/contentType.txt");
    std::vector<std::string> contentTypes;
    if (!file) {
        std::cerr << "Erro ao abrir o arquivo." << std::endl;
        return "text/plain";
    }
    std::string line;
    while (std::getline(file, line)) {
        contentTypes.push_back(line);
    }
    file.close();

    for (std::vector<std::string>::iterator it = contentTypes.begin(); it != contentTypes.end(); ++it) {
        size_t pos = it->find(contentType);
        if (pos != std::string::npos)
            return(*it);
    }

    return ("text/plain");
}

std::string  Response::findLocationRoot(Server &web, std::string RequestPathResource){
    std::map<std::string, std::map<std::string, std::string> >::iterator outerIt;

    for (outerIt = web.locationMap.begin(); outerIt != web.locationMap.end(); ++outerIt){
        if (outerIt->first == "Server " + web.hostMessageReturn){
            if (web.pathSegments.empty()){
                size_t  findContentTypeReturn;
                std::string type;
                web.contentType = "";
                web.pathSegments = splitPath(RequestPathResource, '/');
                if (!web.pathSegments.empty()){
                    findContentTypeReturn = web.pathSegments[web.pathSegments.size() - 1].rfind(".");
                    if (findContentTypeReturn != std::string::npos){
                        type = web.pathSegments[web.pathSegments.size() - 1].substr(findContentTypeReturn + 1);
                        web.contentType = returnContenType(type);
                    }
                }
            }
            if (web.locationRoot.empty())
                web.locationRoot = checkLocationRoot(web.pathSegments, outerIt->second, web);
            if (web.locationRoot.empty()){
                std::map<std::string, std::map<std::string, std::string> >::iterator serverIt;
                for (serverIt = web.serverMap.begin(); serverIt != web.serverMap.end(); ++serverIt){
                    if (serverIt->first == "Server " + web.hostMessageReturn)
                        web.locationRoot = serverIt->second["root"];
                }
            }
            if (web.locationPath != "/")
                RequestPathResource = web.locationRoot + RequestPathResource.substr(web.locationPath.length(), RequestPathResource.length() - web.locationPath.length());
            else
                RequestPathResource = web.locationRoot + RequestPathResource.substr(0, RequestPathResource.length());
        }
    }
    return(RequestPathResource);
}

std::string listDirectoriesAsButtons(Server &web, std::string& directoryPath) {
    std::string responseDirectory;

    responseDirectory += "<html><head><title>Directory</title></head><body>";

    DIR* dir;
    struct dirent* entry;
    if ((dir = opendir(directoryPath.c_str())) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (std::string(entry->d_name) != "." && std::string(entry->d_name) != "..") {
                if (web.getPathResource != "/"){
                    responseDirectory = responseDirectory + "<form action=\"" + web.getPathResource + "/" + entry->d_name + "\">"
                       + "<input type=\"submit\" value=\"" + entry->d_name + "\"/>"
                        + "</form>";
                }
                else{
                    responseDirectory = responseDirectory + "<form action=\" ./" + entry->d_name + "\">"
                        + "<input type=\"submit\" value=\"" + entry->d_name + "\"/>"
                        + "</form>";
                }
            }
        }
        closedir(dir);
    }
    else{
        return("");
    }

    responseDirectory += "</body></html>";

    return(responseDirectory);
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
                    size_t  findContentType;
                    path = path + "/" + entry->d_name;
                    temp = entry->d_name;
                    findContentType = temp.rfind(".");
                    if (findContentType != std::string::npos)
                        web.contentType = returnContenType(temp.substr(findContentType + 1));
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

std::string  Response::getResponseFile(std::string responseRequestFilePath, Server &web, std::string RequestPathResource){
    if (web.autoindex)
    {
        std::string directoryReturn = listDirectoriesAsButtons(web, responseRequestFilePath);
        if (directoryReturn != ""){
            web.contentType = "html";
            return(createResponseMessage(web, directoryReturn));
        }
    }
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
        closedir(directory);
        std::vector<std::string>().swap(web.pathSegments);
        return("Error 404");
    }
    response = createResponseMessage(web, content);
    closedir(directory);
    std::vector<std::string>().swap(web.pathSegments);
    return (response);
}

std::string  Response::responseRequest(Server &web, std::string RequestPathResource, std::string &header){
    std::string fullRequestPathResource;

    fullRequestPathResource = findLocationRoot(web, RequestPathResource);
    if(web.containsCgi)
        return(web.cgi.handleCgi(fullRequestPathResource, web, header));
    std::map<std::string, std::string> keyValueMap;
    std::string response;
    response = getResponseFile(fullRequestPathResource, web, RequestPathResource);
    return(response);
}

std::string  Response::createResponseMessage(Server &web, std::string body){
    std::string body_size = itoa(body.size() + 1);
    std::string response = "HTTP/1.1 200 OK\r\n"
                            "Content-Type: "+web.contentType+"\r\n"
                            "Content-Length: " + body_size + "\r\n"
                            "\r\n"
                            + body + "\n";
    return(response);
}


std::string Response::createResponseMessageWithError(std::string body, std::string erro, std::string messageErro){
    std::string body_size = Request::itoa(body.size() + 1);
    std::string response = "HTTP/1.1 "+ erro +" "+messageErro+"\r\n"
                            "Content-Type: text/html\r\n"
                            "Content-Length: " + body_size + "\r\n"
                            "\r\n"
                            + body + "\n";
    return(response);
}


std::string  getResponseFileDefault(std::string responseFileDefault){
    std::ifstream file(responseFileDefault.c_str());
    std::string content;
    std::string response;
    DIR* directory = opendir(responseFileDefault.c_str());

    if (file.is_open() && !directory){
        std::string line;
        while(std::getline(file, line)){
            content += line;
        }
        file.close();
    }
    else
        return("Error 404");
    return (response);
}

std::string Response::deleteResponse(Server &web, std::string pathToDelete){
    std::string rootPath = findLocationRoot(web, pathToDelete);
    const char* filename = rootPath.c_str();
    std::string response;

    if (access(filename, F_OK) != -1){
        std::cout << "Arquivo no caminho " << rootPath << " Localizado." << std::endl;
        if (std::remove(filename) == 0) {
            std::cout << "Arquivo no caminho " << rootPath << " deletado." << std::endl;
            std::string body = getResponseFileDefault("./utils/deleteSuccess.html");
            response = Response::createResponseMessage(web, body);
            return(response);
        } else {
            response = "Error 403";
            return(response);
        }
    }
    else
    {
        std::cout << "O arquivo não existe." << std::endl;
        response = "Error 404";
        std::cout << response << std::endl;
    }
    return(response);
}

std::string getErrorReturn(std::string path, std::string errorNum, Server &web){
    std::string errorPage;
    std::string content = "";
    std::ifstream fileToOpen;

    errorPage = web.getItemFromServerMap(web, "Server " + web.hostMessageReturn, "error_page " + errorNum);
    if (errorPage != "wrong"){
        std::ifstream filetoOpen(errorPage.c_str());
        if (filetoOpen.is_open()){
            std::string line;
            while(std::getline(filetoOpen, line)){
                content += line;
            }
            filetoOpen.close();
        }
    }
    if (content == "")
    {
        std::ifstream fileToOpenDefault(path.c_str());
        if (fileToOpenDefault.is_open()){
            std::string line;
            while(std::getline(fileToOpenDefault, line)){
                content += line;
            }
            fileToOpenDefault.close();
        }
    }
    return(content);
}



std::string Response::errorType(std::string erro, Server &web){
    std::string body;
    std::string content;


    if (erro.substr(0, 5) == "Error")
        web.connection = "close";
    if (erro == "Error 404"){
        body = getErrorReturn("./utils/error_page/404.html", "404", web);
        content = Response::createResponseMessageWithError(body, "404", "Not Found");
        return (content);
    }
    else if(erro == "Error 403"){
        body = getErrorReturn("./utils/error_page/403.html", "403", web);
        content = Response::createResponseMessageWithError(body, "403", "Forbidden");
        return (content);
    }
    else if(erro == "Error 400"){
        body = getErrorReturn("./utils/error_page/400.html", "400", web);
        content = Response::createResponseMessageWithError(body, "400", "Bad Request");
        return (content);
    }
    else if(erro == "Error 411"){
        body = getErrorReturn("./utils/error_page/411.html", "411", web);
        content = Response::createResponseMessageWithError(body, "411", "Length Required");
        return (content);
    }
    else if(erro == "Error 413"){
        body = getErrorReturn("./utils/error_page/413.html", "413", web);
        content = Response::createResponseMessageWithError(body, "413", "Request Entity too Large");
        return (content);
    }
    else if(erro == "Error 415"){
        body = getErrorReturn("./utils/error_page/415.html", "415", web);
        content = Response::createResponseMessageWithError(body, "415", "Unsupported Media Type");
        return (content);
    }
    else if(erro == "Error 405"){
        body = getErrorReturn("./utils/error_page/405.html", "405", web);
        content = Response::createResponseMessageWithError(body, "405", "Method Not Allowed");
        return (content);
    }
    else if(erro == "Error 500"){
        body = getErrorReturn("./utils/error_page/500.html", "500", web);
        content = Response::createResponseMessageWithError(body, "500", "Internal Server Error");
        return (content);
    }
    else{
        content = "OK";
        return (content);
    }
    return(content);
}
