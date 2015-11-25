/*
 * Copyright (c) 2015, AirBitz, Inc.
 * All rights reserved.
 *
 * See the LICENSE file for more information.
 */

#include "../Command.hpp"
#include "../../abcd/util/Util.hpp"
#include "../../abcd/wallet/Wallet.hpp"
#include <unistd.h>
#include <signal.h>
#include <iostream>

using namespace abcd;

COMMAND(InitLevel::wallet, CliAddressList, "address-list")
{
    if (argc != 3)
        return ABC_ERROR(ABC_CC_Error, "usage: ... address-list <user> <pass> <wallet>");

    auto list = session.wallet->addresses.list();
    for (const auto &i: list)
    {
        abcd::Address address;
        ABC_CHECK(session.wallet->addresses.get(address, i));
        std::cout << address.address << " #" <<
            address.index << ", " <<
            (address.recyclable ? "recyclable" : "used") << std::endl;
    }

    return Status();
}

COMMAND(InitLevel::wallet, CliAddressGenerate, "address-generate")
{
    if (argc != 4)
        return ABC_ERROR(ABC_CC_Error, "usage: ... address-generate <user> <pass> <wallet-name> <count>");

    for(int c = 0; c < atol(argv[3]); c++)
    {
        tABC_TxDetails txDetails;
        AutoString requestId;
        ABC_CHECK_OLD(ABC_CreateReceiveRequest(session.username,
                                               session.password,
                                               argv[2],
                                               &txDetails,
                                               &requestId.get(),
                                               &error
                                              ));

        ABC_CHECK_OLD(ABC_FinalizeReceiveRequest(session.username,
                                           session.password,
                                           argv[2],
                                           requestId,
                                           &error
                                          ));
        std::cout << requestId << std::endl;
    }
    return Status();
}
