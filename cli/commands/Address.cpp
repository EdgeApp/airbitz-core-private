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
    if (argc != 0)
        return ABC_ERROR(ABC_CC_Error, "usage: abc-cli address-list");

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
    if (argc != 1 || strcmp(argv[0], "help") == 0)
        return ABC_ERROR(ABC_CC_Error, "usage: abc-cli address-generate <count>");

    for(int c = 0; c < atol(argv[0]); c++)
    {
        tABC_TxDetails txDetails;
        AutoString requestId;
        ABC_CHECK_OLD(ABC_CreateReceiveRequest(session.username,
                                               session.password,
                                               session.uuid,
                                               &txDetails,
                                               &requestId.get(),
                                               &error
                                              ));

        ABC_CHECK_OLD(ABC_FinalizeReceiveRequest(session.username,
                                           session.password,
                                           session.uuid,
                                           requestId,
                                           &error
                                          ));
        std::cout << requestId << std::endl;
    }
    return Status();
}
