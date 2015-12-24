/*
 * Copyright (c) 2015, AirBitz, Inc.
 * All rights reserved.
 *
 * See the LICENSE file for more information.
 */

#include "../Command.hpp"
#include "../../abcd/bitcoin/StratumConnection.hpp"
#include "../../abcd/http/Uri.hpp"
#include <iostream>

using namespace abcd;

static int done = 0;

COMMAND(InitLevel::context, Temp, "stratum-version",
        " <server>")
{
    if (1 != argc)
        return ABC_ERROR(ABC_CC_Error, helpString(*this));
    const auto rawUri = argv[0];

    Uri uri;
    if (!uri.decode(rawUri))
        return ABC_ERROR(ABC_CC_ParseError, "Bad URI - wrong format");

    bool ssl;
    if ("stratum" == uri.scheme())
        ssl = false;
    else if ("stratums" == uri.scheme())
        ssl = true;
    else
        return ABC_ERROR(ABC_CC_ParseError, "Bad URI - wrong scheme");

    auto server = uri.authority();
    auto last = server.find(':');
    if (std::string::npos == last)
        return ABC_ERROR(ABC_CC_ParseError, "Bad URI - no port");
    auto serverName = server.substr(0, last);
    auto serverPort = server.substr(last + 1, std::string::npos);

    // Connect to the server:
    StratumConnection c;
    ABC_CHECK(c.connect(serverName, atoi(serverPort.c_str())));
    std::cout << "Connection established" << std::endl;

    // Send the version command:
    auto onError = [](std::error_code ec)
    {
        ++done;
    };
    auto onReply = [](const std::string &version)
    {
        std::cout << "version: " << version << std::endl;
        ++done;
        return Status();
    };
    c.version(onError, onReply);

    {
        // Send the getHeightByTx command:
        auto onError = [](std::error_code ec)
        {
            std::cout << "getHeightByTx: ERROR!" << std::endl;
            ++done;
        };
        auto onReply = [](const size_t &height, const size_t &index)
        {
            std::cout << "getHeightByTx: " << height << std::endl;
            ++done;
            return Status();
        };
        bc::hash_digest hash;
        bc::decode_hash(hash,
                        "698945ec1d4f69f26223d53b3e609329d90783d3d672807af8387c2934e2510a");
        c.getHeightByTx(onError, onReply, hash);
    }

    // Main loop:
    while (true)
    {
        SleepTime sleep;
        ABC_CHECK(c.wakeup(sleep));
        if (2 <= done)
            break;

        zmq_pollitem_t pollitem =
        {
            nullptr, c.pollfd(), ZMQ_POLLIN, ZMQ_POLLOUT
        };
        long timeout = sleep.count() ? sleep.count() : -1;
        zmq_poll(&pollitem, 1, timeout);
    }

    return Status();
}
