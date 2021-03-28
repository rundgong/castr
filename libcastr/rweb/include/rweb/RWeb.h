#pragma once

#include <string>
#include <vector>


struct PathFilterItem
{
    std::string publicPath;     // file path in the url
    std::string internalPath;   // file path in the file system. May be global, or relative to root dir
};

class RWeb
{
public:
    RWeb(const std::string& rootDir, int port );
    ~RWeb();

    bool start();
    void stop();

    void setFilter(const std::vector<PathFilterItem>& filter );

    enum ServerState
    {
        ss_Init,
        ss_Running,
        ss_Stopping,
        ss_Finished
    };

private:

    void serverLoop();
    void handleRequest(int fd, int connectionId);
    std::string getInternalPath(const std::string& publicPath);

    std::string mRootDir;
    int mPort;
    ServerState mServerState = ss_Init;
    int mListenFD = 0;

    // When filter is enabled, only serve files in the filter.
    // Otherwise serve all files below root dir
    std::vector<PathFilterItem> mFilter;
};

