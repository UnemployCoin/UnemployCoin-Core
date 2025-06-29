// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2021 The UnemployCoin Core developers
// Copyright (c) 2024-2025 The UnemployCoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UNEMPLOYCOIN_ADDRESSINDEX_H
#define UNEMPLOYCOIN_ADDRESSINDEX_H

#include "uint256.h"
#include "amount.h"
#include "script/script.h"

static const std::string UNEMP = "UNEMP";

struct CAddressUnspentKey {
    unsigned int type;
    uint160 hashBytes;
    std::string asset;
    uint256 txhash;
    size_t index;

    size_t GetSerializeSize() const {
        return 57 + asset.size();
    }
    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata8(s, type);
        hashBytes.Serialize(s);
        ::Serialize(s, asset);
        txhash.Serialize(s);
        ser_writedata32(s, index);
    }
    template<typename Stream>
    void Unserialize(Stream& s) {
        type = ser_readdata8(s);
        hashBytes.Unserialize(s);
        ::Unserialize(s, asset);
        txhash.Unserialize(s);
        index = ser_readdata32(s);
    }

    CAddressUnspentKey(unsigned int addressType, uint160 addressHash, uint256 txid, size_t indexValue) {
        type = addressType;
        hashBytes = addressHash;
        asset = UNEMP;
        txhash = txid;
        index = indexValue;
    }

    CAddressUnspentKey(unsigned int addressType, uint160 addressHash, std::string assetName, uint256 txid, size_t indexValue) {
        type = addressType;
        hashBytes = addressHash;
        asset = assetName;
        txhash = txid;
        index = indexValue;
    }

    CAddressUnspentKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        asset.clear();
        txhash.SetNull();
        index = 0;
    }
};

struct CAddressUnspentValue {
    CAmount satoshis;
    CScript script;
    int blockHeight;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(satoshis);
        READWRITE(*(CScriptBase*)(&script));
        READWRITE(blockHeight);
    }

    CAddressUnspentValue(CAmount sats, CScript scriptPubKey, int height) {
        satoshis = sats;
        script = scriptPubKey;
        blockHeight = height;
    }

    CAddressUnspentValue() {
        SetNull();
    }

    void SetNull() {
        satoshis = -1;
        script.clear();
        blockHeight = 0;
    }

    bool IsNull() const {
        return (satoshis == -1);
    }
};

struct CAddressIndexKey {
    unsigned int type;
    uint160 hashBytes;
    std::string asset;
    int blockHeight;
    unsigned int txindex;
    uint256 txhash;
    size_t index;
    bool spending;

    size_t GetSerializeSize() const {
        return 34 + asset.size();
    }
    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata8(s, type);
        hashBytes.Serialize(s);
        ::Serialize(s, asset);
        // Heights are stored big-endian for key sorting in LevelDB
        ser_writedata32be(s, blockHeight);
        ser_writedata32be(s, txindex);
        txhash.Serialize(s);
        ser_writedata32(s, index);
        char f = spending;
        ser_writedata8(s, f);
    }
    template<typename Stream>
    void Unserialize(Stream& s) {
        type = ser_readdata8(s);
        hashBytes.Unserialize(s);
        ::Unserialize(s, asset);
        blockHeight = ser_readdata32be(s);
        txindex = ser_readdata32be(s);
        txhash.Unserialize(s);
        index = ser_readdata32(s);
        char f = ser_readdata8(s);
        spending = f;
    }

    CAddressIndexKey(unsigned int addressType, uint160 addressHash, int height, int blockindex,
                     uint256 txid, size_t indexValue, bool isSpending) {
        type = addressType;
        hashBytes = addressHash;
        asset = UNEMP;
        blockHeight = height;
        txindex = blockindex;
        txhash = txid;
        index = indexValue;
        spending = isSpending;
    }

    CAddressIndexKey(unsigned int addressType, uint160 addressHash, std::string assetName, int height, int blockindex,
                     uint256 txid, size_t indexValue, bool isSpending) {
        type = addressType;
        hashBytes = addressHash;
        asset = assetName;
        blockHeight = height;
        txindex = blockindex;
        txhash = txid;
        index = indexValue;
        spending = isSpending;
    }

    CAddressIndexKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        asset.clear();
        blockHeight = 0;
        txindex = 0;
        txhash.SetNull();
        index = 0;
        spending = false;
    }

};

struct CAddressIndexIteratorKey {
    unsigned int type;
    uint160 hashBytes;

    size_t GetSerializeSize() const {
        return 21;
    }
    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata8(s, type);
        hashBytes.Serialize(s);
    }
    template<typename Stream>
    void Unserialize(Stream& s) {
        type = ser_readdata8(s);
        hashBytes.Unserialize(s);
    }

    CAddressIndexIteratorKey(unsigned int addressType, uint160 addressHash) {
        type = addressType;
        hashBytes = addressHash;
    }

    CAddressIndexIteratorKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
    }
};

