/*
    Copyright 2015-2021 rundgong

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

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "utils/SslWrapper.h"
#include "cast_media_player/CastLink.h"
#include "cast_media_player/CastMediaPlayer.h"
#include "cast_media_player/StreamHelper.h"
#include "rlog/RLog.h"
#include "getch.h"
#include "avahi_wrapper/AvahiWrapper.h"
#include "utils/Utils.h"
#include "rweb/RWeb.h"
#include "rweb/RWebUtils.h"
#include "CliMediaStatus.h"
#include "fmt/core.h"

// We need to start a webserver if we play local files.
// This is not needed if we control files located on other web servers.
static bool sNeedsWebserver = false;

// For streaming files we can not serve up only specific files.
// We need to serve entire folder of the stream since the video is
// split up into multiple chunks.
// This means we can only serve 1 local stream.
static std::string sRWebStreamingFolder = "";

// Toggle this to disable the CLI "UI"
static bool sEnableUI = true;

// This controls is live streams are started from the beginning,
// or if we start the live stream at the "current" time.
// Chromecast will default to the current time, but we can seek
// to the beginning after it starts playing.
static bool sEnableStreamRestart = false;

static std::string sChromecastHost = "";
static std::string sDeviceName = "";
static std::vector<std::string> sFileList;
static std::vector<std::string> sValidFileTypes =
        {"aac", "mp3", "wav", "webm", "mp4",
         "gif", "jpg", "png", "webp","mpd","m3u8"};



static std::string playback_help_string()
{
    std::ostringstream oss;

    oss << "  -- castr playback help --\n"
        << "Controlling playback:\n"
        << "  SPACEBAR: Play/Pause\n"
        << "  n:        Next\n"
        << "  p:        Previous\n"
        << "  s:        Stop\n"
        << "  r:        Restart media from beginning*\n"
        << "  f:        Seek forward 60 seconds*\n"
        << "  F:        Seek forward 600 seconds*\n"
        << "  b:        Seek backwards 60 seconds*\n"
        << "  B:        Seek backwards 600 seconds*\n"
        << "  m:        Mute on/off\n"
        << "  v:        Volume down\n"
        << "  V:        Volume up\n"
        << "  h:        Help\n"
        << "  q:        Quit\n"
        << "\n"
        << "  * Note: Seeking is not supported by all media\n"
        << "\n"
        << "Status bar:\n"
        << "STATUS: X:XX [####  ] Y:YY* | VOL\n"
        << "  STATUS: PLAYING/PAUSED/STOPPED\n"
        << "  X: Current time\n"
        << "  #: Progress bar\n"
        << "  Y: Media length. (*) indicates stream is still encoding, so value will update\n"
        << "  VOL: Media volume\n"
        << "\n";

    return oss.str();
}

static std::string supported_filetypes_help_string()
{
    std::ostringstream oss;

    oss << "Supported file types:\n    ";
    for (auto extension : sValidFileTypes)
    {
        oss << extension << ", ";
    }
    oss << "\n";

    return oss.str();
}

static void print_help(std::string programName)
{
    std::cout << "\nUsage:\n " << programName << " [options] FILE\n\n"
              << "Cast media files to Chromecast device.\n\n"
              << "Options:\n"
              << "  --host=IP_ADDR          IP Address to Chromecast device\n"
              << "  --device=DEVICE_NAME    Name of Chromecast device. Case sensitive\n"
              << "  --help|-h               Print this help message\n"
              << "  --list-devices|-l       List cast devices\n"
              << "  --list-devices-verbose  List cast devices, verbose info\n\n"
              << "  --no-ui                 Disable the default text UI\n\n"
              << "  --stream-restart|-s     Start live stream from beginning\n\n"
              << rlog::logHelp()
              << "\n\n" << supported_filetypes_help_string()
              << "\n\n" << playback_help_string();
}

static std::vector<MdnsServiceData> get_cc_devices()
{
    AvahiWrapper avahi("_googlecast._tcp");
    std::vector<MdnsServiceData> ccServers = avahi.getServers();

    if (ccServers.size() == 0)
    {
        std::cout << "*** Error: No Chromecast devices found on the network" << std::endl;
    }

    return ccServers;
}

static void list_cast_devices(bool verbose)
{
    std::cout << "Searching for Chromecast Devices..." << std::endl;

    std::vector<MdnsServiceData> ccServers = get_cc_devices();

    for (auto mdnsData : ccServers)
    {
        if (verbose)
        {
            std::cout << fmt::format("{} / {} - ID: {} @ {}:{}\n",
                mdnsData.txt["fn"], mdnsData.txt["md"], mdnsData.serviceName,
                ipv4ToString(mdnsData.ipv4Address), mdnsData.port);
        }
        else
        {
            std::cout << mdnsData.txt["fn"] << std::endl;
        }
    }
}

static bool get_default_host(std::string& host, std::string& friendlyName)
{
    std::vector<MdnsServiceData> ccServers = get_cc_devices();
    if (ccServers.size() == 0){ return false; }

    host = ipv4ToString(ccServers.front().ipv4Address);
    friendlyName = ccServers.front().txt["fn"];

    return true;
}

static bool get_host_from_device_name(const std::string& deviceName, std::string& host)
{
    std::vector<MdnsServiceData> ccServers = get_cc_devices();

    for (auto mdnsData : ccServers)
    {
        if (deviceName == mdnsData.txt["fn"])
        {
            host = ipv4ToString(mdnsData.ipv4Address);
            return true;
        }
    }

    std::cout << "*** Error: Can not find device \"" << deviceName << "\"" << std::endl;

    return false;
}

static bool is_valid_file_type(const std::string& filename )
{
    std::string fileExtension = ascii_to_lower(split(filename, '.').back());

    if (found_in(sValidFileTypes, fileExtension))
    {
        return true;
    }

    return false;
}

static bool is_web_url(const std::string& path)
{
    if (path.substr(0,7)=="http://" || path.substr(0,8)=="https://")
    {
        return true;
    }
    return false;
}

static std::string url_encode(const std::string& inputString)
{
    std::ostringstream oss;
    oss.fill('0');

    for (char c : inputString)
    {
        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            oss << c;
            continue;
        }

        // Encode all strange characters as %XX
        oss << std::uppercase;
        oss << '%' << std::setw(2) << std::hex << int((unsigned char) c);
        oss << std::nouppercase;
    }
    //std::cout << "url_encode " << inputString << " => " << oss.str() << std::endl;
    return oss.str();
}

static bool validate_file_list(const std::vector<std::string>& fileList)
{
    if (fileList.size() == 0)
    {
        std::cout << "At least one FILE required" << std::endl;
        return false;
    }

    int localFiles = 0;
    int localStreams = 0;
    int remoteFiles = 0;

    for (const std::string& file : fileList)
    {
        if (is_web_url(file))
        {
            ++remoteFiles;
            continue;
        }
        if (ends_with(file,".mpd") || ends_with(file,".m3u8"))
        {
            if (localStreams!=0)
            {
                std::cout << "Max 1 local stream supported" << std::endl;
                return false;
            }
            ++localStreams;
            sNeedsWebserver = true;
            sRWebStreamingFolder = std::filesystem::path(file).parent_path();
            continue;
        }
        ++localFiles;
        sNeedsWebserver = true;
    }

    if (localStreams && localFiles)
    {
        std::cout << "Can not play both local files and local streams" << std::endl;
        return false;
    }
    return true;
}

int handle_arguments(int argc, char* argv[])
{
    int i;
    for( i=1; i<argc; ++i )
    {
        std::string arg(argv[i]);
        if (rlog::parseArgument(arg)) {;}
        else if (arg.substr(0,7) == "--host=")
        {
            sChromecastHost = arg.substr(7);
        }
        else if (arg.substr(0,9) == "--device=")
        {
            sDeviceName = arg.substr(9);
        }
        else if (arg == "--help" || arg == "-h")
        {
            print_help(argv[0]);
            return 1;
        }
        else if (arg == "--list-devices" || arg == "-l")
        {
            list_cast_devices(false);
            return 1;
        }
        else if (arg == "--list-devices-verbose")
        {
            list_cast_devices(true);
            return 1;
        }
        else if (arg == "--no-ui")
        {
            sEnableUI = false;
        }
        else if (arg == "--stream-restart" || arg == "-s")
        {
            sEnableStreamRestart = true;
        }
        else if (arg.substr(0,2) != "--")
        {
            if (is_valid_file_type(arg))
            {
                sFileList.push_back(arg);
            }
            else
            {
                std::cout << "Invalid file type: " << arg << std::endl;
                std::cout << supported_filetypes_help_string() << std::endl;
                return 1;
            }
        }
        else
        {
            std::cout << "Unknown argument: " << arg << std::endl;
            print_help(argv[0]);
            return 1;
        }
    }

    if (!validate_file_list(sFileList))
    {
        return 1;
    }

    return 0;
}


std::vector<PathFilterItem>
create_rweb_filter_from_file_list(const std::vector<std::string>& fileList)
{
    std::vector<PathFilterItem> pathFilters;

    for (const std::string& filePath : fileList)
    {
        // The public path must be url encoded as that is how it will
        // come in the http request
        std::string fileName = url_encode(split(filePath, '/').back());
        pathFilters.push_back({fileName, filePath});
    }

    return pathFilters;
}

std::string
create_url(const std::string& hostName, uint16_t port, const std::string& path)
{
    std::ostringstream oss;
    oss << "http://" << hostName << ":" << port << "/" << path;

    return oss.str();
}

std::vector<std::string>
create_playlist(const std::string& hostName, uint16_t port, const std::vector<std::string>& fileList)
{
    std::vector<std::string> playlist;
    for (const std::string& filePath : fileList)
    {
        playlist.push_back(create_url(hostName, port, filePath));
        RLOG(rlog::Verbose, "playlist add  " << playlist.back() )
    }

    return playlist;
}

std::vector<std::string>
create_playlist(const std::string& hostName, uint16_t port, const std::vector<PathFilterItem>& filterList)
{
    std::vector<std::string> playlist;
    for (auto& item : filterList)
    {
        if (is_web_url(item.internalPath))
        {
            playlist.push_back(item.internalPath);
        }
        else
        {
            playlist.push_back(create_url(hostName, port, item.publicPath));
        }
        RLOG(rlog::Verbose, "playlist add  " << playlist.back() )
    }

    return playlist;
}

int main(int argc, char* argv[])
{
    uint32_t myIp = getMyIp();
    uint16_t rwebPort = 20000;
    std::string rwebRoot = ".";
    bool useRwebFilter = true;  // By default only serve specific files.
    std::unique_ptr<RWeb> rwebPtr;
    std::unique_ptr<CastMediaPlayer> castMediaPlayerPtr;
    std::string ccFriendlyName;
    CliMediaStatus cliMediaStatus;
    StreamStartHelper streamStartHelper;

    rlog::logLevel = rlog::Important;   // Silence most logging

    if( handle_arguments(argc,argv)  ){ return 1; }

    if (sDeviceName != "")
    {
        if (!get_host_from_device_name(sDeviceName, sChromecastHost)){ return 1; }

        ccFriendlyName = sDeviceName;
    }

    if (sChromecastHost == "") 
    {
        if (!get_default_host(sChromecastHost, ccFriendlyName)){ return 1; }
    }

    std::vector<PathFilterItem> rwebFilter = create_rweb_filter_from_file_list(sFileList);
    std::vector<std::string> playlist = create_playlist(ipv4ToString(myIp), rwebPort, rwebFilter);

    for (auto x : rwebFilter)
    {
        RLOG(rlog::Debug, "rwebFilter " << x.publicPath << " => " << x.internalPath)
    }
    if (sNeedsWebserver)
    {
        RLOG(rlog::Debug, "Starting webserver")
        if ( sRWebStreamingFolder != "")
        {
            rwebRoot = sRWebStreamingFolder;
            useRwebFilter = false;  // Serve all of streaming folder instead of specific files
        }
        rwebPtr = std::make_unique<RWeb>(rwebRoot, rwebPort);
        if (useRwebFilter)
        {
            rwebPtr->setFilter(rwebFilter);
        }
        if (rwebPtr->start() == false)
        {
            std::cerr << "*** Error: failed to start webserver" << std::endl;
            return 1;
        }
    }
    RLOG(rlog::Important, "castr player started at " << ipv4ToString(myIp) )
    RLOG(rlog::Important, "Chromecast host: " << ccFriendlyName << " @ " << sChromecastHost )

    castMediaPlayerPtr = std::make_unique<CastMediaPlayer>(sChromecastHost, 8009);
    castMediaPlayerPtr->setPlayList(playlist);
    if (sEnableUI)
    {
        castMediaPlayerPtr->addMediaStatusCallBack(&cliMediaStatus);
        castMediaPlayerPtr->addReceiverStatusCallBack(&cliMediaStatus);
    }

    if (sEnableStreamRestart)
    {
        streamStartHelper.setPlayer(castMediaPlayerPtr.get());
        castMediaPlayerPtr->addMediaStatusCallBack(&streamStartHelper);
    }

    castMediaPlayerPtr->playOrPause();  // Automatically start playback
    castMediaPlayerPtr->startStatusTimer();  // Periodically receive Media Status updates

    auto seekDiff = [&castMediaPlayerPtr](double diff)
    {
        if (!castMediaPlayerPtr->seekEnabled())
        {
            std::cout << "\n\nSeeking not supported\n\n";
            return;
        }
        castMediaPlayerPtr->seekDiff(diff);
    };

    bool run = true;

    while(run)
    {
        int c = getch();

        if( c == ' ' ){ castMediaPlayerPtr->playOrPause(); }
        if( c == 'h' ){ std::cout << "\n\n" << playback_help_string(); }
        if( c == 's' ){ castMediaPlayerPtr->stop(); }
        if( c == 'n' ){ castMediaPlayerPtr->next(); }
        if( c == 'p' ){ castMediaPlayerPtr->previous(); }
        if( c == 'g' ){ castMediaPlayerPtr->getStatus(); }
        if( c == 'm' ){ castMediaPlayerPtr->toggleVolumeMuted(); }
        if( c == 'v' ){ castMediaPlayerPtr->decreaseVolumeLevel(); }
        if( c == 'V' ){ castMediaPlayerPtr->increaseVolumeLevel(); }
        if( c == 'r' ){ castMediaPlayerPtr->seek(0); }
        if( c == 'f' ){ seekDiff(60); }
        if( c == 'F' ){ seekDiff(600); }
        if( c == 'b' ){ seekDiff(-60); }
        if( c == 'B' ){ seekDiff(-600); }
        if( c == 'q' ){ run = false; }
    }

    castMediaPlayerPtr.reset(); // Finish all threads before logging exit

    RLOG(rlog::Important, "\ncastr player exit" )
    rlog::closeFile();

    return 0;
}

