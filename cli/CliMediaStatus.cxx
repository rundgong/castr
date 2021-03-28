/*
    Copyright 2020-2021 rundgong

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

#include "CliMediaStatus.h"
#include "fmt/core.h"
#include "rweb/RWebUtils.h"

static std::string progress_bar(int length, double progressValue)
{
    if (progressValue < 0.0) progressValue = 0.0;
    if (progressValue > 1.0) progressValue = 1.0;

    int internalLength = length - 2;   // space for border
    int consumedBars = internalLength * progressValue;

    std::string out = fmt::format("[{}{}]", 
                        std::string(consumedBars,'#'),
                        std::string(internalLength - consumedBars,'_'));
    return out;
}

void
CliMediaStatus::onMediaStatusUpdate( const MediaStatus& mediaStatus )
{
    mMediaStatus = mediaStatus;
    updateUI();
}

void CliMediaStatus::updateUI()
{
    if (mMediaStatus.contentId != mContentId 
        && mMediaStatus.contentId != "")
    {
        mContentId = mMediaStatus.contentId;
        std::cout << "\n----------------\n    Playing "
                  << split(mContentId, '/').back() << "\n\n";
    }

    // castErrorCode signifies an error outside of the media status.
    // MEDIA_UNKNOWN usually means some decoder error due to a video stream
    // that is incompatible with the Chromecast.
    if (mMediaStatus.castErrorCode != 0 && mMediaStatus.castErrorCode != mErrorCode)
    {
        mErrorCode = mMediaStatus.castErrorCode;
        std::cout << "\nCast Error at time " << mMediaStatus.currentTime << " seconds"  << std::endl;
        std::cout << "    Error code: " << mErrorCode << std::endl;
        if (mErrorCode == (int)CastErrorCodes::MEDIA_UNKNOWN)
        {
            std::cout << "    Unknown Media Error. Probably an incompatible video stream.\n";
        }
        if (mErrorCode == (int)CastErrorCodes::MEDIA_DECODE)
        {
            std::cout << "    Media Decode Error.\n";
        }
        if (mErrorCode == (int)CastErrorCodes::MEDIA_NETWORK)
        {
            std::cout << "    Network Error.\n";
        }
    }

    std::string playerStateString = to_string(mMediaStatus.playerState);

    if (mMediaStatus.playerState == PlayerState::IDLE)
    {
        playerStateString = to_string(mMediaStatus.idleReason);
    }

    // Clear line by overwriting previous text with spaces
    std::string clearLineString(mPreviousOutputLength, ' ');
    std::cout << "\r" << clearLineString << "\r";

    // Streaming media may not have a known duration.
    // But it should be at least as long as the end of
    // the "liveSeekableRange", so try that when no duration.
    double duration = mMediaStatus.duration;
    if (duration < 0 && mMediaStatus.seekRangeEnd > 0)
    {
        duration = mMediaStatus.seekRangeEnd;
    }
    if (duration < 0)
    {
        duration = 0;
    }

    double progress = 0;
    if (duration != 0)
    {
        progress = mMediaStatus.currentTime/duration;
    }

    std::string unfinishedIndicator = "";
    if (mMediaStatus.seekRangeEnd > 0 && !mMediaStatus.streamIsFinished)
    {
        unfinishedIndicator = "*";
    }

    std::string str = fmt::format(
        "{} | {}:{:02} {} {}:{:02}{} | Vol: {}% {}",
        playerStateString,
        (int)mMediaStatus.currentTime/60,
        (int)mMediaStatus.currentTime%60,
        progress_bar(50, progress),
        (int)duration/60,
        (int)duration%60,
        unfinishedIndicator,
        (int)(mReceiverStatus.volumeLevel*100.0),
        mReceiverStatus.volumeMuted ? "MUTED" : "");

    mPreviousOutputLength = str.size();
    std::cout << str;
    std::cout.flush();
}

void
CliMediaStatus::onReceiverStatusUpdate( const ReceiverStatus& receiverStatus )
{
    mReceiverStatus = receiverStatus;
    updateUI();
}