struct CAddressIndexIteratorAssetKey {
    unsigned int type;
    uint160 hashBytes;
    std::string asset;

    size_t GetSerializeSize() const {
        return 21 + asset.size();
    }
    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata8(s, type);
        hashBytes.Serialize(s);
        ::Serialize(s, asset);
    }
    template<typename Stream>
    void Unserialize(Stream& s) {
        type = ser_readdata8(s);
        hashBytes.Unserialize(s);
        ::Unserialize(s, asset);
    }

    CAddressIndexIteratorAssetKey(unsigned int addressType, uint160 addressHash) {
        type = addressType;
        hashBytes = addressHash;
        asset = UNEMP;
    }

    CAddressIndexIteratorAssetKey(unsigned int addressType, uint160 addressHash, std::string assetName) {
        type = addressType;
        hashBytes = addressHash;
        asset = assetName;
    }

    CAddressIndexIteratorAssetKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        asset.clear();
    }
};

struct CAddressIndexIteratorHeightKey {
    unsigned int type;
    uint160 hashBytes;
    std::string asset;
    int blockHeight;

    size_t GetSerializeSize() const {
        return 25 + asset.size();
    }
    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata8(s, type);
        hashBytes.Serialize(s);
        ::Serialize(s, asset);
        ser_writedata32be(s, blockHeight);
    }
    template<typename Stream>
    void Unserialize(Stream& s) {
        type = ser_readdata8(s);
        hashBytes.Unserialize(s);
        ::Unserialize(s, asset);
        blockHeight = ser_readdata32be(s);
    }

    CAddressIndexIteratorHeightKey(unsigned int addressType, uint160 addressHash, int height) {
        type = addressType;
        hashBytes = addressHash;
        asset = UNEMP;
        blockHeight = height;
    }

    CAddressIndexIteratorHeightKey(unsigned int addressType, uint160 addressHash, std::string assetName, int height) {
        type = addressType;
        hashBytes = addressHash;
        asset = assetName;
        blockHeight = height;
    }

    CAddressIndexIteratorHeightKey() {
        SetNull();
    }

    void SetNull() {
        type = 0;
        hashBytes.SetNull();
        asset.clear();
        blockHeight = 0;
    }
};

struct CMempoolAddressDelta
{
    int64_t time;
    CAmount amount;
    uint256 prevhash;
    unsigned int prevout;

    CMempoolAddressDelta(int64_t t, CAmount a, uint256 hash, unsigned int out) {
        time = t;
        amount = a;
        prevhash = hash;
        prevout = out;
    }

    CMempoolAddressDelta(int64_t t, CAmount a) {
        time = t;
        amount = a;
        prevhash.SetNull();
        prevout = 0;
    }
};

struct CMempoolAddressDeltaKey
{
    int type;
    uint160 addressBytes;
    std::string asset;
    uint256 txhash;
    unsigned int index;
    int spending;

    CMempoolAddressDeltaKey(int addressType, uint160 addressHash, std::string assetName,
                            uint256 hash, unsigned int i, int s) {
        type = addressType;
        addressBytes = addressHash;
        asset = assetName;
        txhash = hash;
        index = i;
        spending = s;
    }

    CMempoolAddressDeltaKey(int addressType, uint160 addressHash, uint256 hash, unsigned int i, int s) {
        type = addressType;
        addressBytes = addressHash;
        asset = "";
        txhash = hash;
        index = i;
        spending = s;
    }

    CMempoolAddressDeltaKey(int addressType, uint160 addressHash, std::string assetName) {
        type = addressType;
        addressBytes = addressHash;
        asset = assetName;
        txhash.SetNull();
        index = 0;
        spending = 0;
    }

    CMempoolAddressDeltaKey(int addressType, uint160 addressHash) {
        type = addressType;
        addressBytes = addressHash;
        asset = "";
        txhash.SetNull();
        index = 0;
        spending = 0;
    }
};

struct CMempoolAddressDeltaKeyCompare
{
    bool operator()(const CMempoolAddressDeltaKey& a, const CMempoolAddressDeltaKey& b) const {
        if (a.type == b.type) {
            if (a.addressBytes == b.addressBytes) {
                if (a.asset == b.asset) {
                    if (a.txhash == b.txhash) {
                        if (a.index == b.index) {
                            return a.spending < b.spending;
                        } else {
                            return a.index < b.index;
                        }
                    } else {
                        return a.txhash < b.txhash;
                    }
                } else {
                    return a.asset < b.asset;
                }
            } else {
                return a.addressBytes < b.addressBytes;
            }
        } else {
            return a.type < b.type;
        }
    }
};

#endif // UNEMPLOYCOIN_ADDRESSINDEX_H
