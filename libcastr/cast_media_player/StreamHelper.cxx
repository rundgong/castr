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

#include "cast_media_player/StreamHelper.h"
#include "rlog/RLog.h"

StreamStartHelper::StreamStartHelper()
{
}

void
StreamStartHelper::setPlayer(CastMediaPlayer* player)
{
    mCastMediaPlayer = player;
}


void
StreamStartHelper::onMediaStatusUpdate( const MediaStatus& mediaStatus )
{
    // Has a new media stream been loaded
    if (mMediaStatus.contentId != mediaStatus.contentId 
        && mediaStatus.contentId != "")
    {
        mStreamRestarted = false;
    }

    // Find first media update with a real time in it.
    if (!mStreamRestarted
        && mCastMediaPlayer != nullptr
        && mediaStatus.playerState == PlayerState::PLAYING
        && mediaStatus.currentTime > 0.0)
    {
        RLOG(rlog::Verbose, "StreamStartHelper first update at time " << mediaStatus.currentTime)

        // Guess that we did not start from the beginning
        // if first update is more than 5 seconds in.
        if (mediaStatus.currentTime > 5.0)
        {
            mCastMediaPlayer->seek(0);
        }

        mStreamRestarted = true;
    }

    mMediaStatus = mediaStatus;
}
