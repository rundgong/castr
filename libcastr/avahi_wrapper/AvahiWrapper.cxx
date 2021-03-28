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

#include "avahi_wrapper/AvahiWrapper.h"
#include "rlog/RLog.h"
#include <arpa/inet.h>

std::map<std::string, std::string>
parseTxtList(AvahiStringList *txtList)
{
    std::map<std::string, std::string> txtData;
    while (txtList!=nullptr)
    {
        std::string item((const char*)txtList->text, txtList->size);
        int equalsPos = item.find("=");
        std::string name = item.substr(0,equalsPos);
        std::string val = item.substr(equalsPos+1);
        txtData[name] = val;
        RLOG(rlog::Debug, "txt[" << name.c_str() << "]=" << val.c_str() )
        txtList = txtList->next;
    }

    return txtData;
}

static void resolve_callback(
    AvahiServiceResolver *resolver,
    AVAHI_GCC_UNUSED AvahiIfIndex interface,
    AVAHI_GCC_UNUSED AvahiProtocol protocol,
    AvahiResolverEvent event,
    const char *name,
    const char *type,
    const char *domain,
    const char *host_name,
    const AvahiAddress *address,
    uint16_t port,
    AvahiStringList *txt,
    AvahiLookupResultFlags flags,
    void* userData)
{
    AvahiWrapper* avahi = static_cast<AvahiWrapper*>(userData);
    AvahiClient *client = avahi->client();


    assert(resolver);
    // Called whenever a service has been resolved successfully or timed out
    switch (event) {
        case AVAHI_RESOLVER_FAILURE:
            RLOG(rlog::Important, "(Resolver) Failed to resolve service '" << name 
                                << "' of type '" << type << "' in domain '" << domain 
                                << "': " << avahi_strerror(avahi_client_errno(client)))
            break;
        case AVAHI_RESOLVER_FOUND: 
        {
            if (address->proto!=AVAHI_PROTO_INET){ break; }  // Only interested in ipv4

            MdnsServiceData serviceData;
            serviceData.serviceName = name;
            serviceData.ipv4Address = ntohl(address->data.ipv4.address);
            serviceData.port = port;
            serviceData.txt = parseTxtList(txt);
            avahi->addService(serviceData);

            char a[AVAHI_ADDRESS_STR_MAX], *t;
            RLOG(rlog::Debug, "Service '" << name << "' of type '" << type << "' in domain '" << domain << "':\n")
            avahi_address_snprint(a, sizeof(a), address);
            t = avahi_string_list_to_string(txt);
            RLOG(rlog::Debug, "\t" << host_name << ":" << port << " (" << a << ")\n"
                            << "\tTXT=" << t
                            << "\n\tcookie is " << avahi_string_list_get_service_cookie(txt)
                            << "\n\tis_local: " << !!(flags & AVAHI_LOOKUP_RESULT_LOCAL)
                            << "\n\tour_own: " << !!(flags & AVAHI_LOOKUP_RESULT_OUR_OWN)
                            << "\n\twide_area: " << !!(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA)
                            << "\n\tmulticast: " << !!(flags & AVAHI_LOOKUP_RESULT_MULTICAST)
                            << "\n\tcached: " << !!(flags & AVAHI_LOOKUP_RESULT_CACHED))
            avahi_free(t);
        }
    }
    avahi_service_resolver_free(resolver);
}
static void browse_callback(
    AvahiServiceBrowser *browser,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    const char *type,
    const char *domain,
    AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
    void* userData)
{
    AvahiWrapper* avahi = static_cast<AvahiWrapper*>(userData);

    AvahiClient *client = avahi->client();
    assert(browser);
    // Called whenever a new services becomes available on the LAN or is removed from the LAN
    switch (event)
    {
        case AVAHI_BROWSER_FAILURE:
            RLOG(rlog::Important, "(Browser) " << avahi_strerror(avahi_client_errno(client)))
            avahi->quitPoll();
            avahi->setFinished();
            return;
        case AVAHI_BROWSER_NEW:
            RLOG(rlog::Debug, "(Browser) NEW: service '" << name << "' of type '" << type << "' in domain '" << domain << "'") 
            /* We ignore the returned resolver object. In the callback
               function we free it. If the server is terminated before
               the callback function is called the server will free
               the resolver for us. */
            if (!(avahi_service_resolver_new(client, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, (AvahiLookupFlags) 0, resolve_callback, avahi)))
            {
                RLOG(rlog::Debug, "Failed to resolve service '" << name << "': " << avahi_strerror(avahi_client_errno(client)))
            }
            break;
        case AVAHI_BROWSER_REMOVE:
            RLOG(rlog::Debug, "(Browser) REMOVE: service '" << name << "' of type '" << type << "' in domain '" << domain << "'") 
            break;
        case AVAHI_BROWSER_ALL_FOR_NOW:
            RLOG(rlog::Debug, "(Browser) ALL_FOR_NOW");
            avahi->setFinished();
            break;
        case AVAHI_BROWSER_CACHE_EXHAUSTED:
            RLOG(rlog::Debug, "(Browser) CACHE_EXHAUSTED");
            break;
    }
}
static void client_callback(AvahiClient *client, 
                            AvahiClientState state,
                            void * userData)
{
    AvahiWrapper* avahi = static_cast<AvahiWrapper*>(userData);
    assert(client);
    /* Called whenever the client or server state changes */
    if (state == AVAHI_CLIENT_FAILURE) 
    {
        RLOG(rlog::Debug, "Server connection failure: " << avahi_strerror(avahi_client_errno(client)))
        avahi->quitPoll();
        avahi->setFinished();
    }
}




