//
// Created by ROSHii on 2019-06-01.
//

#include "base58.h"
#include "data/bip39_vectors.json.h"
#include "key.h"
#include "util.h"
#include "utilstrencodings.h"
#include "test/test_unemploycoin.h"
#include "wallet/bip39.h"

#include <boost/test/unit_test.hpp>

#include <univalue.h>

// In script_tests.cpp
extern UniValue read_json(const std::string& jsondata);

BOOST_FIXTURE_TEST_SUITE(bip39_tests, BasicTestingSetup)

// https://github.com/trezor/python-mnemonic/blob/b502451a33a440783926e04428115e0bed87d01f/vectors.json
BOOST_AUTO_TEST_CASE(bip39_vectors)
{
    UniValue tests = read_json(std::string(json_tests::bip39_vectors, json_tests::bip39_vectors + sizeof(json_tests::bip39_vectors)));

    for (unsigned int i = 0; i < tests.size(); i++) {
        // printf("%d\n", i);
        UniValue test = tests[i];
        std::string strTest = test.write();
        if (test.size() < 4) // Allow for extra stuff (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }

        std::vector<uint8_t> vData = ParseHex(test[0].get_str());
        SecureVector data(vData.begin(), vData.end());

        SecureString m = CMnemonic::FromData(data, data.size());
        std::string strMnemonic = test[1].get_str();
        SecureString mnemonic(strMnemonic.begin(), strMnemonic.end());

        // printf("%s\n%s\n", m.c_str(), mnemonic.c_str());
        BOOST_CHECK(m == mnemonic);
        BOOST_CHECK(CMnemonic::Check(mnemonic));

        SecureVector seed;
        SecureString passphrase("TREZOR");
        CMnemonic::ToSeed(mnemonic, passphrase, seed);
        // printf("seed: %s\n", HexStr(seed).c_str());
        BOOST_CHECK(HexStr(seed) == test[2].get_str());

        CExtKey key;
        CExtPubKey pubkey;

        key.SetSeed(&seed[0], 64);
        pubkey = key.Neuter();

        CRavenExtKey b58key;
        b58key.SetKey(key);
        // printf("CRavenExtKey: %s\n", b58key.ToString().c_str());
        BOOST_CHECK(b58key.ToString() == test[3].get_str());
    }
}

BOOST_AUTO_TEST_SUITE_END()