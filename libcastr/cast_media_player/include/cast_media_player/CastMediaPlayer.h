#pragma once

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

#include <string>
#include "CastLink.h"
#include "ReceiverHandler.h"
#include "MediaHandler.h"

// Seeking to other timestamps in media is usually only supported
// in "streaming" media where a video is downloaded in multiple
// chunks instead of as one big file.
// Files are likely to re-start from the beginning if you try to seek
// in a file that does not support it.
enum class SeekEnabledMode
{
    Never = 0,          // Seeking disabled for all media
    Always = 1,         // Seeking enabled for all media
    StreamingOnly = 2   // Seeking enabled for streams. Disabled for regular files
};

class CastMediaPlayer : public MediaFinishedCallBack
{
public:
    CastMediaPlayer(const std::string& host, 
                    uint16_t port = 8009,
                    const std::string& playListFileName = "");
    ~CastMediaPlayer();

    void playOrPause();
    void stop();
    void next();
    void previous();

    void setSeekEnabledMode(SeekEnabledMode mode);
    bool seekEnabled();             // Seeking enabled for currently playing media
    void seek(double targetTime);   // Seek to absolute time
    void seekDiff(double timeDiff); // Seek relative to current time

    void increaseVolumeLevel();
    void decreaseVolumeLevel();
    void setVolumeLevel(double level);
    void toggleVolumeMuted();

    const ReceiverStatus& receiverStatus(){ return mReceiverHandler.receiverStatus(); }
    const MediaStatus& mediaStatus(){ return mMediaHandler.mediaStatus(); }
    void getStatus();   // Request status update from chromecast device

    // Start periodic call to getStatus
    // Fetching status too often appear to give strange values  back
    void startStatusTimer(int timeout_ms = 2000);
    void stopStatusTimer();

    void setPlayList(std::vector<std::string> playList);
    void addMediaStatusCallBack(MediaStatusCallBack* callback);
    void addReceiverStatusCallBack(ReceiverStatusCallBack* callback);

    void onMediaFinished() override;

private:

    void loadPlayList(const std::string& playListFileName);
    void loadMediaFromPlaylist();
    void verifyMediaConnection();

    uint32_t getNextRequestId();

    extensions::api::cast_channel::CastMessage getMediaCastMessage();
    extensions::api::cast_channel::CastMessage getReceiverCastMessage();

    void receiverLaunch(const std::string& receiverApp);
    void receiverStop();

    void mediaLoad(const std::string& mediaUrl);
    void mediaPlay();
    void mediaPause();
    void mediaSeek(double time);
    void receiverSetVolume(double level);
    void receiverSetMuted(bool muted);

    bool verifyPlaylist();

    std::shared_ptr<CastLink> mCastLink;
    ReceiverHandler mReceiverHandler;
    MediaHandler mMediaHandler;

    std::shared_ptr<std::thread> mStatusThread;
    int mStatusThreadTimeout = 0;

    std::vector<std::string> mPlayList;
    std::size_t mPlayListIndex;

    std::string mHost;
    uint16_t mPort;

    SeekEnabledMode mSeekMode = SeekEnabledMode::StreamingOnly;

    uint32_t mRequestId;
};


