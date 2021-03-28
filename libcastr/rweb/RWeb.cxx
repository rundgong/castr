/*
    Copyright 2019-2021 rundgong

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <iostream>
#include <filesystem>

#include "rlog/RLog.h"
#include "rweb/RWeb.h"
#include "rweb/RWebUtils.h"

#define VERSION 1
#define BUFSIZE 8096
#define FORBIDDEN 403
#define NOTFOUND  404


static std::string FORBIDDEN_MESSAGE = "HTTP/1.1 403 Forbidden\n"
"Content-Length: 185\n"
"Connection: close\n"
"Content-Type: text/html\n"
"\n"
"<html><head>\n"
"<title>403 Forbidden</title>\n"
"</head><body>\n"
"<h1>Forbidden</h1>\n"
"The requested URL, file type or operation is not allowed on this simple static file webserver.\n"
"</body></html>\n";

static std::string NOTFOUND_MESSAGE = "HTTP/1.1 404 Not Found\n"
"Content-Length: 136\n"
"Connection: close\n"
"Content-Type: text/html\n"
"\n"
"<html><head>\n"
"<title>404 Not Found</title>\n"
"</head><body>\n"
"<h1>Not Found</h1>\n"
"The requested URL was not found on this server.\n"
"</body></html>\n";

static void send_error_response(int errorNumber, int fdSocket)
{
    switch (errorNumber)
    {
    case FORBIDDEN:
        send(fdSocket,
             FORBIDDEN_MESSAGE.c_str(),
             FORBIDDEN_MESSAGE.size(),
             MSG_NOSIGNAL);
        break;

    case NOTFOUND:
        send(fdSocket,
             NOTFOUND_MESSAGE.c_str(),
             NOTFOUND_MESSAGE.size(),
             MSG_NOSIGNAL);
        break;
    }
}


static int log_error(const std::string& message, int errorCode=1)
{
    RLOG(rlog::Critical, "ERROR: " << message << ",Errno=" << errno << ", pid=" << getpid());
    return errorCode;
}


static void log_http_error(int errorCode, const std::string& headline, const std::string& message, int connectionId)
{
    char logbuffer[BUFSIZE*2];

    switch (errorCode)
    {
    case FORBIDDEN: 
        (void)sprintf(logbuffer,"%d: FORBIDDEN: %s: %s", connectionId, headline.c_str(), message.c_str());
        break;
    case NOTFOUND: 
        (void)sprintf(logbuffer,"%d: NOT FOUND: %s: %s", connectionId, headline.c_str(), message.c_str()); 
        break;
    default:
        RLOG(rlog::Critical, "Log error. Unknown error code=" << errorCode);
        return;
    }
    RLOG_NETWORK(logbuffer);
}

static std::string get_origin_header_url(const std::string& request)
{
    std::size_t origin_header_start = request.find("Origin: ");
    if (origin_header_start == std::string::npos)
    {
        return "";
    }
    std::size_t origin_value_start = origin_header_start + 8;
    std::size_t origin_end = request.find("\n", origin_value_start);
    std::string result = request.substr(origin_value_start, origin_end - origin_value_start);

    return result;
}

void 
RWeb::handleRequest(int fd, int connectionId)
{
    int file_fd;
    long ret, len;
    std::string buffer(BUFSIZE+1, 0);

    ret = read(fd, buffer.data(), BUFSIZE);     // read http request in one go
    if (ret == 0 || ret == -1)    // read failure - abort
    {
        log_http_error(FORBIDDEN,"failed to read browser request","", connectionId);
        send_error_response(FORBIDDEN,fd);
        return;
    }
    if (ret > 0 && ret < BUFSIZE)    // return code is valid chars
    {
        buffer[ret]=0;        // terminate the buffer
    }
    else
    {
        buffer[0]=0;
    }
//     for (i=0; i<ret; i++)    // remove CR and LF characters
//     {
//         if (buffer[i] == '\r' || buffer[i] == '\n')
//         {
//             buffer[i]='*';
//         }
//     }

    RLOG_NETWORK(connectionId << ": NET Request:\n" << buffer.c_str());
    
    std::string method = buffer.substr(0,4);

    if (!found_in({"GET ", "get "}, method))
    {
        log_http_error(FORBIDDEN,"Only simple GET operation supported", buffer, connectionId);
        send_error_response(FORBIDDEN,fd);
        return;
    }

    // http format is "GET file_name " + other stuff
    // find the second space to ignore extra stuff
    std::size_t fileNameEnd;
    if ((fileNameEnd = buffer.find(' ', 4)) == std::string::npos)
    {
        log_http_error(FORBIDDEN,"Request Error", buffer, connectionId);
        send_error_response(FORBIDDEN,fd);
        return;
    }

    std::string fileName(buffer.substr(4, fileNameEnd-4));
    if (fileName.find("..") != std::string::npos)
    {
        log_http_error(FORBIDDEN,"Parent directory (..) path names not supported", fileName, connectionId);
        send_error_response(FORBIDDEN,fd);
        return;
    }

    if (ends_with(fileName, "/"))
    {
        fileName += "index.html";
    }

    std::string internalFileName = getInternalPath(fileName);
    if (internalFileName == "")
    {
        log_http_error(NOTFOUND, "failed get internal path", fileName, connectionId);
        send_error_response(NOTFOUND,fd);
        return;
    }

    std::string mimeType = extension_to_mime_type(internalFileName);

    if (mimeType == "")
    {
        log_http_error(FORBIDDEN,"file extension type not supported", internalFileName, connectionId);
        send_error_response(FORBIDDEN,fd);
        return;
    }

    if (( file_fd = open(internalFileName.c_str(),O_RDONLY)) == -1)  // open the file for reading
    {
        log_http_error(NOTFOUND, "failed to open file", internalFileName, connectionId);
        send_error_response(NOTFOUND,fd);
        return;
    }

    RLOG_N(connectionId << ": SEND " << fileName << " => " << internalFileName);
    len = (long)lseek(file_fd, (off_t)0, SEEK_END); // lseek to the file end to find the length
    lseek(file_fd, (off_t)0, SEEK_SET); // lseek back to the file start ready for reading

    std::string originUrl = get_origin_header_url(buffer);
    std::string originResponse = "";
    if (originUrl.size() != 0)
    {
        originResponse = "Access-Control-Allow-Origin: " + originUrl + "\n"
                        + "Vary: Origin\n";
    }

    sprintf(buffer.data(), "HTTP/1.1 200 OK\nServer: rweb/%d.0\n"
                           "Content-Length: %ld\n"
                           "%s" // origin response
                           "Connection: close\n"
                           "Content-Type: %s\n\n", VERSION, len, originResponse.c_str(), mimeType.c_str()); // Headers + a blank line 
//     sprintf(buffer,"HTTP/1.1 200 OK\nServer: rweb/%d.0\nConnection: close\nContent-Type: %s\n\n", VERSION, mimeType.c_str()); // Headers + a blank line

    RLOG_NETWORK(connectionId << ": NET Response Headers:\n" << buffer.c_str());
    send(fd, buffer.data(), strlen(buffer.c_str()), MSG_NOSIGNAL);

    // send file in 8KB block - last block may be smaller
    while ( (ret = read(file_fd, buffer.data(), BUFSIZE)) > 0 )
    {
//         write(fd, buffer.data(), ret);
        ret = send(fd, buffer.data(), ret, MSG_NOSIGNAL);
        if (ret == -1) { break; }   // Something went wrong, so we can stop trying to send
    }
    RLOG_N(connectionId << ": SEND Finished");
    sleep(1);    // allow socket to drain before signalling the socket is closed
    close(file_fd);
    close(fd);
}


RWeb::RWeb(const std::string& rootDir, int port)
  : mRootDir(rootDir), 
    mPort(port)
{
}

RWeb::~RWeb()
{
    stop();
}


bool
RWeb::start()
{
    if (mRootDir == "")
    {
        RLOG(rlog::Critical, "Error: root directory not specified");
        return false;
    }
    if (mRootDir == ".")
    {
        mRootDir = std::filesystem::current_path();
    }

    if (mRootDir == "/"
      || mRootDir.substr(0,5) == "/sbin"
      || found_in({"/bin","/dev","/etb","/lib","/usr"}, mRootDir.substr(0,4)))
    {
        RLOG(rlog::Critical, "ERROR: Bad root directory " << mRootDir << ", see rweb -h");
        return false;
    }

//     if (chdir(mRootDir.c_str()) == -1)
//     {
//         RLOG(rlog::Critical, "ERROR: Can't Change to directory " << mRootDir);
//         return false;
//     }
// 
    if (mPort < 1024 || mPort >60000)
    {
        RLOG(rlog::Critical, "Error: Invalid port number: " << mPort );
        return false;
    }

    RLOG_N("rweb starting. Port: " << mPort << ", root dir: " << mRootDir);

    // setup the listen socket
    if ((mListenFD = socket(AF_INET, SOCK_STREAM,0)) <0)
    {
        log_error("socket create");
        return false;
    }

    struct sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(mPort);

    if (bind(mListenFD, (struct sockaddr *)&serverAddr,sizeof(serverAddr)) <0)
    {
        log_error("socket bind");
        return false;
    }
    if (listen(mListenFD,64) <0)
    {
        log_error("socket listen");
        return false;
    }

    mServerState = ss_Running;
    std::thread(
        [=](){
            this->serverLoop();
        }
    ).detach();

    return true;
}

void
RWeb::stop()
{
    if (mServerState == ss_Finished){ return; }

    RLOG_N("RWeb stopping...");
    if (mServerState == ss_Running)
    {
        mServerState = ss_Stopping;
        while(mServerState != ss_Finished)
        {
            usleep(1000);
        }
    }
    else
    {
        mServerState = ss_Finished;
    }

    close(mListenFD);

    RLOG_N("RWeb stopped");
}

void
RWeb::setFilter(const std::vector<PathFilterItem>& filter )
{
    mFilter = filter;
}

void
RWeb::serverLoop()
{
    int connectionId = 0;
    RLOG(rlog::Verbose, "Server loop start");
    RLOG(rlog::Verbose, "Waiting for connection #" << connectionId);
    while (mServerState == ss_Running)
    {
        struct pollfd pollEvents = {mListenFD, 0xFF, 0};
        if (poll(&pollEvents, 1, 10) == 0)
        {
            continue;
        }

        struct sockaddr_in clientAddr = {};
        int socketfd;
        socklen_t length = sizeof(clientAddr);

        if ((socketfd = accept(mListenFD, (struct sockaddr *)&clientAddr, &length)) < 0)
        {
            log_error("socket accept");
            return;
        }


        std::thread(
            [=](){
                handleRequest(socketfd,connectionId); 
                return 0;
            }
        ).detach();

        ++connectionId;
        RLOG(rlog::Verbose, "Waiting for connection #" << connectionId);
    }
    mServerState = ss_Finished;
    RLOG(rlog::Verbose, "Server loop finished");
}

std::string
RWeb::getInternalPath(const std::string& publicPath)
{
    if (mFilter.size() == 0)
    {
        return mRootDir + "/" + publicPath;
    }

    RLOG(rlog::Debug, "getInternalPath " << publicPath);
    for (auto item : mFilter)
    {
        RLOG(rlog::Debug, "  publicPath " << item.publicPath);

        // Match with and without leading slash
        // publicPath should contain leading slash.
        // item.publicPath may skip leading slash   
        if (publicPath == item.publicPath ||
            publicPath.substr(1) == item.publicPath)
        {
            if (item.internalPath[0] == '/')    // Absolute path
            {
                RLOG(rlog::Debug, "    match: internalPath " << item.internalPath);
                return item.internalPath;
            }
            else    // relative path
            {
                RLOG(rlog::Debug, "    match: internalPath " << item.internalPath);
                return mRootDir + "/" + item.internalPath;
            }
        }
    }

    return "";
}
