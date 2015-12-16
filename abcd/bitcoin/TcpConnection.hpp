/*
 *  Copyright (c) 2015, AirBitz, Inc.
 *  All rights reserved.
 *
 * See the LICENSE file for more information.
 */

#ifndef ABCD_BITCOIN_TCP_CONNECTION_HPP
#define ABCD_BITCOIN_TCP_CONNECTION_HPP

#include "../util/Status.hpp"
#include "../util/Data.hpp"
#include <openssl/ssl.h>

namespace abcd {

/**
 * Init SSL
 */
Status
initSSL();

class TcpConnection
{
public:
    ~TcpConnection();
    TcpConnection();

    /**
     * Connect to the specified server.
     */
    Status
    connect(const std::string &hostname, unsigned port, bool ssl=false);

    /**
     * Send some data over the socket.
     * Will block until all the data is sent.
     */
    Status
    send(DataSlice data);

    /**
     * Read all pending data from the socket (might not produce anything).
     */
    Status
    read(DataChunk &result);

    /**
     * Obtains a socket that the main loop should sleep on.
     */
    int pollfd() const { return fd_; }

private:
    /**
     * Init SSL Context for this connection
     */
    Status
    initSSLContext();

    int fd_;
    SSL_CTX *ctx_;
    SSL *ssl_;
};

} // namespace abcd

#endif
