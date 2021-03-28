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

#include "utils/Utils.h"
#include <ifaddrs.h>
#include <arpa/inet.h>

uint32_t getMyIp()
{
    uint32_t ipv4Address = 0;
    struct ifaddrs *ifaddr, *ifa;
    int family;

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return 0;
    }

    /* Walk through linked list, maintaining head pointer so we
        can free list later */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL){ continue; }

        family = ifa->ifa_addr->sa_family;
        if (family != AF_INET){ continue; } // Only want ipv4

        struct sockaddr_in* addrIpv4 = (struct sockaddr_in*)ifa->ifa_addr;
        ipv4Address = ntohl(addrIpv4->sin_addr.s_addr);

        // Stop at first address that is not 127.x.x.x
        if ((ipv4Address>>24) != 127) { break; }
    }
    freeifaddrs(ifaddr);

    return ipv4Address;
}


std::string  ipv4ToString(uint32_t ipv4Address)
{
    struct in_addr ip_addr;
    ip_addr.s_addr = htonl(ipv4Address);
    return std::string(inet_ntoa(ip_addr));
}

