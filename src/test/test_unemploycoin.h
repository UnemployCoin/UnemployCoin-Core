// Copyright (c) 2015-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The UnemployCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UNEMPLOYCOIN_TEST_TEST_UNEMPLOYCOIN_H
#define UNEMPLOYCOIN_TEST_TEST_UNEMPLOYCOIN_H

#include "chainparamsbase.h"
#include "fs.h"
#include "key.h"
#include "pubkey.h"
#include "random.h"
#include "scheduler.h"
#include "txdb.h"
#include "txmempool.h"

#include <boost/thread.hpp>
#include <test/test_unemploycoin.h>

extern uint256 insecure_rand_seed;
extern FastRandomContext insecure_rand_ctx;

static inline void SeedInsecureRand(bool fDeterministic = false)
{
    if (fDeterministic)
    {
        insecure_rand_seed = uint256();
    } else
    {
        insecure_rand_seed = GetRandHash();
    }
    insecure_rand_ctx = FastRandomContext(insecure_rand_seed);
}

static inline uint32_t InsecureRand32()
{ return insecure_rand_ctx.rand32(); }

static inline uint256 InsecureRand256()
{ return insecure_rand_ctx.rand256(); }

static inline uint64_t InsecureRandBits(int bits)
{ return insecure_rand_ctx.randbits(bits); }

static inline uint64_t InsecureRandRange(uint64_t range)
{ return insecure_rand_ctx.randrange(range); }

static inline bool InsecureRandBool()
{ return insecure_rand_ctx.randbool(); }

/** Basic testing setup.
 * This just configures logging and chain parameters.
 */
struct BasicTestingSetup
{
    ECCVerifyHandle globalVerifyHandle;

    explicit BasicTestingSetup(const std::string &chainName = CBaseChainParams::MAIN);

    ~BasicTestingSetup();
};

/** Testing setup that configures a complete environment.
 * Included are data directory, coins database, script check threads setup.
 */
class CConnman;

class PeerLogicValidation;

struct TestingSetup : public BasicTestingSetup
{
    CCoinsViewDB *pcoinsdbview;
    fs::path pathTemp;
    boost::thread_group threadGroup;
    CConnman *connman;
    CScheduler scheduler;
    std::unique_ptr<PeerLogicValidation> peerLogic;

    explicit TestingSetup(const std::string &chainName = CBaseChainParams::MAIN);

    ~TestingSetup();
};

class CBlock;

struct CMutableTransaction;

class CScript;

//
// Testing fixture that pre-creates a
// 100-block REGTEST-mode block chain
//
struct TestChain100Setup : public TestingSetup
{
    TestChain100Setup();

    // Create a new block with just given transactions, coinbase paying to
    // scriptPubKey, and try to add it to the current chain.
    CBlock CreateAndProcessBlock(const std::vector<CMutableTransaction> &txns,
                                 const CScript &scriptPubKey);

    ~TestChain100Setup();

    std::vector<CTransaction> coinbaseTxns; // For convenience, coinbase transactions
    CKey coinbaseKey; // private/public key needed to spend coinbase transactions
};

class CTxMemPoolEntry;

struct TestMemPoolEntryHelper
{
    // Default values
    CAmount nFee;
    int64_t nTime;
    unsigned int nHeight;
    bool spendsCoinbase;
    unsigned int sigOpCost;
    LockPoints lp;

    TestMemPoolEntryHelper() :
            nFee(0), nTime(0), nHeight(1),
            spendsCoinbase(false), sigOpCost(4)
    {}

    CTxMemPoolEntry FromTx(const CMutableTransaction &tx);

    CTxMemPoolEntry FromTx(const CTransaction &tx);

    // Change the default value
    TestMemPoolEntryHelper &Fee(CAmount _fee)
    {
        nFee = _fee;
        return *this;
    }

    TestMemPoolEntryHelper &Time(int64_t _time)
    {
        nTime = _time;
        return *this;
    }

    TestMemPoolEntryHelper &Height(unsigned int _height)
    {
        nHeight = _height;
        return *this;
    }

    TestMemPoolEntryHelper &SpendsCoinbase(bool _flag)
    {
        spendsCoinbase = _flag;
        return *this;
    }

    TestMemPoolEntryHelper &SigOpsCost(unsigned int _sigopsCost)
    {
        sigOpCost = _sigopsCost;
        return *this;
    }
};

CBlock getBlock13b8a();

#endif
