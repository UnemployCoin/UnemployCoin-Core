// Copyright (c) 2017-2017 The Bitcoin Core developers
// Copyright (c) 2017-2020 The UnemployCoin Core developers
// Copyright (C) 2024-2025 UnemployCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UNEMPLOYCOIN_CONSENSUS_TX_VERIFY_H
#define UNEMPLOYCOIN_CONSENSUS_TX_VERIFY_H

#include "amount.h"

#include <stdint.h>
#include <vector>
#include <string>
#include <set>

class CBlockIndex;
class CCoinsViewCache;
class CTransaction;
class CValidationState;
class CAssetsCache;
class CTxOut;
class uint256;
class CMessage;
class CNullAssetTxData;

/** Transaction validation functions */

/** Context-independent validity checks */
bool CheckTransaction(const CTransaction& tx, CValidationState& state, bool fCheckDuplicateInputs=true, bool fMempoolCheck = false, bool fBlockCheck = false);

namespace Consensus {
/**
 * Check whether all inputs of this transaction are valid (no double spends and amounts)
 * This does not modify the UTXO set. This does not check scripts and sigs.
 * @param[out] txfee Set to the transaction fee if successful.
 * Preconditions: tx.IsCoinBase() is false.
 */
bool CheckTxInputs(const CTransaction& tx, CValidationState& state, const CCoinsViewCache& inputs, int nSpendHeight, CAmount& txfee);

/** UNEMP START */
bool CheckTxAssets(const CTransaction& tx, CValidationState& state, const CCoinsViewCache& inputs, CAssetsCache* assetCache, bool fCheckMempool, std::vector<std::pair<std::string, uint256> >& vPairReissueAssets, const bool fRunningUnitTests = false, std::set<CMessage>* setMessages = nullptr, int64_t nBlocktime = 0,  std::vector<std::pair<std::string, CNullAssetTxData>>* myNullAssetData = nullptr);
/** UNEMP END */
} // namespace Consensus

/** Auxiliary functions for transaction validation (ideally should not be exposed) */

/**
 * Count ECDSA signature operations the old-fashioned (pre-0.6) way
 * @return number of sigops this transaction's outputs will produce when spent
 * @see CTransaction::FetchInputs
 */
unsigned int GetLegacySigOpCount(const CTransaction& tx);

/**
 * Count ECDSA signature operations in pay-to-script-hash inputs.
 * 
 * @param[in] mapInputs Map of previous transactions that have outputs we're spending
 * @return maximum number of sigops required to validate this transaction's inputs
 * @see CTransaction::FetchInputs
 */
unsigned int GetP2SHSigOpCount(const CTransaction& tx, const CCoinsViewCache& mapInputs);

/**
 * Compute total signature operation cost of a transaction.
 * @param[in] tx     Transaction for which we are computing the cost
 * @param[in] inputs Map of previous transactions that have outputs we're spending
 * @param[out] flags Script verification flags
 * @return Total signature operation cost of tx
 */
int64_t GetTransactionSigOpCost(const CTransaction& tx, const CCoinsViewCache& inputs, int flags);

/**
 * Check if transaction is final and can be included in a block with the
 * specified height and time. Consensus critical.
 */
bool IsFinalTx(const CTransaction &tx, int nBlockHeight, int64_t nBlockTime);

/**
 * Calculates the block height and previous block's median time past at
 * which the transaction will be considered final in the context of BIP 68.
 * Also removes from the vector of input heights any entries which did not
 * correspond to sequence locked inputs as they do not affect the calculation.
 */
std::pair<int, int64_t> CalculateSequenceLocks(const CTransaction &tx, int flags, std::vector<int>* prevHeights, const CBlockIndex& block);

bool EvaluateSequenceLocks(const CBlockIndex& block, std::pair<int, int64_t> lockPair);
/**
 * Check if transaction is final per BIP 68 sequence numbers and can be included in a block.
 * Consensus critical. Takes as input a list of heights at which tx's inputs (in order) confirmed.
 */
bool SequenceLocks(const CTransaction &tx, int flags, std::vector<int>* prevHeights, const CBlockIndex& block);

#endif // UNEMPLOYCOIN_CONSENSUS_TX_VERIFY_H
