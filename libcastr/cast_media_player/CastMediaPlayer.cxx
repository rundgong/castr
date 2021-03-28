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

#include <iostream>
#include <fstream>

#include "cast_media_player/CastMediaPlayer.h"
#include "cast_media_player/CastPayloads.h"

#include "rlog/RLog.h"
#include "utils/Utils.h"
#include "json/json.h"
#include "rweb/RWebUtils.h"


CastMediaPlayer::CastMediaPlayer(const std::string& host, 
                                 uint16_t port,
                                 const std::string& playListFileName)
 :  mPlayListIndex(0), mHost(host), mPort(port), mRequestId(0)
{
    if (playListFileName != "")
    {
        loadPlayList(playListFileName);
    }
    mMediaHandler.addMediaFinishedCallback(this);
}

CastMediaPlayer::~CastMediaPlayer()
{
    stopStatusTimer();
    stop(); // Stop any playing videos before disconnect
}



void
CastMediaPlayer::setPlayList(std::vector<std::string> playList)
{
    mPlayList = playList;
    mPlayListIndex = 0;
}

void
CastMediaPlayer::addMediaStatusCallBack(MediaStatusCallBack* callback)
{
    mMediaHandler.addMediaStatusCallBack(callback);
}

void
CastMediaPlayer::addReceiverStatusCallBack(ReceiverStatusCallBack* callback)
{
    mReceiverHandler.addReceiverStatusCallBack(callback);
}

void
CastMediaPlayer::onMediaFinished()
{
    RLOG_N("Video #" << mPlayListIndex << " finished. Auto play next")

    next();
}

void
CastMediaPlayer::loadPlayList(const std::string& playListFileName)
{
    RLOG_N( "Load playlist" )
    std::ifstream playListStream(playListFileName);

    std::string line;

    while (!playListStream.eof() && !playListStream.fail())
    {
        std::getline(playListStream,line);
        line = trim(line);  // Remove leading and trailing white space

        if (line.size() == 0)
        {
            continue;
        }

        RLOG_N( "Load playlist #" << mPlayList.size() << " - " << line << ".")

        mPlayList.push_back(line);
    }
    RLOG(rlog::Verbose, "Load playlist finished" )
    mPlayListIndex = 0;
}

void
CastMediaPlayer::loadMediaFromPlaylist()
{
    if (!verifyPlaylist()) return;

    RLOG_N( "Load Media #" << mPlayListIndex << " - " << mPlayList[mPlayListIndex]  )

    mediaLoad( mPlayList[mPlayListIndex] );
}

void
CastMediaPlayer::verifyMediaConnection()
{
    if (!mCastLink || !mCastLink->isConnected())
    {
        //mCastLink = std::shared_ptr<CastLink>( new CastLink(mHost, mPort) );
        mCastLink.reset( new CastLink(mHost, mPort) );
        mCastLink->addCallback(&mReceiverHandler);
        mCastLink->addCallback(&mMediaHandler);

    }

    if (mReceiverHandler.sessionId() == "")
    {
        receiverLaunch(castrReceiverApp);
    }
}

uint32_t
CastMediaPlayer::getNextRequestId()
{
    return ++mRequestId;
}

void
CastMediaPlayer::playOrPause()
{
    RLOG_N( "PLAY/PAUSE" )
    if (!verifyPlaylist()) return;

    verifyMediaConnection();

    if (mMediaHandler.playerState() == PlayerState::BUFFERING
        || mMediaHandler.playerState() == PlayerState::PLAYING)
    {
        mediaPause();
    }
    else if (mMediaHandler.playerState() == PlayerState::PAUSED)
    {
        mediaPlay();
    }
    else    // IDLE
    {
        loadMediaFromPlaylist();
    }
}

void
CastMediaPlayer::stop()
{
    RLOG_N( "STOP" )

    if (mReceiverHandler.sessionId() != "")
    {
        receiverStop();
        mReceiverHandler.reset();
        mMediaHandler.reset();
    }

    mRequestId = 0;
    mCastLink.reset();  // Drop connection to CC when we stop, and connect again when needed

}

void
CastMediaPlayer::next()
{
    RLOG_N( "NEXT" )
    if (!verifyPlaylist()) return;

    mPlayListIndex = (mPlayListIndex + 1) % mPlayList.size();

    loadMediaFromPlaylist();
}

void
CastMediaPlayer::previous()
{
    RLOG_N( "PREVIOUS" )
    if (!verifyPlaylist()) return;

    if( mPlayListIndex == 0 )
    {
        mPlayListIndex = mPlayList.size() -1;
    }
    else
    {
        --mPlayListIndex;
    }

    loadMediaFromPlaylist();
}

void
CastMediaPlayer::setSeekEnabledMode(SeekEnabledMode mode)
{
    mSeekMode = mode;
}

