/*
    Copyright 2015-2020 rundgong

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

#include "cast_media_player/ReceiverHandler.h"
#include "json/json.h"
#include "rlog/RLog.h"


ReceiverHandler::ReceiverHandler()
  :  mLatestRequestId(0)
{
}

int
ReceiverHandler::onCastMessage( CastLink* castLink, const CastMessage& castMessage )
{
    RLOG(rlog::Verbose, "ReceiverHandler::onCastMessage" )

    Json::Reader jsonReader;
    Json::Value castPayloadJson;
    jsonReader.parse( castMessage.payload_utf8(), castPayloadJson );

    uint32_t requestId = castPayloadJson["requestId"].asUInt();
    std::string type = castPayloadJson["type"].asString();

    if (type != "RECEIVER_STATUS"){ return 0; }    // Unknown message type
    if (castPayloadJson["status"].isMember("applications") == false){ return 0; }    // Message has no application info

    std::string appId = castPayloadJson["status"]["applications"][0]["appId"].asString();

    if (appId == castrReceiverApp)
    {
        mSessionId = castPayloadJson["status"]["applications"][0]["sessionId"].asString();
        mTransportId = castPayloadJson["status"]["applications"][0]["transportId"].asString();
    }
    else
    {
        // When the idle app, or other app, starts we are not interested in it's session
        mSessionId = "";
        mTransportId = "";
    }

    if (castPayloadJson["status"].isMember("volume"))
    {
        auto& volumeJson = castPayloadJson["status"]["volume"];

        if (volumeJson.isMember("level"))
        {
            mReceiverStatus.volumeLevel = volumeJson["level"].asDouble();
        }

        if (volumeJson.isMember("muted"))
        {
            mReceiverStatus.volumeMuted = volumeJson["muted"].asBool();
        }
    }

    RLOG(rlog::Verbose,
            "  requestId = " << requestId << std::endl
         << "  type = " << type << std::endl
         << "  sessionId = " << mSessionId << std::endl
         << "  transportId = " << mTransportId << std::endl
         << "  vol.level = " << mReceiverStatus.volumeLevel << std::endl
         << "  vol.muted = " << mReceiverStatus.volumeMuted << std::endl )

    if (requestId != 0)
    {
        mLatestRequestId = requestId;
    }

    for (ReceiverStatusCallBack* cb : mReceiverStatusCallbacks)
    {
        cb->onReceiverStatusUpdate(mReceiverStatus);
    }

    return 0;
}

void ReceiverHandler::reset()
{
    mLatestRequestId = 0;
    mSessionId = "";
    mTransportId = "";
}


void
ReceiverHandler::addReceiverStatusCallBack(ReceiverStatusCallBack* callback)
{
    mReceiverStatusCallbacks.push_back(callback);
}
