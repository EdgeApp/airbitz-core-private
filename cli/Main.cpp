/*
 * Copyright (c) 2014, AirBitz, Inc.
 * All rights reserved.
 *
 * See the LICENSE file for more information.
 */

#include "Command.hpp"
#include "../abcd/json/JsonObject.hpp"
#include "../src/LoginShim.hpp"
#include <iostream>

using namespace abcd;

#define CA_CERT "./cli/ca-certificates.crt"

struct ConfigJson:
    public JsonObject
{
    ABC_JSON_STRING(apiKey, "apiKey", nullptr)
    ABC_JSON_STRING(chainKey, "chainKey", nullptr)
    ABC_JSON_STRING(hiddenBitzKey, "hiddenBitzKey", nullptr)
};

static std::string
configPath()
{
    // Mac: ~/Library/Application Support/Airbitz/airbitz.conf
    // Unix: ~/.config/airbitz/airbitz.conf
    const char *home = getenv("HOME");
    if (!home || !strlen(home))
        home = "/";

#ifdef MAC_OSX
    return std::string(home) + "/Library/Application Support/Airbitz/airbitz.conf";
#else
    return std::string(home) + "/.config/airbitz/airbitz.conf";
#endif
}

/**
 * The main program body.
 */
static Status run(int argc, char *argv[])
{
    ConfigJson json;
    ABC_CHECK(json.load(configPath()));
    ABC_CHECK(json.apiKeyOk());
    ABC_CHECK(json.chainKeyOk());
    ABC_CHECK(json.hiddenBitzKeyOk());

    // Drop our own name:
    --argc;
    ++argv;

    // Find the command:
    if (argc < 1)
    {
        CommandRegistry::print();
        return Status();
    }
    const auto commandName = argv[0];
    --argc;
    ++argv;

    Command *command = CommandRegistry::find(commandName);
    if (!command)
        return ABC_ERROR(ABC_CC_Error,
                         "unknown command " + std::string(commandName));

    // Populate the session up to the required level:
    Session session;
    if (InitLevel::context <= command->level())
    {
        if (argc < 1)
            return ABC_ERROR(ABC_CC_Error, std::string("No working directory given"));
        auto workingDir = argv[0];
        --argc;
        ++argv;

        unsigned char seed[] = {1, 2, 3};
        ABC_CHECK_OLD(ABC_Initialize(workingDir,
                                     CA_CERT,
                                     json.apiKey(),
                                     json.chainKey(),
                                     json.hiddenBitzKey(),
                                     seed,
                                     sizeof(seed),
                                     &error));
    }
    if (InitLevel::lobby <= command->level())
    {
        if (argc < 1)
            return ABC_ERROR(ABC_CC_Error, std::string("No username given"));
        session.username = argv[0];
        --argc;
        ++argv;

        ABC_CHECK(cacheLobby(session.lobby, session.username.c_str()));
    }
    if (InitLevel::login <= command->level())
    {
        if (argc < 1)
            return ABC_ERROR(ABC_CC_Error, std::string("No password given"));
        session.password = argv[0];
        --argc;
        ++argv;

        ABC_CHECK_OLD(ABC_SignIn(session.username.c_str(),
                                 session.password.c_str(),
                                 &error));
        ABC_CHECK(cacheLogin(session.login, session.username.c_str()));
    }
    if (InitLevel::account <= command->level())
    {
        ABC_CHECK(cacheAccount(session.account, session.username.c_str()));
    }
    if (InitLevel::wallet <= command->level())
    {
        if (argc < 1)
            return ABC_ERROR(ABC_CC_Error, std::string("No wallet name given"));
        session.uuid = argv[0];
        --argc;
        ++argv;

        ABC_CHECK(cacheWallet(session.wallet,
                              session.username.c_str(), session.uuid.c_str()));
    }

    // Invoke the command:
    ABC_CHECK((*command)(session, argc, argv));

    // Clean up:
    ABC_Terminate();
    return Status();
}

int main(int argc, char *argv[])
{
    Status s = run(argc, argv);
    if (!s)
        std::cerr << s << std::endl;
    return s ? 0 : 1;
}
