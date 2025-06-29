// Copyright (c) 2014-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The UnemployCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "validation.h"
#include "net.h"

#include "test/test_unemploycoin.h"

#include <boost/signals2/signal.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(main_tests, TestingSetup)

    static void TestBlockSubsidyHalvings(const Consensus::Params &consensusParams)
    {
        int maxHalvings = 64;
        CAmount nInitialSubsidy = 5000 * COIN;

        CAmount nPreviousSubsidy = nInitialSubsidy * 2; // for height == 0
        BOOST_CHECK_EQUAL(nPreviousSubsidy, nInitialSubsidy * 2);
        for (int nHalvings = 0; nHalvings < maxHalvings; nHalvings++)
        {
            int nHeight = nHalvings * consensusParams.nSubsidyHalvingInterval;
            CAmount nSubsidy = GetBlockSubsidy(nHeight, consensusParams);
            BOOST_CHECK(nSubsidy <= nInitialSubsidy);
            BOOST_CHECK_EQUAL(nSubsidy, nPreviousSubsidy / 2);
            nPreviousSubsidy = nSubsidy;
        }
        BOOST_CHECK_EQUAL(GetBlockSubsidy(maxHalvings * consensusParams.nSubsidyHalvingInterval, consensusParams), 0);
    }

    static void TestBlockSubsidyHalvings(int nSubsidyHalvingInterval)
    {
        Consensus::Params consensusParams;
        consensusParams.nSubsidyHalvingInterval = nSubsidyHalvingInterval;
        TestBlockSubsidyHalvings(consensusParams);
    }

    BOOST_AUTO_TEST_CASE(block_subsidy_test)
    {
        BOOST_TEST_MESSAGE("Running Block Subsidy Test");

        const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
        TestBlockSubsidyHalvings(chainParams->GetConsensus()); // As in main
        TestBlockSubsidyHalvings(240); // As in regtest
        TestBlockSubsidyHalvings(1000); // Just another interval
    }

    BOOST_AUTO_TEST_CASE(subsidy_limit_test)
    {
        BOOST_TEST_MESSAGE("Running Subsidy Limit Test");

        const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
        CAmount nSum = 0;
        for (int nHeight = 0; nHeight < 14000000; nHeight += 1000)
        {
            CAmount nSubsidy = GetBlockSubsidy(nHeight, chainParams->GetConsensus());
            BOOST_CHECK(nSubsidy <= 5000 * COIN);
            nSum += nSubsidy * 1000;
            BOOST_CHECK(MoneyRange(nSum));
        }
        BOOST_CHECK_EQUAL(nSum, (int64_t)2078125000000000000ULL);
    }

    bool ReturnFalse()
    { return false; }

    bool ReturnTrue()
    { return true; }

    BOOST_AUTO_TEST_CASE(combiner_all_test)
    {
        BOOST_TEST_MESSAGE("Running Combiner All Test");

        boost::signals2::signal<bool(), CombinerAll> Test;
        BOOST_CHECK(Test());
        Test.connect(&ReturnFalse);
        BOOST_CHECK(!Test());
        Test.connect(&ReturnTrue);
        BOOST_CHECK(!Test());
        Test.disconnect(&ReturnFalse);
        BOOST_CHECK(Test());
        Test.disconnect(&ReturnTrue);
        BOOST_CHECK(Test());
    }

BOOST_AUTO_TEST_SUITE_END()
