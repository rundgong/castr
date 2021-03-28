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

#include "rlog/RLog.h"
#include <iostream>
#include <fstream>

namespace rlog
{
    LogLevel logLevel = Normal;
    bool networkLogEnabled = false;

    std::ostream* logStream = &std::cout;

static std::ofstream* sFileLogStream = 0;

bool
parseArgument(const std::string& arg)
{
    if (arg == "--log=on" )   { rlog::logLevel = rlog::Normal; }
    else if (arg == "--log=off" )  { rlog::logLevel = rlog::Off;}
    else if (arg.substr(0,11) == "--log-file=" )
    {
        std::string logFileName = arg.substr(11);
        sFileLogStream = new std::ofstream(logFileName);
        rlog::logStream = sFileLogStream;
    }
    else if (arg == "--log-level=off" )        { rlog::logLevel = rlog::Off; }
    else if (arg == "--log-level=critical" )   { rlog::logLevel = rlog::Critical; }
    else if (arg == "--log-level=important" )  { rlog::logLevel = rlog::Important; }
    else if (arg == "--log-level=normal" )     { rlog::logLevel = rlog::Normal; }
    else if (arg == "--log-level=verbose" )    { rlog::logLevel = rlog::Verbose; }
    else if (arg == "--log-level=debug" )      { rlog::logLevel = rlog::Debug; }
    else if (arg == "--log-level=all" )        { rlog::logLevel = rlog::All; }
    else if (arg == "--log-network" )          { rlog::networkLogEnabled = true; }
    else { return false;}

    return true;
}

std::string
logHelp()
{
    std::string help("  --log=on|off - same effect as writing --log-level=normal|off\n");
    help += "  --log-level=off|critical|important|normal|verbose|debug|all\n";
    help += "  --log-network\n";
    help += "  --log-file=<filename>\n";

    return help;
}

void closeFile()
{
    if( sFileLogStream != 0 )
    {
        rlog::logStream = &std::cout;   // Make sure nothing accidentally logs to file after it is deleted
        delete sFileLogStream;
    }
}

}