AvahiWrapper::AvahiWrapper(const std::string& serviceType)
    : mServiceType(serviceType)
{
    initAvahi();
}

AvahiWrapper::~AvahiWrapper()
{
    closeAvahi();

    if (mPollThread.joinable())
    {
        mPollThread.join();
    }
}

std::vector<MdnsServiceData>
AvahiWrapper::getServers()
{
    while (mInitialized && !mBrowseFinished)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return mServers;
}


void
AvahiWrapper::initAvahi()
{
    int error;

    mSimplePoll = avahi_simple_poll_new(); // Allocate main loop object
    if (!mSimplePoll)
    {
        RLOG(rlog::Important, "Failed to create simple poll object.")
        return;
    }

    // Allocate a new client
    mAvahiClient = avahi_client_new(avahi_simple_poll_get(mSimplePoll),
                                    (AvahiClientFlags)0, client_callback, NULL, &error);
    if (!mAvahiClient)
    {
        RLOG(rlog::Important, "Failed to create client: " << avahi_strerror(error))
        return;
    }

    // Create the service browser
    mServiceBrowser = avahi_service_browser_new(mAvahiClient, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
                                                mServiceType.c_str(), NULL, (AvahiLookupFlags)0, 
                                                browse_callback, this);
    if (!mServiceBrowser)
    {
        RLOG(rlog::Important, "Failed to create service browser: " << avahi_strerror(avahi_client_errno(mAvahiClient)))
        return;
    }

    // Run the main loop
    mPollThread = std::thread([=]()
        {
            avahi_simple_poll_loop(mSimplePoll);
            mInitialized = false;
        });

    mInitialized = true;
}

void
AvahiWrapper::closeAvahi()
{
    if (mInitialized)
    {
        quitPoll();
    }
    if (mServiceBrowser)
    {
        avahi_service_browser_free(mServiceBrowser);
        mServiceBrowser = nullptr;
    }
    if (mAvahiClient)
    {
        avahi_client_free(mAvahiClient);
        mAvahiClient = nullptr;
    }
    if (mSimplePoll)
    {
        avahi_simple_poll_free(mSimplePoll);
        mSimplePoll = nullptr;
    }
}

void
AvahiWrapper::quitPoll()
{
    avahi_simple_poll_quit(mSimplePoll);
    while (mInitialized)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void
AvahiWrapper::setFinished()
{
    mBrowseFinished = true;
}

AvahiClient*
AvahiWrapper::client()
{
    return mAvahiClient;
}

void
AvahiWrapper::addService(const MdnsServiceData& serviceData)
{
    mServers.push_back(serviceData);
}
