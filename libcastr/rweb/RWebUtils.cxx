/*
    Copyright 2019-2021 rundgong

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

#include "rweb/RWebUtils.h"


bool
found_in(const std::vector<std::string>& haystack,
         const std::string& needle)
{
    for (auto x : haystack)
    {
        if (needle==x) return true;
    }

    return false;
}


bool
ends_with(const std::string& haystack,
          const std::string& needle )
{
    if (needle.size() > haystack.size()) { return false; }

    std::string suffix(haystack, haystack.size()-needle.size());
    if (suffix==needle)
    {
        return true;
    }

    return false;
}

std::vector<std::string>
split(const std::string &str, char delim)
{
    std::size_t first = 0, last = 0;
    std::vector<std::string> result;
    while (first < str.size())
    {
        last = str.find(delim, first);
        if (last == std::string::npos)
        {
            result.push_back(str.substr(first));
            break;
        }
        else
        {
            result.push_back(str.substr(first, last-first));
        }
        first = last + 1;
    }

    // If last character is delimiter, then add empty string
    // so "::" returns as many items as "a:a:a"
    if (str[str.size()-1] == delim)
    {
        result.push_back("");
    }

    return result;
}

std::string
ascii_to_lower(const std::string& str)
{
    std::string result;

    for (char x : str)
    {
        if (x>='A' && x<='Z')
        {
            result.push_back(x - ('A'-'a'));
        }
        else
        {
            result.push_back(x);
        }
    }

    return result;
}

std::vector<ExtensionMimeType> extensionMimeTypes = {
    {"aac",  "audio/mp4" },
    {"mp3",  "audio/mp3" },
    {"wav",  "audio/wav" },
    {"webm", "video/webm" },
    {"mp4",  "video/mp4" },
    {"m4s",  "video/mp4" },
    {"mpd",  "application/dash+xml" },
    {"m3u8", "application/x-mpegurl" },
    {"ts",   "video/MP2T" },
    {"js",   "application/javascript" },
    {"gif",  "image/gif" },
    {"bmp",  "image/bmp" },
    {"jpg",  "image/jpg" },
    {"jpeg", "image/jpeg"},
    {"png",  "image/png" },
    {"webp", "image/webp" },
    {"ico",  "image/ico" },
    {"zip",  "image/zip" },
    {"gz",   "image/gz"  },
    {"tar",  "image/tar" },
    {"txt",  "text/plain" },
    {"htm",  "text/html" },
    {"html", "text/html" }
    };

std::string
extension_to_mime_type(const std::string& filename)
{
    std::string mimeType;
    for (ExtensionMimeType& emt : extensionMimeTypes)
    {
        if (ends_with(ascii_to_lower(filename), emt.fileExtension))
        {
            mimeType = emt.mimeType;
            break;
        }
    }

    return mimeType;
}


