// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2021 The UnemployCoin Core developers
// Copyright (c) 2024-2025 The UnemployCoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"
#include "arith_uint256.h"

#include <assert.h>
#include "chainparamsseeds.h"




static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << CScriptNum(0) << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "CNN 20/Jun/2025 Indiana Pacers crush Oklahoma City Thunder to force Game 7 in the NBA Finals";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

void CChainParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

void CChainParams::TurnOffSegwit() {
	consensus.nSegwitEnabled = false;
}

void CChainParams::TurnOffCSV() {
	consensus.nCSVEnabled = false;
}

void CChainParams::TurnOffBIP34() {
	consensus.nBIP34Enabled = false;
}

void CChainParams::TurnOffBIP65() {
	consensus.nBIP65Enabled = false;
}

void CChainParams::TurnOffBIP66() {
	consensus.nBIP66Enabled = false;
}

bool CChainParams::BIP34() {
	return consensus.nBIP34Enabled;
}

bool CChainParams::BIP65() {
	return consensus.nBIP34Enabled;
}

bool CChainParams::BIP66() {
	return consensus.nBIP34Enabled;
}

bool CChainParams::CSVEnabled() const{
	return consensus.nCSVEnabled;
}


/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 2100000;
        consensus.nBIP34Enabled = true;
        consensus.nBIP65Enabled = true;
        consensus.nBIP66Enabled = true;
        consensus.nSegwitEnabled = false;
        consensus.nCSVEnabled = false;

        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.kawpowLimit = uint256S("0000000000ffffffffffffffffffffffffffffffffffffffffffffffffffffff");

        consensus.nPowTargetTimespan = 2016 * 60;
        consensus.nPowTargetSpacing = 1 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1613;
        consensus.nMinerConfirmationWindow = 2016;

        consensus.nMinimumChainWork = uint256S("0x00");
        consensus.defaultAssumeValid = uint256S("0x00");

        // Message start string 'U', 'N', 'E', 'M'
        pchMessageStart[0] = 0x55;
        pchMessageStart[1] = 0x4e;
        pchMessageStart[2] = 0x45;
        pchMessageStart[3] = 0x4d;
        nDefaultPort = 19771;
        nPruneAfterHeight = 100000;

        // Genesis Block
        genesis = CreateGenesisBlock(
            1718830000,     // Timestamp (20 June 2024)
            2459986,        // Nonce (from generated block)
            0x1e00ffff,     // Bits
            4,              // Version
            10000 * COIN    // Reward
        );

        consensus.hashGenesisBlock = genesis.GetX16RHash();
        assert(consensus.hashGenesisBlock == uint256S("000007cf5c3dbe4e0b59ecbb9e7d50ad48d90557e34a06b3b28522ffb1cb8799"));
        assert(genesis.hashMerkleRoot == uint256S("1c07f11d5c1b034e61b1bc79fdb4cd9607d2a3a2fc3e5b86f011c942308c74e4"));

        // DNS Seeds
        vSeeds.emplace_back("seed1.unemploycoin.com", false);
        vSeeds.emplace_back("seed2.unemploycoin.org", false);

        // Base58 Prefixes
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 68);    // 'U'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 125);   // custom
        base58Prefixes[SECRET_KEY]     = std::vector<unsigned char>(1, 196);   // '5'-range
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        nExtCoinType = 2025;

        // Seeds
        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fMiningRequiresPeers = true;

        // Checkpoints
        checkpointData = (CCheckpointData) {
            {
                {0, uint256S("000007cf5c3dbe4e0b59ecbb9e7d50ad48d90557e34a06b3b28522ffb1cb8799")}
            }
        };

        // Chain stats
        chainTxData = ChainTxData{
            1718830000, // timestamp
            1,          // 1 transaction (premine)
            0.001       // tps
        };

        /** Optional burn system (rebranded for now) **/
        nIssueAssetBurnAmount = 500 * COIN;
        nReissueAssetBurnAmount = 100 * COIN;
        nIssueSubAssetBurnAmount = 100 * COIN;
        nIssueUniqueAssetBurnAmount = 5 * COIN;
        nIssueMsgChannelAssetBurnAmount = 100 * COIN;
        nIssueQualifierAssetBurnAmount = 1000 * COIN;
        nIssueSubQualifierAssetBurnAmount = 100 * COIN;
        nIssueRestrictedAssetBurnAmount = 1500 * COIN;
        nAddNullQualifierTagBurnAmount = 0.1 * COIN;

        strIssueAssetBurnAddress = "UNissueAssetXXXXXXXXXXXXXXXXXXxyZ";
        strReissueAssetBurnAddress = "UNReissueAssetXXXXXXXXXXXXXXXabcD";
        strIssueSubAssetBurnAddress = "UNissueSubAssetXXXXXXXXXXXXXYqweT";
        strIssueUniqueAssetBurnAddress = "UNissueUniqueAssetXXXXXXXXXXxpLmN";
        strIssueMsgChannelAssetBurnAddress = "UNMsgChannelAssetXXXXXXXXXXXhTrE";
        strIssueQualifierAssetBurnAddress = "UNissueQualifierXXXXXXXXXXXXTgbV";
        strIssueSubQualifierAssetBurnAddress = "UNSubQualifierXXXXXXXXXXXXXXasSd";
        strIssueRestrictedAssetBurnAddress = "UNRestrictedAssetXXXXXXXXXXXErTz";
        strAddNullQualifierTagBurnAddress = "UNTagBurnXXXXXXXXXXXXXXXXXXXvBnM";
        strGlobalBurnAddress = "UNBurnXXXXXXXXXXXXXXXXXXXXXXXXZxCv";

        // Activation settings
        nDGWActivationBlock = 10;
        nAssetActivationHeight = 100;
        nMessagingActivationBlock = 200;
        nRestrictedActivationBlock = 300;
        nKAWPOWActivationTime = 1718831000;
    }
};


