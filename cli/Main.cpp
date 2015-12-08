/*
 * Copyright (c) 2014, AirBitz, Inc.
 * All rights reserved.
 *
 * See the LICENSE file for more information.
 */

#include "Command.hpp"
#include "../abcd/json/JsonObject.hpp"
#include "../abcd/util/Util.hpp"
#include "../src/LoginShim.hpp"
#include <iostream>
#include <getopt.h>

using namespace abcd;

#define CA_CERT "./cli/ca-certificates.crt"

struct ConfigJson:
    public JsonObject
{
    ABC_JSON_STRING(apiKey, "apiKey", nullptr)
    ABC_JSON_STRING(chainKey, "chainKey", nullptr)
    ABC_JSON_STRING(hiddenBitzKey, "hiddenBitzKey", nullptr)
    ABC_JSON_STRING(workingDir, "workingDir", nullptr)
    ABC_JSON_STRING(username, "username", nullptr)
    ABC_JSON_STRING(password, "password", nullptr)
    ABC_JSON_STRING(wallet, "wallet", nullptr)
};

static std::string GetConfigFilePath()
{
    // Mac: ~/Library/Application Support/Airbitz/airbitz.conf
    // Unix: ~/.config/airbitz/airbitz.conf
    std::string pathRet;
    char *pszHome = getenv("HOME");
    if (pszHome == NULL || strlen(pszHome) == 0)
        pathRet = "/";
    else
        pathRet = pszHome;

#ifdef MAC_OSX
    pathRet.append("/Library/Application Support/Airbitz/airbitz.conf");
#else
    pathRet.append("/.config/airbitz/airbitz.conf");
#endif
    return pathRet;
}

/**
 * The main program body.
 */
static Status run(int argc, char *argv[])
{
    ConfigJson json;
    ABC_CHECK(json.load(GetConfigFilePath()));
    ABC_CHECK(json.apiKeyOk());
    ABC_CHECK(json.chainKeyOk());
    ABC_CHECK(json.hiddenBitzKeyOk());

    if (argc < 2 || strcmp(argv[1], "help") == 0
            || strcmp(argv[1], "--help") == 0)
    {
        CommandRegistry::print();
        return Status();
    }

    // Find the command:
    Command *command = CommandRegistry::find(argv[1]);
    if (!command)
        return ABC_ERROR(ABC_CC_Error, "Unknown command " + std::string(argv[1])
                         + "\n" + "Try 'abc-cli --help' for more information.");

    // Populate the session up to the required level:
    Session session;
    char *workingDir = NULL;
    session.password = "";
    session.username = "";
    session.uuid   = "";

    int c;
    opterr = 0;

    static struct option long_options[] =
    {
        {"working-dir", required_argument, 0, 'd'},
        {"username",    required_argument, 0, 'u'},
        {"password",    required_argument, 0, 'p'},
        {0, 0, 0, 0}
    };
    /* getopt_long stores the option index here. */
    int option_index = 0;

    while ((c = getopt_long (argc, argv, "d:u:p:", long_options,
                             &option_index)) != -1)
    {
        switch (c)
        {
        case 'd':
            workingDir = optarg;
            break;
        case 'p':
            session.password = optarg;
            break;
        case 'u':
            session.username = optarg;
            break;
        case '?':
            if (optopt == 'd')
                return ABC_ERROR(ABC_CC_Error, std::string("-d requires a working directory"));
            else if (optopt == 'p')
                return ABC_ERROR(ABC_CC_Error, std::string("-p requires a password"));
            else if (optopt == 'u')
                return ABC_ERROR(ABC_CC_Error, std::string("-u requires a username"));
            else if (isprint (optopt))
                return ABC_ERROR(ABC_CC_Error, std::string("Unknown option `-%c'.\n", optopt));
            else
                return ABC_ERROR(ABC_CC_Error,
                                 std::string("Unknown option character `\\x%x'.\n",
                                             optopt));
        default:
            abort ();
        }
    }

    if(workingDir == NULL)
    {
        if(json.workingDirOk())
            workingDir = (char *)json.workingDir();
        else
            return ABC_ERROR(ABC_CC_Error, std::string("No working directory given"));
    }

    if (InitLevel::context <= command->level())
    {
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
        if(strcmp(session.username, "") == 0)
        {
            if(json.usernameOk())
                session.username = (char *)json.username();
            else
                return ABC_ERROR(ABC_CC_Error, std::string("No username given"));
        }
        ABC_CHECK(cacheLobby(session.lobby, session.username));
    }
    if (InitLevel::login <= command->level())
    {
        if(strcmp(session.password, "") == 0)
        {
            if(json.passwordOk())
                session.password = (char *)json.password();
            else
                return ABC_ERROR(ABC_CC_Error, std::string("No password given"));
        }
        tABC_Error error;
        tABC_CC cc =  ABC_SignIn(session.username, session.password, &error);
        if (ABC_CC_InvalidOTP == cc)
        {
            AutoString date;
            ABC_CHECK_OLD(ABC_OtpResetDate(&date.get(), &error));
            if (strlen(date))
                std::cout << "Pending OTP reset ends at " << date.get() << std::endl;
            std::cout << "No OTP token, resetting account 2-factor auth." << std::endl;
            ABC_CHECK_OLD(ABC_OtpResetSet(session.username, &error));
        }
        ABC_CHECK(cacheLogin(session.login, session.username));
    }
    if (InitLevel::account <= command->level())
    {
        ABC_CHECK(cacheAccount(session.account, session.username));
    }
    if (InitLevel::wallet <= command->level())
    {
        if(strcmp(session.uuid, "") == 0)
        {
            if(json.passwordOk())
                session.uuid = (char *)json.wallet();
            else
                return ABC_ERROR(ABC_CC_Error, std::string("No wallet name given"));
        }

        session.uuid = argv[5];
        ABC_CHECK(cacheWallet(session.wallet, session.username, session.uuid));
    }

    // Invoke the command:
    ABC_CHECK((*command)(session, argc-optind-1, argv+optind+1));

    // Clean up:
    ABC_Terminate();
    return Status();
}

int main(int argc, char *argv[])
{
    Status s = run(argc, argv);
    if (!s)
        std::cerr << s.message() << std::endl;
    return s ? 0 : 1;
}
