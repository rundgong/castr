#pragma once

/*
    Copyright 2020 rundgong

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


#include <string>
#include <list>
#include "CastLink.h"

static const std::string castrReceiverApp = "CC1AD845";

struct ReceiverStatus
{
    double volumeLevel = 1.0;
    bool volumeMuted = false;
};

class ReceiverStatusCallBack
{
public:
    virtual ~ReceiverStatusCallBack(){}

    virtual void onReceiverStatusUpdate(const ReceiverStatus& mediaStatus) = 0;
};

class ReceiverHandler : public CastMessageHandler
{
public:
    static constexpr auto sNameSpace = "urn:x-cast:com.google.cast.receiver";

    ReceiverHandler();

    std::string nameSpace(){ return sNameSpace; }
    int onCastMessage(CastLink* castLink, const CastMessage& castMessage );

    uint32_t latestRequestId() { return mLatestRequestId; };
    std::string sessionId() { return mSessionId; };
    std::string transportId() { return mTransportId; };
    void reset();

    const ReceiverStatus& receiverStatus() { return mReceiverStatus; };
    void addReceiverStatusCallBack(ReceiverStatusCallBack* callback);

private:
    uint32_t mLatestRequestId;
    std::string mSessionId;
    std::string mTransportId;

    ReceiverStatus mReceiverStatus;
    std::list<ReceiverStatusCallBack*> mReceiverStatusCallbacks;
};