/**
 * Testnet (v7)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 2100000;
        consensus.nBIP34Enabled = true;
        consensus.nBIP65Enabled = true;
        consensus.nBIP66Enabled = true;
        consensus.nSegwitEnabled = false;
        consensus.nCSVEnabled = false;

        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.kawpowLimit = uint256S("000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 2016 * 60;
        consensus.nPowTargetSpacing = 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1310;
        consensus.nMinerConfirmationWindow = 2016;

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY] = {28, 1199145601, 1230767999, 1310, 2016};
        consensus.vDeployments[Consensus::DEPLOYMENT_ASSETS] = {5, 1533924000, 1577257200, 1310, 2016};
        consensus.vDeployments[Consensus::DEPLOYMENT_MSG_REST_ASSETS] = {6, 1570428000, 1577257200, 1310, 2016};
        consensus.vDeployments[Consensus::DEPLOYMENT_TRANSFER_SCRIPT_SIZE] = {8, 1586973600, 1618509600, 1310, 2016};
        consensus.vDeployments[Consensus::DEPLOYMENT_ENFORCE_VALUE] = {9, 1593453600, 1624989600, 1411, 2016};
        consensus.vDeployments[Consensus::DEPLOYMENT_COINBASE_ASSETS] = {10, 1597341600, 1628877600, 1411, 2016};

        consensus.nMinimumChainWork = uint256S("0x00");
        consensus.defaultAssumeValid = uint256S("0x00");

        // Message start string: U N E T
        pchMessageStart[0] = 0x55;
        pchMessageStart[1] = 0x4e;
        pchMessageStart[2] = 0x45;
        pchMessageStart[3] = 0x54;
        nDefaultPort = 19772;
        nPruneAfterHeight = 1000;

        uint32_t nGenesisTime = 1718830100;
        genesis = CreateGenesisBlock(nGenesisTime, 1246790, 0x1e00ffff, 2, 5000 * COIN);
        consensus.hashGenesisBlock = genesis.GetX16RHash();

        assert(consensus.hashGenesisBlock == uint256S("0000048ad1f9d02f6443c8142256213f38045c51c5f72a11b4ecfa73f9e0914f"));
        assert(genesis.hashMerkleRoot == uint256S("1c07f11d5c1b034e61b1bc79fdb4cd9607d2a3a2fc3e5b86f011c942308c74e4"));

        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.emplace_back("testnet-seed1.unemploycoin.com", false);
        vSeeds.emplace_back("testnet-seed2.unemploycoin.org", false);

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 111);  // 'm' or 'n'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
        base58Prefixes[SECRET_KEY]     = std::vector<unsigned char>(1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};
        nExtCoinType = 1;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fMiningRequiresPeers = true;

        checkpointData = {
            {
                {0, uint256S("0000048ad1f9d02f6443c8142256213f38045c51c5f72a11b4ecfa73f9e0914f")}
            }
        };

        chainTxData = {
            nGenesisTime,
            1,
            0.01
        };

        /** Optional burn settings **/
        nIssueAssetBurnAmount = 500 * COIN;
        nReissueAssetBurnAmount = 100 * COIN;
        nIssueSubAssetBurnAmount = 100 * COIN;
        nIssueUniqueAssetBurnAmount = 5 * COIN;
        nIssueMsgChannelAssetBurnAmount = 100 * COIN;
        nIssueQualifierAssetBurnAmount = 1000 * COIN;
        nIssueSubQualifierAssetBurnAmount = 100 * COIN;
        nIssueRestrictedAssetBurnAmount = 1500 * COIN;
        nAddNullQualifierTagBurnAmount = 0.1 * COIN;

        strIssueAssetBurnAddress = "n1UNEMPTestIssueXXXXXXXXXXXXXXXXXX";
        strReissueAssetBurnAddress = "n1UNEMPTestReissueXXXXXXXXXXXXXXX";
        strIssueSubAssetBurnAddress = "n1UNEMPTestSubAssetXXXXXXXXXXXXXX";
        strIssueUniqueAssetBurnAddress = "n1UNEMPTestUniqueAssetXXXXXXXXXXX";
        strIssueMsgChannelAssetBurnAddress = "n1UNEMPTestMsgAssetXXXXXXXXXXXXXX";
        strIssueQualifierAssetBurnAddress = "n1UNEMPTestQualifierXXXXXXXXXXXXXX";
        strIssueSubQualifierAssetBurnAddress = "n1UNEMPTestSubQualifierXXXXXXXXXX";
        strIssueRestrictedAssetBurnAddress = "n1UNEMPTestRestrictedXXXXXXXXXXXXX";
        strAddNullQualifierTagBurnAddress = "n1UNEMPTestTagBurnXXXXXXXXXXXXXXXX";
        strGlobalBurnAddress = "n1UNEMPTestGlobalBurnXXXXXXXXXXXXXX";

        nDGWActivationBlock = 1;
        nAssetActivationHeight = 50;
        nMessagingActivationBlock = 100;
        nRestrictedActivationBlock = 150;
        nKAWPOWActivationTime = nGenesisTime + 60;
    }
};


