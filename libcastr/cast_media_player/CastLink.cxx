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

#include "cast_media_player/CastLink.h"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <iomanip>
#include <stdexcept>
#include "rlog/RLog.h"

extensions::api::cast_channel::CastMessage
getCastMessage(const char* sourceId, const char* destinationId, const char* nameSpace)
{
    extensions::api::cast_channel::CastMessage castMessage;

    castMessage.set_protocol_version(extensions::api::cast_channel::CastMessage_ProtocolVersion_CASTV2_1_0);
    castMessage.set_source_id(sourceId);
    castMessage.set_destination_id(destinationId);
    castMessage.set_namespace_(nameSpace);
    castMessage.set_payload_type(extensions::api::cast_channel::CastMessage_PayloadType_STRING);

    return castMessage;
}


int
HeartBeatHandler::onCastMessage(CastLink* castLink, const CastMessage& castMessage )
{
    RLOG(rlog::Verbose, "HeartBeatHandler::onCastMessage")

    // TODO check if PING?
    // Sending unsolicited PONG seems to have no negative side effects.

    CastMessage pongMessage = getCastMessage( CastLink::sDefaultSender,
                                              CastLink::sDefaultReceiver,
                                              nameSpace().c_str());

    pongMessage.set_payload_utf8( R"({"type":"PONG"})");

    castLink->send(pongMessage);

    return 0;
}

int
ConnectionHandler::onCastMessage(CastLink* castLink, const CastMessage& castMessage )
{
    RLOG(rlog::Verbose, "ConnectionReceiver::onCastMessage")

    return 0;
}



CastLink::CastLink(const std::string& host, uint16_t port)
  : mIsConnected(false)
{
    RLOG(rlog::Debug, "CastLink::CastLink " << host << ":" << port )
    mSslWrapper = std::shared_ptr<SslWrapper>(new SslWrapper(host, port));

    init();

    addDestination(sDefaultReceiver);
}

void
CastLink::init()
{
    RLOG(rlog::Debug, "CastLink::init" )

    mIsConnected = true;

    mReceiverThread.reset( new std::thread(
            [this]()
            {
                this->receiverLoop();
            }
        ));

    addCallback(&mHeartBeatHandler);
    addCallback(&mConnectionHandler);

}

void CastLink::addDestination(const std::string& destination )
{
    RLOG(rlog::Verbose, "CastLink::addDestination " << destination )

    char connectPayload[] = R"({ "type": "CONNECT" })";

    extensions::api::cast_channel::CastMessage connectionMessage =
        getCastMessage(sDefaultSender, destination.c_str(), ConnectionHandler::sNameSpace );

    connectionMessage.set_payload_utf8(connectPayload);

    send(connectionMessage);

}

CastLink::~CastLink()
{
    RLOG(rlog::Debug, "CastLink::~CastLink begin" )

    mSslWrapper->closeConnection();
    mIsConnected = false;
    mReceiverThread->join();
    RLOG(rlog::Debug, "CastLink::~CastLink end" )
}

enum ReceiverState
{
    rs_readMessageLength,
    rs_readMessagePayload
};

uint32_t CastLink::readMessageLength()
{
    uint32_t messageSize, messageSizeNBO;
    int len;

    len = mSslWrapper->read((uint8_t*)(&messageSizeNBO), sCastHeaderLength);
    if (len != sCastHeaderLength)
    {
        throw std::runtime_error("readMessageLength error");
    }
    messageSize = ntohl(messageSizeNBO);

    return messageSize;
}

void CastLink::readPayload( std::vector<uint8_t>&  messageBuffer )
{
    int len;

    len = mSslWrapper->read(& messageBuffer[0], messageBuffer.size() );
    if (len != (int)messageBuffer.size())
    {
        throw std::runtime_error("readPayload error");
    }
}

void CastLink::receiverLoop()
{

    uint32_t messageLength;
    std::vector<uint8_t> messageBuffer;
    extensions::api::cast_channel::CastMessage receivedCastMessage;
    std::string responsePayload;

    RLOG(rlog::Debug, "CastLink::receiverLoop begin" )

    try
    {
        while (mIsConnected)
        {
            messageLength = readMessageLength();

            messageBuffer.resize(messageLength);
            readPayload(messageBuffer);

            receivedCastMessage.ParseFromArray(&messageBuffer[0], messageLength);

            // Only log heartbeat when we have set Verbose or higher log level
            if (receivedCastMessage.namespace_() != HeartBeatHandler::sNameSpace
              || rlog::logLevel >= rlog::Verbose)
            {
                RLOG_NETWORK( "\nCastLink receive messageLength=" << messageLength
                          << " - (" << receivedCastMessage.source_id()
                          << "/" << receivedCastMessage.destination_id() << ") - "
                          << receivedCastMessage.namespace_() << std::endl
                          << receivedCastMessage.payload_utf8() )
            }

            dispatchCastMessage(receivedCastMessage);

            usleep(1000000);
        }
    }
    catch( std::runtime_error& e )
    {
        // This usually means the TLS socket has been disconnected
        RLOG(rlog::Debug, "CastLink::receiverLoop exception: " << e.what() )
        mIsConnected = false;
    }
    RLOG(rlog::Debug, "CastLink::receiverLoop done" )

    return;
}

void CastLink::dispatchCastMessage(const CastMessage& castMessage)
{
    RLOG(rlog::Debug, "CastLink::dispatchCastMessage (" << castMessage.source_id()
              << "/" << castMessage.destination_id() << ") - "
              << castMessage.namespace_() )
    for (auto& castCallback : mCastCallbacks)
    {
        if (castCallback.nameSpace == castMessage.namespace_())
        {
            castCallback.receiverPtr->onCastMessage(this, castMessage);
        }
    }
}

void CastLink::send(const CastMessage& castMessage)
{
    // TODO - Not yet multi-thread safe
    static std::vector<uint8_t> sendBuffer;

    int ret;
    uint32_t messageSize, messageSizeNBO;
    messageSize = castMessage.ByteSize();
    messageSizeNBO = htonl(messageSize);

    // Only log heartbeat when we have set Verbose or higher log level
    if (castMessage.namespace_() != HeartBeatHandler::sNameSpace
      || rlog::logLevel >= rlog::Verbose)
    {
        RLOG_NETWORK( "\nCastLink::send messageSize=" << messageSize
                  << " - (" << castMessage.source_id()
                  << "/" << castMessage.destination_id() << ") - "
                  << castMessage.namespace_() << std::endl
                  << castMessage.payload_utf8() )
    }

    sendBuffer.resize( sCastHeaderLength + messageSize );

    // First 4 bytes = length of message (uint32 in network byte order)
    // After this comes the actual message
    memmove(&sendBuffer[0], &messageSizeNBO, sCastHeaderLength);
    castMessage.SerializeWithCachedSizesToArray(&sendBuffer[0] + sCastHeaderLength);

    ret = mSslWrapper->write(&sendBuffer[0], sendBuffer.size());
    if (ret != (int)sendBuffer.size())
    {
        throw std::runtime_error("CastLink send error");
    }

}

void CastLink::addCallback( CastMessageHandler* receiverPtr)
{
    mCastCallbacks.push_back({receiverPtr->nameSpace(),receiverPtr});
}

bool CastLink::isConnected()
{
    return mIsConnected;
}
