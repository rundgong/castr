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

#include "rlog/RLog.h"
#include "utils/Utils.h"
#include "json/json.h"
#include "cast_media_player/MediaHandler.h"


std::string
to_string(PlayerState playerState)
{
    if (playerState == PlayerState::IDLE) return "IDLE";
    if (playerState == PlayerState::BUFFERING) return "BUFFERING";
    if (playerState == PlayerState::PLAYING) return "PLAYING";
    if (playerState == PlayerState::PAUSED) return "PAUSED";

    return "--UNKNOWN-STATE--";
}

std::string
to_string(IdleReason idleReason)
{
    if (idleReason == IdleReason::UNKNOWN) return "UNKNOWN";
    if (idleReason == IdleReason::STOPPED) return "STOPPED";
    if (idleReason == IdleReason::FINISHED) return "FINISHED";
    if (idleReason == IdleReason::ERROR) return "ERROR";

    return "--UNKNOWN-IDLE-REASON--";
}

MediaHandler::MediaHandler()
{
    reset();
}

void
MediaHandler::reset()
{
    mLatestRequestId = 0;
    mMediaSessionId = 0;
    mMediaStatus = MediaStatus();
    for (MediaStatusCallBack* cb : mMediaStatusCallbacks)
    {
        cb->onMediaStatusUpdate(mMediaStatus);
    }
}

void
MediaHandler::addMediaFinishedCallback(MediaFinishedCallBack* callback)
{
    mMediaFinishedCallbacks.push_back(callback);
}

void
MediaHandler::addMediaStatusCallBack(MediaStatusCallBack* callback)
{
    mMediaStatusCallbacks.push_back(callback);
}

static PlayerState
player_state_from_string(const std::string& playerStateString)
{
    if (playerStateString == "IDLE" || playerStateString == "")
    {
        return PlayerState::IDLE;
    }

    if (playerStateString == "BUFFERING")
    {
        return PlayerState::BUFFERING;
    }

    if (playerStateString == "PLAYING")
    {
        return PlayerState::PLAYING;
    }

    if (playerStateString == "PAUSED")
    {
        return PlayerState::PAUSED;
    }

    return PlayerState::IDLE;  // Should normally not happen
}

int
MediaHandler::onCastMessage( CastLink* castLink, const CastMessage& castMessage )
{
    RLOG(rlog::Verbose, "MediaHandler::onCastMessage" )

    Json::Reader jsonReader;
    Json::Value castPayloadJson;
    jsonReader.parse( castMessage.payload_utf8(), castPayloadJson );

    uint32_t requestId = castPayloadJson["requestId"].asUInt();
    std::string type = castPayloadJson["type"].asString();

    if (type == "ERROR")
    {
        if (castPayloadJson.isMember("detailedErrorCode"))
        {
            mMediaStatus.castErrorCode = castPayloadJson["detailedErrorCode"].asInt();
        }
        executeMediaStatusCallbacks();
        return 0;
    }

    if (type != "MEDIA_STATUS"){ return 0; }    // Unknown message type
    if (castPayloadJson["status"].size() == 0){ return 0; }    // Empty message

    std::string newPlayerState = castPayloadJson["status"][0]["playerState"].asString();
    mMediaSessionId = castPayloadJson["status"][0]["mediaSessionId"].asUInt();

    mMediaStatus.playerState = player_state_from_string(newPlayerState);
    mMediaStatus.idleReason = IdleReason::UNKNOWN;
    mMediaStatus.castErrorCode = 0;

    if(mMediaStatus.playerState == PlayerState::IDLE)
    {
        std::string idleReason = castPayloadJson["status"][0]["idleReason"].asString();
        if (idleReason == "FINISHED")
        {
            mMediaStatus.idleReason = IdleReason::FINISHED;
            for (MediaFinishedCallBack* cb : mMediaFinishedCallbacks)
            {
                cb->onMediaFinished();
            }
        }
        if (idleReason == "ERROR")
        {
            mMediaStatus.idleReason = IdleReason::ERROR;
        }
    }

    if (castPayloadJson["status"][0].isMember("currentTime"))
    {
        mMediaStatus.currentTime = castPayloadJson["status"][0]["currentTime"].asDouble();
    }

    if (castPayloadJson["status"][0].isMember("media"))
    {
        auto& mediaJson = castPayloadJson["status"][0]["media"];
        if (mediaJson.isMember("duration"))
        {
            mMediaStatus.duration = mediaJson["duration"].asDouble();
        }
        if (mediaJson.isMember("contentId"))
        {
            mMediaStatus.contentId = mediaJson["contentId"].asString();
        }
    }

    if (castPayloadJson["status"][0].isMember("liveSeekableRange"))
    {
        auto& liveSeekableRangeJson = castPayloadJson["status"][0]["liveSeekableRange"];
        if (liveSeekableRangeJson.isMember("start"))
        {
            mMediaStatus.seekRangeStart = liveSeekableRangeJson["start"].asDouble();
        }
        if (liveSeekableRangeJson.isMember("end"))
        {
            mMediaStatus.seekRangeEnd = liveSeekableRangeJson["end"].asDouble();
        }
        if (liveSeekableRangeJson.isMember("isLiveDone"))
        {
            mMediaStatus.streamIsFinished = liveSeekableRangeJson["isLiveDone"].asBool();
        }
    }

    RLOG(rlog::Verbose, "Media Status: mediaSessionId=" << mMediaSessionId
        << ", playerState=" << to_string(mMediaStatus.playerState)
        << ", contentId=" << mMediaStatus.contentId
        << ", duration=" << mMediaStatus.duration
        << ", currentTime=" << mMediaStatus.currentTime
        << ", seekRangeStart=" << mMediaStatus.seekRangeStart
        << ", seekRangeEnd=" << mMediaStatus.seekRangeEnd
        << ", streamIsFinished=" << mMediaStatus.streamIsFinished)

    if (requestId != 0)
    {
        mLatestRequestId = requestId;
    }

    executeMediaStatusCallbacks();

    return 0;
}

void
MediaHandler::executeMediaStatusCallbacks()
{
    for (MediaStatusCallBack* cb : mMediaStatusCallbacks)
    {
        cb->onMediaStatusUpdate(mMediaStatus);
    }
}
