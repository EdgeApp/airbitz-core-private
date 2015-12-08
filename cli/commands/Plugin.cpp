/*
 * Copyright (c) 2014, AirBitz, Inc.
 * All rights reserved.
 *
 * See the LICENSE file for more information.
 */

#include "../Command.hpp"
#include "../../abcd/account/PluginData.hpp"
#include <iostream>
#include <string.h>

using namespace abcd;

COMMAND(InitLevel::account, PluginGet, "plugin-get")
{
    if (argc != 2 || strcmp(argv[0], "help") == 0)
        return ABC_ERROR(ABC_CC_Error, "usage: abc-cli plugin-get <plugin> <key>");

    std::string value;
    ABC_CHECK(pluginDataGet(*session.account, argv[0], argv[1], value));
    std::cout << '"' << value << '"' << std::endl;
    return Status();
}

COMMAND(InitLevel::account, PluginSet, "plugin-set")
{
    if (argc != 3 || strcmp(argv[0], "help") == 0)
        return ABC_ERROR(ABC_CC_Error,
                         "usage: abc-cli plugin-set <plugin> <key> <value>");
    ABC_CHECK(pluginDataSet(*session.account, argv[0], argv[1], argv[2]));

    return Status();
}

COMMAND(InitLevel::account, PluginRemove, "plugin-remove")
{
    if (argc != 2 || strcmp(argv[0], "help") == 0)
        return ABC_ERROR(ABC_CC_Error, "usage: abc-cli plugin-remove <plugin> <key>");
    ABC_CHECK(pluginDataRemove(*session.account, argv[0], argv[1]));
    return Status();
}

COMMAND(InitLevel::account, PluginClear, "plugin-clear")
{
    if (argc != 1 || strcmp(argv[0], "help") == 0)
        return ABC_ERROR(ABC_CC_Error, "usage: abc-cli plugin-clear <plugin>");
    ABC_CHECK(pluginDataClear(*session.account, argv[0]));
    return Status();
}
