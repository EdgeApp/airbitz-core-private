/*
 * Copyright (c) 2015, AirBitz, Inc.
 * All rights reserved.
 *
 * See the LICENSE file for more information.
 */

#include "../Command.hpp"
#include "../../abcd/account/Account.hpp"
#include "../../abcd/json/JsonBox.hpp"
#include "../../abcd/login/Login.hpp"
#include "../../abcd/util/FileIO.hpp"
#include "../../abcd/util/Util.hpp"
#include <iostream>
#include <string.h>

using namespace abcd;

COMMAND(InitLevel::context, AccountAvailable, "account-available")
{
    if(argc != 1 || strcmp(argv[0], "help") == 0)
        return ABC_ERROR(ABC_CC_Error, "usage: abc-cli account-available <username>\n");

    ABC_CHECK_OLD(ABC_AccountAvailable(argv[0], &error));
    return Status();
}

COMMAND(InitLevel::account, AccountDecrypt, "account-decrypt")
{
    if (argc != 1 || strcmp(argv[0], "help") == 0)
       return ABC_ERROR(ABC_CC_Error, "usage: abc-cli account-decrypt <filename>\n"
           "note: The filename is account-relative.");

    JsonBox box;
    ABC_CHECK(box.load(session.account->dir() + argv[0]));

    DataChunk data;
    ABC_CHECK(box.decrypt(data, session.login->dataKey()));
    std::cout << toString(data) << std::endl;

    return Status();
}

COMMAND(InitLevel::account, AccountEncrypt, "account-encrypt")
{
    if (argc != 1 || strcmp(argv[0], "help") == 0)
        return ABC_ERROR(ABC_CC_Error, "usage: abc-cli account-encrypt <filename>\n"
            "note: The filename is account-relative.");

    DataChunk contents;
    ABC_CHECK(fileLoad(contents, session.account->dir() + argv[0]));

    JsonBox box;
    ABC_CHECK(box.encrypt(contents, session.login->dataKey()));

    std::cout << box.encode() << std::endl;

    return Status();
}

COMMAND(InitLevel::context, CreateAccount, "account-create")
{
    if (argc != 2 || strcmp(argv[0], "help") == 0)
        return ABC_ERROR(ABC_CC_Error, "usage: abc-cli account-create <user> <pass>");

    ABC_CHECK_OLD(ABC_CreateAccount(argv[0], argv[1], &error));
    ABC_CHECK_OLD(ABC_SetPIN(argv[2], argv[3], "1234", &error));

    return Status();
}