/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nBIP34Enabled = true;
        consensus.nBIP65Enabled = true;
        consensus.nBIP66Enabled = true;
        consensus.nSegwitEnabled = false;
        consensus.nCSVEnabled = false;

        consensus.nSubsidyHalvingInterval = 150;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.kawpowLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 2016 * 60;
        consensus.nPowTargetSpacing = 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108;
        consensus.nMinerConfirmationWindow = 144;

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY] = {28, 0, 999999999999ULL, 108, 144};
        consensus.vDeployments[Consensus::DEPLOYMENT_ASSETS] = {6, 0, 999999999999ULL, 108, 144};
        consensus.vDeployments[Consensus::DEPLOYMENT_MSG_REST_ASSETS] = {7, 0, 999999999999ULL, 108, 144};
        consensus.vDeployments[Consensus::DEPLOYMENT_TRANSFER_SCRIPT_SIZE] = {8, 0, 999999999999ULL, 208, 288};
        consensus.vDeployments[Consensus::DEPLOYMENT_ENFORCE_VALUE] = {9, 0, 999999999999ULL, 108, 144};
        consensus.vDeployments[Consensus::DEPLOYMENT_COINBASE_ASSETS] = {10, 0, 999999999999ULL, 400, 500};

        consensus.nMinimumChainWork = uint256S("0x00");
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0x55; // U
        pchMessageStart[1] = 0x4e; // N
        pchMessageStart[2] = 0x45; // E
        pchMessageStart[3] = 0x52; // R
        nDefaultPort = 19773;
        nPruneAfterHeight = 1000;

        // Regtest Genesis
        genesis = CreateGenesisBlock(1718830200, 1, 0x207fffff, 4, 5000 * COIN);
        consensus.hashGenesisBlock = genesis.GetX16RHash();

        assert(consensus.hashGenesisBlock == uint256S("0x42174d1fa97a8a02c2955b6a28742fa13c3e5f5887a3ef73b8f1c3fd162c7f2e"));
        assert(genesis.hashMerkleRoot == uint256S("1c07f11d5c1b034e61b1bc79fdb4cd9607d2a3a2fc3e5b86f011c942308c74e4"));

        vFixedSeeds.clear(); // No DNS seeds
        vSeeds.clear();

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = {
            {
                {0, uint256S("0x42174d1fa97a8a02c2955b6a28742fa13c3e5f5887a3ef73b8f1c3fd162c7f2e")}
            }
        };

        chainTxData = {
            1718830200,
            1,
            0.001
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
        base58Prefixes[SECRET_KEY]     = std::vector<unsigned char>(1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        nExtCoinType = 1;

        // Burn Logic (Regtest)
        nIssueAssetBurnAmount = 500 * COIN;
        nReissueAssetBurnAmount = 100 * COIN;
        nIssueSubAssetBurnAmount = 100 * COIN;
        nIssueUniqueAssetBurnAmount = 5 * COIN;
        nIssueMsgChannelAssetBurnAmount = 100 * COIN;
        nIssueQualifierAssetBurnAmount = 1000 * COIN;
        nIssueSubQualifierAssetBurnAmount = 100 * COIN;
        nIssueRestrictedAssetBurnAmount = 1500 * COIN;
        nAddNullQualifierTagBurnAmount = 0.1 * COIN;

        strIssueAssetBurnAddress = "n1UNEMPRegIssueXXXXXXXXXXXXXXXXXX";
        strReissueAssetBurnAddress = "n1UNEMPRegReissueXXXXXXXXXXXXXXX";
        strIssueSubAssetBurnAddress = "n1UNEMPRegSubAssetXXXXXXXXXXXXXX";
        strIssueUniqueAssetBurnAddress = "n1UNEMPRegUniqueAssetXXXXXXXXXXX";
        strIssueMsgChannelAssetBurnAddress = "n1UNEMPRegMsgAssetXXXXXXXXXXXXXX";
        strIssueQualifierAssetBurnAddress = "n1UNEMPRegQualifierXXXXXXXXXXXXXX";
        strIssueSubQualifierAssetBurnAddress = "n1UNEMPRegSubQualifierXXXXXXXXXX";
        strIssueRestrictedAssetBurnAddress = "n1UNEMPRegRestrictedXXXXXXXXXXXXX";
        strAddNullQualifierTagBurnAddress = "n1UNEMPRegTagBurnXXXXXXXXXXXXXXXX";
        strGlobalBurnAddress = "n1UNEMPRegGlobalBurnXXXXXXXXXXXXXX";

        nDGWActivationBlock = 0;
        nAssetActivationHeight = 0;
        nMessagingActivationBlock = 0;
        nRestrictedActivationBlock = 0;
        nKAWPOWActivationTime = 3582830167;
    }
};


static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &GetParams() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network, bool fForceBlockNetwork)
{
    SelectBaseParams(network);
    if (fForceBlockNetwork) {
        bNetwork.SetNetwork(network);
    }
    globalChainParams = CreateChainParams(network);
}

void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    globalChainParams->UpdateVersionBitsParameters(d, nStartTime, nTimeout);
}

void TurnOffSegwit(){
	globalChainParams->TurnOffSegwit();
}

void TurnOffCSV() {
	globalChainParams->TurnOffCSV();
}

void TurnOffBIP34() {
	globalChainParams->TurnOffBIP34();
}

void TurnOffBIP65() {
	globalChainParams->TurnOffBIP65();
}

void TurnOffBIP66() {
	globalChainParams->TurnOffBIP66();
}
