/*
 * Copyright (c) 2015, AirBitz, Inc.
 * All rights reserved.
 *
 * See the LICENSE file for more information.
 */

#include "../Command.hpp"
#include "../Util.hpp"
#include "../../abcd/bitcoin/TxUpdater.hpp"
#include "../../abcd/wallet/Wallet.hpp"
#include <fstream>
#include <unistd.h>
#include <signal.h>
#include <iostream>

using namespace abcd;

static bool running = true;

static void
syncCallback(const tABC_AsyncBitCoinInfo *pInfo)
{
}

static void
signalCallback(int dummy)
{
    running = false;
}

COMMAND(InitLevel::wallet, Watcher, "watcher",
        "")
{
    if (argc != 0)
        return ABC_ERROR(ABC_CC_Error, helpString(*this));

    ABC_CHECK_OLD(ABC_DataSyncWallet(session.username.c_str(),
                                     session.password.c_str(),
                                     session.uuid.c_str(),
                                     syncCallback, nullptr, &error));
    {
        WatcherThread thread;
        ABC_CHECK(thread.init(session));

        // The command stops with ctrl-c:
        signal(SIGINT, signalCallback);
        while (running)
            sleep(1);
    }
    ABC_CHECK_OLD(ABC_DataSyncWallet(session.username.c_str(),
                                     session.password.c_str(),
                                     session.uuid.c_str(),
                                     syncCallback, nullptr, &error));

    return Status();
}


COMMAND(InitLevel::wallet, WatcherDump, "watcher-dump",
        " <filename>")
{
    if(argc > 1)
        return ABC_ERROR(ABC_CC_Error, helpString(*this));

    if(argc == 1)
    {
        const std::string filename = argv[0];
        std::ofstream file(filename);
        if (!file.is_open())
        {
            return ABC_ERROR(ABC_CC_Error, "Cannot open file");
        }
        session.wallet->txdb.dump(file);
    }
    else
        session.wallet->txdb.dump(std::cout);

    return Status();
}

COMMAND(InitLevel::wallet, WatcherDumpFile, "watcher-dump-file",
        " <filename>")
{
    if(argc != 1)
        return ABC_ERROR(ABC_CC_Error, helpString(*this));

    const std::string filename = argv[0];
    std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        return ABC_ERROR(ABC_CC_Error, "Cannot open file");
    }

    std::streampos size = file.tellg();
    uint8_t *data = new uint8_t[size];
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char *>(data), size);
    file.close();

    if (!session.wallet->txdb.load(bc::data_chunk(data, data + size)))
        return ABC_ERROR(ABC_CC_Error, "Error while loading data.");

    session.wallet->txdb.dump(std::cout);
    return Status();
}
