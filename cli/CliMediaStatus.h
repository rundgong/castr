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

#pragma once

#include "cast_media_player/CastMediaPlayer.h"


class CliMediaStatus : public MediaStatusCallBack, public ReceiverStatusCallBack
{
public:
    virtual void onMediaStatusUpdate( const MediaStatus& mediaStatus ) override;
    virtual void onReceiverStatusUpdate ( const ReceiverStatus & mediaStatus ) override;

private:

    void updateUI();

    int mPreviousOutputLength = 0;
    std::string mContentId;
    int mErrorCode;

    MediaStatus mMediaStatus;
    ReceiverStatus mReceiverStatus;

};
