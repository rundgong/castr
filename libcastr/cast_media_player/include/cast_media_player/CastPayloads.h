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
#include <cstdint>

std::string getLaunchReceiverPayload(uint32_t requestId, const std::string& appId);
std::string getMediaLoadPayload(uint32_t requestId, const std::string& videoUrl);
std::string getMediaPlayPayload(uint32_t requestId, uint32_t mediaSessionId);
std::string getMediaPausePayload(uint32_t requestId, uint32_t mediaSessionId);
std::string getMediaSeekPayload(uint32_t requestId, uint32_t mediaSessionId, double time);
std::string getReceiverSetVolumeLevelPayload(uint32_t requestId, double level);
std::string getReceiverSetVolumeMutedPayload(uint32_t requestId, bool muted);

