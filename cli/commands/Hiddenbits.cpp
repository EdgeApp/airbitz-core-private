/*
 * Copyright (c) 2016, Airbitz, Inc.
 * All rights reserved.
 *
 * See the LICENSE file for more information.
 */

#include "../Command.hpp"
#include "../../abcd/Context.hpp"
#include "../../abcd/util/Util.hpp"
#include "../../abcd/bitcoin/Text.hpp"
#include "../../abcd/bitcoin/Testnet.hpp"
#include "../../abcd/crypto/Random.hpp"
#include "../../abcd/wallet/Wallet.hpp"
#include <bitcoin/bitcoin.hpp>
#include <iostream>

using namespace abcd;

COMMAND(InitLevel::lobby, CliGenerateHiddenbits, "hiddenbits-generate",
        " <count>")
{
    if (argc != 1)
        return ABC_ERROR(ABC_CC_Error, helpString(*this));
    const auto count = atol(argv[0]);

    for(int a = 0; a < count; a++)
    {
        while(1)
        {
            libbitcoin::data_chunk cand(21);
            randomData(cand, 21);
            std::string minikey = libbitcoin::encode_base58(cand);
            minikey.insert(0,"a");
            if (minikey.size() == 30
                    && 0x00 == bc::sha256_hash(bc::to_data_chunk(minikey + "!"))[0])
            {
                bc::ec_secret result;
                bc::data_chunk ec_addr;
                bc::payment_address address;
                char *szAddress = NULL;
                minikey.insert(0,"hbits://");
                hbitsDecode(result, minikey);

                // XOR with our magic number:
                auto mix = bc::decode_hex(gContext->hiddenBitzKey());
                for (size_t i = 0; i < mix.size() && i < result.size(); ++i)
                    result[i] ^= mix[i];

                // Get address:
                ec_addr = bc::secret_to_public_key(result, true);
                address.set(pubkeyVersion(), bc::bitcoin_short_hash(ec_addr));
                szAddress = stringCopy(address.encoded());
                std::cout << minikey << " " << szAddress << std::endl;
                break;
            }
        }
    }

    return Status();
}
