/*
 *  Copyright (c) 2015, AirBitz, Inc.
 *  All rights reserved.
 *
 * See the LICENSE file for more information.
 */

#include "TcpConnection.hpp"
#include <openssl/err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

namespace abcd {

Status
initSSL()
{
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    return Status();
}

/**
 * Creates a non-blocking socket and attempts to connect it.
 * @param timeout milliseconds to wait for the server.
 */
static int
socketConnect(struct addrinfo *target, int timeout)
{
    int fd = socket(target->ai_family, target->ai_socktype,
                    target->ai_protocol);
    if (fd < 0)
        return -1;

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
        close(fd);
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        close(fd);
        return -1;
    }

    if (0 == connect(fd, target->ai_addr, target->ai_addrlen))
        return fd;

    struct pollfd pfd = {fd, POLLOUT, 0};
    if (poll(&pfd, 1, timeout) < 1)
    {
        close(fd);
        return -1;
    }

    int so_error;
    socklen_t len = sizeof(so_error);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
    if (so_error)
    {
        close(fd);
        return -1;
    }

    return fd;
}

TcpConnection::~TcpConnection()
{
    if (0 < fd_)
        close(fd_);
    if (ssl_)
        SSL_free(ssl_);
    if (ctx_)
        SSL_CTX_free(ctx_);
}

TcpConnection::TcpConnection():
    fd_(0),
    ctx_(nullptr),
    ssl_(nullptr)
{
}

Status
TcpConnection::connect(const std::string &hostname, unsigned port, bool ssl)
{
    // Do the DNS lookup:
    struct addrinfo hints {};
    struct addrinfo *list = nullptr;
    hints.ai_family = AF_UNSPEC; // Allow IPv6 or IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP only
    if (getaddrinfo(hostname.c_str(), std::to_string(port).c_str(), &hints, &list))
        return ABC_ERROR(ABC_CC_ServerError, "Cannot look up " + hostname);

    // Try the returned DNS entries until one connects:
    for (struct addrinfo *p = list; p; ++p)
    {
        auto fd = socketConnect(p, 10000);
        if (fd)
            break;
    }
    if (fd_ < 0)
        return ABC_ERROR(ABC_CC_ServerError, "Cannot connect to " + hostname);

    if(ssl)
    {
        // Setup SSL
        ABC_CHECK(initSSLContext());
        SSL_set_fd(ssl_, fd_);
        if (1 != SSL_connect(ssl_) || !SSL_get_peer_certificate(ssl_))
            return ABC_ERROR(ABC_CC_ServerError, "SSL connection to " +
                             hostname + " failed.");
    }
    return Status();
}

Status
TcpConnection::send(DataSlice data)
{
    int bytes = 0;
    while (data.size())
    {
        if(ssl_)
            bytes = SSL_write(ssl_, data.data(), data.size());
        else
            bytes = ::send(fd_, data.data(), data.size(), 0);

        if (bytes < 0)
            return ABC_ERROR(ABC_CC_ServerError, "Failed to send");

        data = DataSlice(data.data() + bytes, data.end());
    }

    return Status();
}

Status
TcpConnection::read(DataChunk &result)
{
    ssize_t bytes = 0;
    unsigned char data[1024];

    if(ssl_)
    {
        if(true || SSL_pending(ssl_))
        {
            bytes = SSL_read(ssl_, data, sizeof(data));
            auto sslError = SSL_get_error(ssl_, bytes);
            switch (sslError)
            {
            case SSL_ERROR_NONE:
                break;
            case SSL_ERROR_WANT_READ:
                return ABC_ERROR(ABC_CC_ServerError, "SSL_ERROR_WANT_READ");
                break;
            case SSL_ERROR_WANT_WRITE:
                return ABC_ERROR(ABC_CC_ServerError, "SSL_ERROR_WANT_WRITE");
                break;
            case SSL_ERROR_ZERO_RETURN:
                return ABC_ERROR(ABC_CC_ServerError, "SSL_ERROR_ZERO_RETURN");
                break;
            default:
                break;
            }
        }
    }
    else
    {
        bytes = recv(fd_, data, sizeof(data), MSG_DONTWAIT);
        if (bytes < 0)
        {
            if (EAGAIN != errno && EWOULDBLOCK != errno)
                return ABC_ERROR(ABC_CC_ServerError, "Cannot read from socket");

            // No data, but that's fine:
            bytes = 0;
        }
    }

    result = DataChunk(data, data + bytes);
    return Status();
}

Status
TcpConnection::initSSLContext()
{
    auto method = SSLv23_method(); // Highest available SSL/TLS version
    ctx_ = SSL_CTX_new(method);
    if (!ctx_)
        return ABC_ERROR(ABC_CC_ServerError, "Cannot initialize SSL context");

    ssl_ = SSL_new(ctx_);
    if (!ssl_)
        return ABC_ERROR(ABC_CC_ServerError, "Cannot initialize SSL");

    return Status();
}

} // namespace abcd