static bool
isStreamingMedia(const std::string url)
{
    std::string fileExtension = ascii_to_lower(split(url, '.').back());

    // DASH or HLS streams
    if (found_in({"mpd", "m3u8"}, fileExtension))
    {
        return true;
    }

    return false;
}

static bool
mediaSupportsSeeking(const MediaStatus& mediaStatus)
{
    if (mediaStatus.seekRangeEnd > 0)
    {
        return true;
    }
    if (isStreamingMedia(mediaStatus.contentId))
    {
        return true;
    }

    return false;
}

bool
CastMediaPlayer::seekEnabled()
{
    bool ret = false;
    if (mSeekMode == SeekEnabledMode::Always)
    {
        ret = true;
    }
    if (mSeekMode == SeekEnabledMode::StreamingOnly
        && mediaSupportsSeeking(mediaStatus()))
    {
        ret = true;
    }

    RLOG(rlog::Debug, "seekEnabled " << mediaStatus().contentId << " " << ret )
    return ret;
}

void
CastMediaPlayer::seek(double targetTime)
{
    if (!seekEnabled()) { return; }

    RLOG_N( "SEEK " << targetTime )

    if (targetTime<0)
    {
        targetTime = 0;   // Negative times does not make sense
    }
    // If we know the duration, use that as upper limit
    if (mediaStatus().duration > 0
        && targetTime > mediaStatus().duration)
    {
        targetTime = mediaStatus().duration;
    }

    // If we got a "seek range", use that range as limits
    double seekRangeStart = mediaStatus().seekRangeStart;
    double seekRangeEnd = mediaStatus().seekRangeEnd;
    if (seekRangeStart >= 0
        && targetTime < seekRangeStart)
    {
        targetTime = seekRangeStart;
        RLOG_N( "Seek limited by seekRangeStart to " << targetTime )
    }
    if (seekRangeEnd >= 0
        && targetTime > seekRangeEnd)
    {
        targetTime = seekRangeEnd;
        RLOG_N( "Seek limited by seekRangeEnd to " << targetTime )
    }

    verifyMediaConnection();
    mediaSeek(targetTime);

}

void
CastMediaPlayer::seekDiff(double timeDiff)
{
    double targetTime = mediaStatus().currentTime + timeDiff;

    seek(targetTime);
}

void
CastMediaPlayer::increaseVolumeLevel()
{
    double level = mReceiverHandler.receiverStatus().volumeLevel + 0.1;
    if (level>1.0) { level = 1.0; }

    RLOG_N( "Increase volume " << level )
    verifyMediaConnection();

    receiverSetVolume(level);
}

void
CastMediaPlayer::decreaseVolumeLevel()
{
    double level = mReceiverHandler.receiverStatus().volumeLevel - 0.1;
    if (level<0.0) { level = 0.0; }

    RLOG_N( "Decrease volume " << level )
    verifyMediaConnection();

    receiverSetVolume(level);
}

void
CastMediaPlayer::setVolumeLevel(double level)
{
    RLOG_N( "Set volume " << level )
    verifyMediaConnection();

    receiverSetVolume(level);
}

void
CastMediaPlayer::toggleVolumeMuted()
{
    bool newMutedStatus = !receiverStatus().volumeMuted;
    RLOG_N( "Toggle muted " << newMutedStatus )
    verifyMediaConnection();

    receiverSetMuted(newMutedStatus);
}


void
CastMediaPlayer::getStatus()
{
    RLOG(rlog::Debug, "CastMediaPlayer::getStatus" )

    if (!mCastLink)
    {
        RLOG(rlog::Debug, "getStatus ignored, no cast link" )
        return;
    }

    auto castMessage = getMediaCastMessage();

    Json::Value statusPayload;
    statusPayload["requestId"] = getNextRequestId();
    statusPayload["type"] = "GET_STATUS";

    castMessage.set_payload_utf8( getJsonString(statusPayload) );
    mCastLink->send(castMessage);
}

void
CastMediaPlayer::startStatusTimer(int timeout_ms)
{
    RLOG(rlog::Debug, "CastMediaPlayer::startStatusTimer timeout_ms=" << timeout_ms )

    if (timeout_ms<=0)
    {
        RLOG(rlog::Debug, "startStatusTimer poll once" )
        getStatus();
        return;
    }

    mStatusThreadTimeout = timeout_ms;

    mStatusThread.reset( new std::thread(
        [this]()
        {
            while (this->mStatusThreadTimeout > 0)
            {
                this->getStatus();
                std::this_thread::sleep_for(std::chrono::milliseconds(this->mStatusThreadTimeout));
            }

            RLOG(rlog::Debug, "CastMediaPlayer status thread stopped")
        }
    ));

}

void
CastMediaPlayer::stopStatusTimer()
{
    if (!mStatusThread) { return; }

    RLOG(rlog::Debug, "CastMediaPlayer::stopStatusTimer")

    mStatusThreadTimeout = 0;
    mStatusThread->join();
    mStatusThread.reset();

    RLOG(rlog::Debug, "CastMediaPlayer::stopStatusTimer finished")
}


