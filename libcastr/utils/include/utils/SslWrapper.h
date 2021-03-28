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

#ifndef SSLWRAPPER_H_
#define SSLWRAPPER_H_

#include <stdint.h>
#include <string>

#include <openssl/ssl.h>

class SslWrapper
{
public:
    SslWrapper(const std::string& host, uint16_t port = 0);    // host may be "host.domain" or "host.domain:port"
    ~SslWrapper();

    int read( uint8_t* buffer, size_t bufferSize );
    int write( uint8_t* buffer, size_t bufferSize );
    void closeConnection();

private:
    SSL_CTX* mSslCtx;
    BIO *mSslBio;
    SSL *mSsl;

    std::string mHostPort;
};



#endif /* SSLWRAPPER_H_ */
