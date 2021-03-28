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

#include "cast_media_player/CastPayloads.h"
#include "json/json.h"
#include <rweb/RWebUtils.h>
#include <utils/Utils.h>


std::string
getLaunchReceiverPayload(uint32_t requestId, const std::string& appId)
{
    Json::Value payload;
    payload["requestId"] = requestId;
    payload["type"] = "LAUNCH";
    payload["appId"] = appId;

    return getJsonString(payload);
}

std::string
getMediaLoadPayload(uint32_t requestId, const std::string& videoUrl)
{
    Json::Value payload;
    payload["requestId"] = requestId;
    payload["type"] = "LOAD";
    payload["media"] = Json::Value();
    payload["media"]["contentId"] = videoUrl;
    payload["media"]["streamType"] = "NONE";    // NONE,BUFFERED,LIVE
    payload["media"]["contentType"] = extension_to_mime_type(videoUrl);

    return getJsonString(payload);
}

std::string
getMediaPlayPayload(uint32_t requestId, uint32_t mediaSessionId)
{
    Json::Value payload;
    payload["requestId"] = requestId;
    payload["mediaSessionId"] = mediaSessionId;
    payload["type"] = "PLAY";

    return getJsonString(payload);
}

std::string
getMediaPausePayload(uint32_t requestId, uint32_t mediaSessionId)
{
    Json::Value payload;
    payload["requestId"] = requestId;
    payload["mediaSessionId"] = mediaSessionId;
    payload["type"] = "PAUSE";

    return getJsonString(payload);
}

std::string
getMediaSeekPayload(uint32_t requestId, uint32_t mediaSessionId, double time)
{
    Json::Value payload;
    payload["requestId"] = requestId;
    payload["mediaSessionId"] = mediaSessionId;
    payload["type"] = "SEEK";
    payload["currentTime"] = time;

    return getJsonString(payload);
}

std::string
getReceiverSetVolumeLevelPayload(uint32_t requestId, double level)
{
    Json::Value payload;
    payload["requestId"] = requestId;
    payload["type"] = "SET_VOLUME";
    payload["volume"] = Json::Value();
    payload["volume"]["level"] = level;

    return getJsonString(payload);
}

std::string
getReceiverSetVolumeMutedPayload(uint32_t requestId, bool muted )
{
    Json::Value payload;
    payload["requestId"] = requestId;
    payload["type"] = "SET_VOLUME";
    payload["volume"] = Json::Value();
    payload["volume"]["muted"] = muted;

    return getJsonString(payload);
}

