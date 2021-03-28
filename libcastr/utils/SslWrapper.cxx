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

#include "utils/SslWrapper.h"
#include <stdexcept>
#include <iostream>
#include <sstream>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/opensslconf.h>


static void initOpenSslLibrary();

void SSL_CHECK(bool pass_condition)
{
    if(!(pass_condition))
    {
        unsigned long ssl_err = ERR_get_error();
        const char* const str = ERR_reason_error_string(ssl_err);
        if(str)
        {
            throw std::runtime_error(str);
        }
        else
        {
            std::ostringstream oss;
            oss << "Unknown SSL error: " << ssl_err;
            throw std::runtime_error(oss.str());
        }
    }
}

SslWrapper::SslWrapper(const std::string& host, uint16_t port)
 : mSslCtx(0), mSslBio(0), mSsl(0), mHostPort(host)
{
    if( port != 0 )
    {
        std::ostringstream oss;
        oss << host << ":" << port;
        mHostPort = oss.str();
    }

    initOpenSslLibrary();

    int res = 0;

    const SSL_METHOD* method = TLSv1_2_client_method();
    SSL_CHECK( method != 0 );

    mSslCtx = SSL_CTX_new(method);
    SSL_CHECK( mSslCtx != 0 );

    // Only versions above TLS 1.0 allowed
    const long flags = SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1; // | SSL_OP_NO_COMPRESSION;
    SSL_CTX_set_options(mSslCtx, flags);

    mSslBio = BIO_new_ssl_connect(mSslCtx);
    SSL_CHECK( mSslBio != 0 );

    //BIO_set_nbio(mSslBio, 1);	// Set non-blocking I/O

    res = BIO_set_conn_hostname(mSslBio, mHostPort.c_str());
    SSL_CHECK( res==1 );

    BIO_get_ssl(mSslBio, &mSsl);
    SSL_CHECK( mSsl != 0 );

    res = BIO_do_connect(mSslBio);
    SSL_CHECK( res==1 );

    res = BIO_do_handshake(mSslBio);
    SSL_CHECK( res==1 );

}

SslWrapper::~SslWrapper()
{
    if( mSslBio != 0 )
    {
        BIO_free_all(mSslBio);
    }

    if( mSslCtx != 0 )
    {
        SSL_CTX_free(mSslCtx);
    }
}

int
SslWrapper::read( uint8_t* buffer, size_t bufferSize )
{
    int len;
    len = BIO_read(mSslBio, buffer, bufferSize);

    if( len<=0 && !BIO_should_retry(mSslBio) )
    {
        throw std::runtime_error("SslWrapper read error");
    }

    return len;
}

int
SslWrapper::write( uint8_t* buffer, size_t bufferSize )
{
    int len;
    len = BIO_write(mSslBio, buffer, bufferSize);

    return len;
}

void SslWrapper::closeConnection()
{
    BIO_ssl_shutdown(mSslBio);
}

static void initOpenSslLibrary()
{
    SSL_library_init();
    SSL_load_error_strings();
}

