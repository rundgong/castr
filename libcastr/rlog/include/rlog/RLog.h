/*
    Copyright 2015-2021 rundgong

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

#ifndef RCAST_RLOG_H_
#define RCAST_RLOG_H_

#include <ostream>
#include <string>

namespace rlog
{
    enum LogLevel
    {
        Off = 0,
        Critical,
        Important,
        Normal,
        Verbose,
        Debug,
        All
    };

    extern LogLevel logLevel;
    extern bool networkLogEnabled;

    // Point this to any ostream to get logging there. Default std::cout
    extern std::ostream* logStream;

    bool parseArgument(const std::string& arg);
    std::string logHelp();
    void closeFile();
}

#define RLOG(_logLevel, _message)\
if( rlog::logLevel >= _logLevel )\
{\
    *rlog::logStream << _message << std::endl;\
}

#define RLOG_N(_message) RLOG(rlog::Normal, _message)

#define RLOG_NETWORK(_message)\
if( rlog::networkLogEnabled )\
{\
    *rlog::logStream << _message << std::endl;\
}


#endif /* RCAST_RLOG_H_ */
