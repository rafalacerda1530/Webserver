/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Post.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mtomomit <mtomomit@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/08/25 20:02:57 by mtomomit          #+#    #+#             */
/*   Updated: 2023/09/07 16:57:17 by mtomomit         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Post.hpp"

void Post::getContentTypeData(std::string &header)
{
    std::size_t findContent;
    std::size_t findBoundary;

    findContent = header.find("Content-Type: ");
    if (findContent != std::string::npos)
        contentType = header.substr(findContent);
    else
        throw UnsupportedMediaType();
    contentType = contentType.substr(0, contentType.find("\r\n"));
    findBoundary = contentType.find("boundary=");
    if (findBoundary != std::string::npos){
        mainBoundary = contentType.substr(findBoundary);
        contentType = contentType.substr(14, findBoundary - 16);
        mainBoundary = mainBoundary.substr(9, mainBoundary.length() - 9);
    }
    else{
        mainBoundary = "";
        contentType = contentType.substr(14);
    }
    if (contentType == "multipart/form-data" && mainBoundary == "")
        throw BadRequest();
}

void Post::getLength(std::string header, Server &web)
{
    std::size_t findLength;
    std::string stringLength;
    std::string clientMaxBodySize;

    findLength = header.find("Content-Length: ");
    if (findLength != std::string::npos)
        stringLength = header.substr(findLength);
    else
        throw LengthRequired();
    stringLength = stringLength.substr(0, stringLength.find("\r\n"));
    stringLength = stringLength.substr(16);
    contentLength = atol(stringLength.c_str());
    if (web.locationPath.empty())
        clientMaxBodySize = web.getItemFromServerMap(web, "Server " + web.hostMessageReturn, "client_max_body_size");
    else
        clientMaxBodySize = web.getItemFromLocationMap(web, "Server " + web.hostMessageReturn, "client_max_body_size " + web.locationPath);
    if (clientMaxBodySize != "wrong")
    {
        if (contentLength > static_cast<long unsigned int>(atol(clientMaxBodySize.c_str())))
            throw RequestEntityTooLarge();
    }
}

void    initializeContentDispositionMarker(std::vector<char> &contentDispositionMarker){
    contentDispositionMarker.push_back('C');
    contentDispositionMarker.push_back('o');
    contentDispositionMarker.push_back('n');
    contentDispositionMarker.push_back('t');
    contentDispositionMarker.push_back('e');
    contentDispositionMarker.push_back('n');
    contentDispositionMarker.push_back('t');
    contentDispositionMarker.push_back('-');
    contentDispositionMarker.push_back('D');
    contentDispositionMarker.push_back('i');
    contentDispositionMarker.push_back('s');
    contentDispositionMarker.push_back('p');
    contentDispositionMarker.push_back('o');
    contentDispositionMarker.push_back('s');
    contentDispositionMarker.push_back('i');
    contentDispositionMarker.push_back('t');
    contentDispositionMarker.push_back('i');
    contentDispositionMarker.push_back('o');
    contentDispositionMarker.push_back('n');
    contentDispositionMarker.push_back(':');
    contentDispositionMarker.push_back(' ');
}

void Post::getBoundaryHeaderData(std::vector<char> &body, std::size_t &bytesReadTotal, std::string &fullRequestPathResource)
{
    std::size_t findContentDisposition;
    std::size_t findFilename;
    std::string header;
    char        buffer[2];
    int         bytesRead;
    std::vector<char>::iterator headerEnd;

    std::vector<char> doubleCRLF;
    doubleCRLF.push_back('\r');
    doubleCRLF.push_back('\n');
    doubleCRLF.push_back('\r');
    doubleCRLF.push_back('\n');
    headerEnd = std::search(body.begin(), body.end(), doubleCRLF.begin(), doubleCRLF.end());
    while (headerEnd == body.end())
    {
        bytesRead = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        bytesReadTotal += bytesRead;
        buffer[bytesRead] = 0;
        body.insert(body.end(), buffer, buffer + bytesRead);
        headerEnd = std::search(body.begin(), body.end(), doubleCRLF.begin(), doubleCRLF.end());
    }
    std::vector<char> contentDispositionMarker;
    initializeContentDispositionMarker(contentDispositionMarker);
    findContentDisposition = std::search(body.begin(), body.end(), contentDispositionMarker.begin(), contentDispositionMarker.end()) - body.begin();
    if (findContentDisposition == body.size())
        throw BadRequest();
    contentDisposition = std::string(body.begin() + findContentDisposition, body.end());
    contentDisposition = contentDisposition.substr(0, contentDisposition.find("\r\n"));
    findFilename = contentDisposition.find("filename=");
    if (findFilename != std::string::npos)
    {
        filename = contentDisposition.substr(findFilename + 10);
        filename = filename.substr(0, filename.find("\""));
        std::ofstream file((fullRequestPathResource + filename).c_str(), std::ios::out | std::ios::binary);
        file.close();
    }
    else{
        filename = "";
        std::ofstream file((fullRequestPathResource + "file").c_str(), std::ios::out | std::ios::binary);
        file.close();
    }
    body = std::vector<char>(headerEnd + 4, body.end());
}
void Post::copyToFile(const std::string &fullRequestPathResource, std::size_t limiter, std::vector<char> &body)
{
    if (filename != ""){
        std::ofstream file((fullRequestPathResource + filename).c_str(), std::ios::out | std::ios::binary | std::ios_base::app);
        if (file.is_open()){
            file.write(body.data(), limiter);
            file.close();
        }
    }
    else{
        std::ofstream file((fullRequestPathResource + "file").c_str(), std::ios::out | std::ios::binary | std::ios_base::app);
        if (file.is_open()){
            file.write(body.data(), limiter);
            file.close();
        }
    }
}

void Post::handleBoundary(std::string fullRequestPathResource)
{
    std::vector<char> buffer(1024);
    std::vector<char> body;
    int bytesRead = 0;
    bytesRead = recv(clientSock, buffer.data(), buffer.size() - 1, 0);
    while (bytesRead == -1)
        bytesRead = recv(clientSock, buffer.data(), buffer.size() - 1, 0);
    size_t bytesReadTotal = bytesRead;
    std::vector<char>::iterator findBoundary;

    std::vector<char> mainBoundaryVec(mainBoundary.begin(), mainBoundary.end());
    buffer[bytesRead] = 0;
    body.insert(body.end(), buffer.begin(), buffer.begin() + bytesRead);
    std::vector<char> doubleCRLF;
    doubleCRLF.push_back('\r');
    doubleCRLF.push_back('\n');
    if (!std::equal(body.begin() + 2, body.begin() + mainBoundaryVec.size(), mainBoundaryVec.begin()))
        throw BadRequest();
    while (bytesReadTotal != contentLength || !body.empty()){
        findBoundary = std::search(body.begin(), body.end(), mainBoundaryVec.begin(), mainBoundaryVec.end());
        if (findBoundary != body.end()){
            if (findBoundary != body.begin() + 2){
                this->copyToFile(fullRequestPathResource, findBoundary - body.begin() - 4, body);
            }
            body = std::vector<char>(findBoundary + mainBoundaryVec.size() + 2, body.end());
            if (!std::equal(body.begin(), body.begin() + 2, doubleCRLF.begin()))
                this->getBoundaryHeaderData(body, bytesReadTotal, fullRequestPathResource);
            else
                body.clear();
            bytesRead = body.size();
        }
        else
        {
            size_t  bytesReadFile;

            bytesReadFile = bytesRead;
            while (findBoundary == body.end())
            {
                bytesRead = recv(clientSock, buffer.data(), buffer.size() - 1, 0);
                if (bytesRead != -1)
                {
                    buffer[bytesRead] = 0;
                    bytesReadTotal += bytesRead;
                    bytesReadFile += bytesRead;
                    body.insert(body.end(), buffer.begin(), buffer.begin() + bytesRead);
                }
                if (bytesReadFile > mainBoundaryVec.size())
                    findBoundary = std::search(body.begin() + bytesReadFile - mainBoundaryVec.size() - 4, body.end(), mainBoundaryVec.begin(), mainBoundaryVec.end());
                else
                    findBoundary = std::search(body.begin(), body.end(), mainBoundaryVec.begin(), mainBoundaryVec.end());
            }
        }
    }
}


void	Post::getBinaryContentDisposition(std::string &fullRequestPathResource, std::string &header)
{
    std::size_t findFilename;
    std::size_t findContentDispostion;

    findContentDispostion = header.find("Content-Disposition: ");
    if (findContentDispostion != std::string::npos)
    {
        contentDisposition = header.substr(0, header.find("Content-Disposition: "));
        contentDisposition = contentDisposition.substr(0, contentDisposition.find("\r\n"));
        findFilename = contentDisposition.find("filename=");
    }
    else
        findFilename = findContentDispostion;
    if (findFilename != std::string::npos)
    {
        filename = contentDisposition.substr(findFilename + 10);
        filename = filename.substr(0, filename.find("\""));
        std::ofstream file((fullRequestPathResource + filename).c_str(), std::ios::binary);
        file.close();
    }
    else
    {
        std::ofstream file((fullRequestPathResource + filename).c_str(), std::ios::binary);
        file.close();
        filename = "";
    }
}

void Post::handleBinary(const std::string &fullRequestPathResource)
{
    std::vector<char> buffer(1024);
    std::vector<char> body;
    int bytesRead = 0;
    size_t bytesReadTotal = 0;

    while (bytesReadTotal != contentLength){
        bytesRead = recv(clientSock, buffer.data(), buffer.size() - 1, 0);
        if (bytesRead != -1)
        {
            bytesReadTotal += bytesRead;
            body.insert(body.end(), buffer.begin(), buffer.begin() + bytesRead);
            copyToFile(fullRequestPathResource, body.size(), body);
            body.clear();
        }
    }
}

std::string  Post::createResponseMessage(std::string &fullRequestPathResource){
    std::string response = "HTTP/1.1 201 Created\r\n"
                            "Content-Type: text/plain\r\n"
                            "Location: " + fullRequestPathResource + "\r\n"
                            "Content-Length: 0\r\n"
                            "\r\n";
    return(response);
}

std::string Post::handleCgi(const std::string &fullRequestPathResource, Server &web)
{
    int pipefd[2];
    pid_t pid;

    if (web.cgiInit == ""){
        return ("Error 400");
    }
    pipe(pipefd);
    pid = fork();
    if (pid == 0){
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        char* argv[] = { (char*)web.cgiInit.c_str(), (char*)fullRequestPathResource.c_str(), NULL };
        char* envp[] = { (char *)web.queryString.c_str(), NULL};
        if (execve(web.cgiInit.c_str(), argv, envp) == -1) {
            std::cerr << "Execve error: " << std::strerror(errno) << '\n';
            exit(1);
        }
    }
    else if (pid > 0){
        char buffer[1024];
        std::string body;
        ssize_t bytesRead;
        int status;

        close(pipefd[1]);
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[bytesRead] = '\0';
            body += buffer;
        }
        close(pipefd[0]);
        waitpid(pid, &status, 0);
        body = Response::createResponseMessage(body);
        return body;
    }
    else {
        std::cerr << "Fork error\n";
        return "Error 400";
    }
    return "Error 400";
}


std::string Post::postResponse(Server &web, std::string RequestPathResource, std::string header)
{
    std::string fullRequestPathResource;
    std::string response;

    fullRequestPathResource = Response::findLocationRoot(web, RequestPathResource);
    DIR* directory = opendir(fullRequestPathResource.c_str());
    if (!directory){
        if (web.containsCgi)
            return (handleCgi(fullRequestPathResource, web));
        else
            return ("Error 400");
    }
    else
        closedir(directory);
    fullRequestPathResource = fullRequestPathResource + "/";
    try{
        this->getContentTypeData(header);
        this->getLength(header, web);
        if (this->contentType == "multipart/form-data")
            this->handleBoundary(fullRequestPathResource);
        else
        {
            this->getBinaryContentDisposition(fullRequestPathResource, header);
            this->handleBinary(fullRequestPathResource);
        }
    }
    catch(std::exception &e){
        std::cout << e.what() << std::endl;
        return (e.what());
    }

    return (this->createResponseMessage(fullRequestPathResource));
}


const char *Post::BadRequest::what() const throw(){
	return ("Error 400");
}

const char *Post::NotFound::what() const throw(){
	return ("Error 404");
}

const char *Post::LengthRequired::what() const throw(){
	return ("Error 411");
}

const char *Post::RequestEntityTooLarge::what() const throw(){
	return ("Error 413");
}

const char *Post::UnsupportedMediaType::what() const throw(){
	return ("Error 415");
}