extensions::api::cast_channel::CastMessage
CastMediaPlayer::getMediaCastMessage()
{
    extensions::api::cast_channel::CastMessage mediaCastMessage =
        getCastMessage( CastLink::sDefaultSender,
                        mReceiverHandler.transportId().c_str(),
                        MediaHandler::sNameSpace);

    return mediaCastMessage;
}

extensions::api::cast_channel::CastMessage
CastMediaPlayer::getReceiverCastMessage()
{
    extensions::api::cast_channel::CastMessage receiverCastMessage =
        getCastMessage( CastLink::sDefaultSender,
                        CastLink::sDefaultReceiver,
                        ReceiverHandler::sNameSpace);

    return receiverCastMessage;
}

void
CastMediaPlayer::receiverLaunch(const std::string& receiverApp)
{
    RLOG(rlog::Verbose, "CastMediaPlayer::receiverLaunch " << receiverApp )

    auto castMessage = getReceiverCastMessage();
    std::string payload = getLaunchReceiverPayload(getNextRequestId(), receiverApp);
    castMessage.set_payload_utf8(payload);

    mCastLink->send(castMessage);

    while (mReceiverHandler.latestRequestId() != mRequestId){ ; }  // Wait for response

    mCastLink->addDestination( mReceiverHandler.transportId() );

}


void
CastMediaPlayer::mediaLoad( const std::string& mediaUrl )
{
    RLOG(rlog::Verbose, "CastMediaPlayer::mediaLoad " << mediaUrl )

    auto castMessage = getMediaCastMessage();
    std::string payload = getMediaLoadPayload(getNextRequestId(), mediaUrl);
    castMessage.set_payload_utf8( payload );

    mCastLink->send(castMessage);
}

void
CastMediaPlayer::mediaPlay()
{
    RLOG(rlog::Verbose, "CastMediaPlayer::mediaPlay " )

    auto castMessage = getMediaCastMessage();
    std::string payload = getMediaPlayPayload(getNextRequestId(), mMediaHandler.mediaSessionId());
    castMessage.set_payload_utf8( payload );

    mCastLink->send(castMessage);
}

void
CastMediaPlayer::mediaPause()
{
    RLOG(rlog::Verbose, "CastMediaPlayer::mediaPause " )

    auto castMessage = getMediaCastMessage();
    std::string payload = getMediaPausePayload(getNextRequestId(), mMediaHandler.mediaSessionId());
    castMessage.set_payload_utf8( payload );

    mCastLink->send(castMessage);
}

void
CastMediaPlayer::mediaSeek(double time)
{
    RLOG(rlog::Verbose, "CastMediaPlayer::mediaSeek " )

    auto castMessage = getMediaCastMessage();
    std::string payload = getMediaSeekPayload(getNextRequestId(), mMediaHandler.mediaSessionId(), time);
    castMessage.set_payload_utf8( payload );

    mCastLink->send(castMessage);
}


void
CastMediaPlayer::receiverSetVolume(double level)
{
    RLOG(rlog::Verbose, "CastMediaPlayer::receiverSetVolume " )

    auto castMessage = getReceiverCastMessage();
    std::string payload = getReceiverSetVolumeLevelPayload(getNextRequestId(), level);
    castMessage.set_payload_utf8( payload );

    mCastLink->send(castMessage);
}

void
CastMediaPlayer::receiverSetMuted(bool muted)
{
    RLOG(rlog::Verbose, "CastMediaPlayer::receiverSetVolume " )

    auto castMessage = getReceiverCastMessage();
    std::string payload = getReceiverSetVolumeMutedPayload(getNextRequestId(), muted);
    castMessage.set_payload_utf8( payload );

    mCastLink->send(castMessage);
}

void
CastMediaPlayer::receiverStop()
{
    RLOG(rlog::Verbose, "CastMediaPlayer::receiverStop " )

    auto castMessage = getReceiverCastMessage();

    Json::Value statusPayload;
    statusPayload["requestId"] = getNextRequestId();
    statusPayload["type"] = "STOP";
    statusPayload["sessionId"] = mReceiverHandler.sessionId();

    castMessage.set_payload_utf8( getJsonString(statusPayload) );
    mCastLink->send(castMessage);

    // TODO wait for response? remove transportId from cast link
    mReceiverHandler.reset();
}

bool
CastMediaPlayer::verifyPlaylist()
{
    if (mPlayList.size() == 0)
    {
        RLOG(rlog::Critical, "Playlist Error: Playlist size=0" )
        return false;
    }
    if (mPlayListIndex >= mPlayList.size())
    {
        RLOG(rlog::Critical, "Playlist Error: Playlist size=" << mPlayList.size()
                << ", index=" << mPlayListIndex )
        return false;
    }

    return true;
}

