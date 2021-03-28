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

#pragma once

#include <string>
#include <vector>
#include <map>
#include <thread>

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>


struct MdnsServiceData
{
    std::string serviceName;
    uint32_t ipv4Address = 0;
    uint16_t port = 0;

    std::map<std::string, std::string> txt;
};

class AvahiWrapper
{
public:
    AvahiWrapper(const std::string& serviceType);
    ~AvahiWrapper();

    std::vector<MdnsServiceData> getServers();

    void quitPoll();
    void setFinished();
    AvahiClient * client();
    
    void addService(const MdnsServiceData& serviceData);

private:

    void initAvahi();
    void closeAvahi();

    bool mInitialized = false;
    bool mBrowseFinished = false;
    std::string mServiceType;

    std::vector<MdnsServiceData> mServers;


    AvahiSimplePoll *mSimplePoll = nullptr;
    AvahiClient *mAvahiClient = nullptr;
    AvahiServiceBrowser *mServiceBrowser = nullptr;

    std::thread mPollThread;

};



