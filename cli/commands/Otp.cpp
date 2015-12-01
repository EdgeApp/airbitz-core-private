/*
 * Copyright (c) 2015, AirBitz, Inc.
 * All rights reserved.
 *
 * See the LICENSE file for more information.
 */

#include "../Command.hpp"
#include "../../abcd/login/Lobby.hpp"
#include "../../abcd/login/LoginDir.hpp"
#include "../../abcd/login/Otp.hpp"
#include <iostream>

using namespace abcd;

COMMAND(InitLevel::lobby, OtpKeyGet, "otp-key-get")
{
    if (argc != 0)
        return ABC_ERROR(ABC_CC_Error, "usage: abc-cli otp-key-get");

    const OtpKey *key = session.lobby->otpKey();
    if (key)
        std::cout << "key: " << key->encodeBase32() << std::endl;
    else
        std::cout << "no key" << std::endl;

    return Status();
}

COMMAND(InitLevel::lobby, OtpKeySet, "otp-key-set")
{
    if (argc != 1 || strcmp(argv[0], "help") == 0)
        return ABC_ERROR(ABC_CC_Error, "usage: abc-cli otp-key-set <key>");

    OtpKey key;
    ABC_CHECK(key.decodeBase32(argv[0]));
    ABC_CHECK(session.lobby->otpKeySet(key));

    return Status();
}

COMMAND(InitLevel::lobby, OtpKeyRemove, "otp-key-remove")
{
    if (argc != 0)
        return ABC_ERROR(ABC_CC_Error, "usage: abc-cli otp-key-remove");

    ABC_CHECK(session.lobby->otpKeyRemove());

    return Status();
}

COMMAND(InitLevel::login, OtpAuthGet, "otp-auth-get")
{
    if (argc != 0)
        return ABC_ERROR(ABC_CC_Error, "usage: abc-cli otp-auth-get");

    bool enabled;
    long timeout;
    ABC_CHECK(otpAuthGet(*session.login, enabled, timeout));
    if (enabled)
        std::cout << "OTP on, timeout: " << timeout << std::endl;
    else
        std::cout << "OTP off." << std::endl;

    return Status();
}

COMMAND(InitLevel::login, OtpAuthSet, "otp-auth-set")
{
    if (argc != 1 || strcmp(argv[0], "help") == 0)
        return ABC_ERROR(ABC_CC_Error, "usage: abc-cli otp-auth-set <timeout-sec>");

    ABC_CHECK(otpAuthSet(*session.login, atol(argv[0])));

    return Status();
}

COMMAND(InitLevel::login, OtpAuthRemove, "otp-auth-remove")
{
    if (argc != 0)
        return ABC_ERROR(ABC_CC_Error, "usage: abc-cli otp-auth-remove");

    ABC_CHECK(otpAuthRemove(*session.login));
    ABC_CHECK(session.lobby->otpKeyRemove());

    return Status();
}

COMMAND(InitLevel::context, OtpResetGet, "otp-reset-get")
{
    if (argc != 0)
        return ABC_ERROR(ABC_CC_Error, "usage: abc-cli otp-reset-get");

    std::list<std::string> result;
    ABC_CHECK(otpResetGet(result, loginDirList()));
    for (auto &i: result)
        std::cout << i << std::endl;

    return Status();
}

COMMAND(InitLevel::login, OtpResetRemove, "otp-reset-remove")
{
    if (argc != 0)
        return ABC_ERROR(ABC_CC_Error, "usage: abc-cli otp-reset-remove");

    ABC_CHECK(otpResetRemove(*session.login));

    return Status();
}
