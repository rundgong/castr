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

enum class PlayerState
{
    IDLE,
    BUFFERING,
    PLAYING,
    PAUSED
};

enum class IdleReason
{
    UNKNOWN,
    STOPPED,
    FINISHED,
    ERROR
};

// Error codes taken from here:
// https://developers.google.com/android/reference/com/google/android/gms/cast/MediaError.DetailedErrorCode
enum class CastErrorCodes
{
    MEDIA_UNKNOWN = 100,
    MEDIA_DECODE = 102,
    MEDIA_NETWORK = 103,
};

std::string to_string(PlayerState playerState);
std::string to_string(IdleReason idleReason);

struct MediaStatus
{
    std::string contentId;  // file name
    double duration = -1.0;
    double currentTime = 0.0;
    PlayerState playerState = PlayerState::IDLE;
    IdleReason idleReason = IdleReason::STOPPED;
    int castErrorCode = 0;

    // Streams may support seeking. These variables indicate the allowed range
    double seekRangeStart = -1.0;
    double seekRangeEnd = -1.0;

    bool streamIsFinished = false;  // Stream is finished or still transmitting/encoding
};

class MediaFinishedCallBack
{
public:
    virtual ~MediaFinishedCallBack(){}

    virtual void onMediaFinished() = 0;
};

class MediaStatusCallBack
{
public:
    virtual ~MediaStatusCallBack(){}

    virtual void onMediaStatusUpdate( const MediaStatus& mediaStatus ) = 0;
};

class MediaHandler : public CastMessageHandler
{
public:

    static constexpr auto sNameSpace = "urn:x-cast:com.google.cast.media";

    MediaHandler();

    std::string nameSpace(){ return sNameSpace; }
    int onCastMessage(CastLink* castLink, const CastMessage& castMessage );
    PlayerState playerState(){ return mMediaStatus.playerState; }
    uint32_t mediaSessionId(){ return mMediaSessionId; }
    const MediaStatus& mediaStatus(){ return mMediaStatus; }

    void reset();

    void addMediaFinishedCallback(MediaFinishedCallBack* callback);
    void addMediaStatusCallBack(MediaStatusCallBack* callback);

private:
    void executeMediaStatusCallbacks();

    uint32_t mLatestRequestId;
    uint32_t mMediaSessionId;
    MediaStatus mMediaStatus;
    std::list<MediaFinishedCallBack*> mMediaFinishedCallbacks;
    std::list<MediaStatusCallBack*> mMediaStatusCallbacks;

};

