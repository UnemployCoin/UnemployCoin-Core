// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2021 The UnemployCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "chain.h"
#include "clientversion.h"
#include "core_io.h"
#include "init.h"
#include "validation.h"
#include "httpserver.h"
#include "net.h"
#include "netbase.h"
#include "rpc/blockchain.h"
#include "rpc/server.h"
#include "timedata.h"
#include "txmempool.h"
#include "util.h"
#include "utilstrencodings.h"
#ifdef ENABLE_WALLET
#include "wallet/rpcwallet.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#endif
#include "warnings.h"

#include <stdint.h>
#ifdef HAVE_MALLOC_INFO
#include <malloc.h>
#endif

#include <univalue.h>

/**
 * @note Do not add or change anything in the information returned by this
 * method. `getinfo` exists for backwards-compatibility only. It combines
 * information from wildly different sources in the program, which is a mess,
 * and is thus planned to be deprecated eventually.
 *
 * Based on the source of the information, new information should be added to:
 * - `getblockchaininfo`,
 * - `getnetworkinfo` or
 * - `getwalletinfo`
 *
 * Or alternatively, create a specific query method for the information.
 **/
UniValue getinfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
            "getinfo\n"
            "\nDEPRECATED. Returns an object containing various state info.\n"
            "\nResult:\n"
            "{\n"
            "  \"deprecation-warning\": \"...\" (string) warning that the getinfo command is deprecated and will be removed in 0.16\n"
            "  \"version\": xxxxx,           (numeric) the server version\n"
            "  \"protocolversion\": xxxxx,   (numeric) the protocol version\n"
            "  \"walletversion\": xxxxx,     (numeric) the wallet version\n"
            "  \"balance\": xxxxxxx,         (numeric) the total UnemployCoin balance of the wallet\n"
            "  \"blocks\": xxxxxx,           (numeric) the current number of blocks processed in the server\n"
            "  \"timeoffset\": xxxxx,        (numeric) the time offset\n"
            "  \"connections\": xxxxx,       (numeric) the number of connections\n"
            "  \"proxy\": \"host:port\",       (string, optional) the proxy used by the server\n"
            "  \"difficulty\": xxxxxx,       (numeric) the current difficulty\n"
            "  \"testnet\": true|false,      (boolean) if the server is using testnet or not\n"
            "  \"keypoololdest\": xxxxxx,    (numeric) the timestamp (seconds since Unix epoch) of the oldest pre-generated key in the key pool\n"
            "  \"keypoolsize\": xxxx,        (numeric) how many new keys are pre-generated\n"
            "  \"unlocked_until\": ttt,      (numeric) the timestamp in seconds since epoch (midnight Jan 1 1970 GMT) that the wallet is unlocked for transfers, or 0 if the wallet is locked\n"
            "  \"paytxfee\": x.xxxx,         (numeric) the transaction fee set in " + CURRENCY_UNIT + "/kB\n"
            "  \"relayfee\": x.xxxx,         (numeric) minimum relay fee for transactions in " + CURRENCY_UNIT + "/kB\n"
            "  \"errors\": \"...\"             (string) any error messages\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getinfo", "")
            + HelpExampleRpc("getinfo", "")
        );

#ifdef ENABLE_WALLET
    CWallet * const pwallet = GetWalletForJSONRPCRequest(request);

    LOCK2(cs_main, pwallet ? &pwallet->cs_wallet : nullptr);
#else
    LOCK(cs_main);
#endif

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("deprecation-warning", "WARNING: getinfo is deprecated and will be fully removed in 0.16."
        " Projects should transition to using getblockchaininfo, getnetworkinfo, and getwalletinfo before upgrading to 0.16");
    obj.pushKV("version", CLIENT_VERSION);
    obj.pushKV("protocolversion", PROTOCOL_VERSION);
#ifdef ENABLE_WALLET
    if (pwallet) {
        obj.pushKV("walletversion", pwallet->GetVersion());
        obj.pushKV("balance",       ValueFromAmount(pwallet->GetBalance()));
    }
#endif
    obj.pushKV("blocks",        (int)chainActive.Height());
    obj.pushKV("timeoffset",    GetTimeOffset());
    if(g_connman)
        obj.pushKV("connections",   (int)g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL));
    obj.pushKV("proxy",         (proxy.IsValid() ? proxy.proxy.ToStringIPPort() : std::string()));
    obj.pushKV("difficulty",    (double)GetDifficulty());
    obj.pushKV("testnet", GetParams().NetworkIDString() == CBaseChainParams::TESTNET);
#ifdef ENABLE_WALLET
    if (pwallet) {
        obj.pushKV("keypoololdest", pwallet->GetOldestKeyPoolTime());
        obj.pushKV("keypoolsize",   (int)pwallet->GetKeyPoolSize());
    }
    if (pwallet && pwallet->IsCrypted()) {
        obj.pushKV("unlocked_until", pwallet->nRelockTime);
    }
    obj.pushKV("paytxfee",      ValueFromAmount(payTxFee.GetFeePerK()));
#endif
    obj.pushKV("relayfee",      ValueFromAmount(::minRelayTxFee.GetFeePerK()));
    obj.pushKV("errors",        GetWarnings("statusbar"));
    return obj;
}


#ifdef ENABLE_WALLET
class DescribeAddressVisitor : public boost::static_visitor<UniValue>
{
public:
    CWallet * const pwallet;

    explicit DescribeAddressVisitor(CWallet *_pwallet) : pwallet(_pwallet) {}

    UniValue operator()(const CNoDestination &dest) const { return UniValue(UniValue::VOBJ); }

    UniValue operator()(const CKeyID &keyID) const {
        UniValue obj(UniValue::VOBJ);
        CPubKey vchPubKey;
        obj.pushKV("isscript", false);
        if (pwallet && pwallet->GetPubKey(keyID, vchPubKey)) {
            obj.pushKV("pubkey", HexStr(vchPubKey));
            obj.pushKV("iscompressed", vchPubKey.IsCompressed());
        }
        return obj;
    }

    UniValue operator()(const CScriptID &scriptID) const {
        UniValue obj(UniValue::VOBJ);
        CScript subscript;
        obj.pushKV("isscript", true);
        if (pwallet && pwallet->GetCScript(scriptID, subscript)) {
            std::vector<CTxDestination> addresses;
            txnouttype whichType;
            int nRequired;
            ExtractDestinations(subscript, whichType, addresses, nRequired);
            obj.pushKV("script", GetTxnOutputType(whichType));
            obj.pushKV("hex", HexStr(subscript.begin(), subscript.end()));
            UniValue a(UniValue::VARR);
            for (const CTxDestination& addr : addresses) {
                a.push_back(EncodeDestination(addr));
            }
            obj.pushKV("addresses", a);
            if (whichType == TX_MULTISIG)
                obj.pushKV("sigsrequired", nRequired);
        }
        return obj;
    }
};
#endif

UniValue validateaddress(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "validateaddress \"address\"\n"
            "\nReturn information about the given unemploycoin address.\n"
            "\nArguments:\n"
            "1. \"address\"     (string, required) The unemploycoin address to validate\n"
            "\nResult:\n"
            "{\n"
            "  \"isvalid\" : true|false,       (boolean) If the address is valid or not. If not, this is the only property returned.\n"
            "  \"address\" : \"address\", (string) The unemploycoin address validated\n"
            "  \"scriptPubKey\" : \"hex\",       (string) The hex encoded scriptPubKey generated by the address\n"
            "  \"ismine\" : true|false,        (boolean) If the address is yours or not\n"
            "  \"iswatchonly\" : true|false,   (boolean) If the address is watchonly\n"
            "  \"isscript\" : true|false,      (boolean) If the key is a script\n"
            "  \"script\" : \"type\"             (string, optional) The output script type. Possible types: nonstandard, pubkey, pubkeyhash, scripthash, multisig, nulldata, witness_v0_keyhash, witness_v0_scripthash\n"
            "  \"hex\" : \"hex\",                (string, optional) The redeemscript for the p2sh address\n"
            "  \"addresses\"                   (string, optional) Array of addresses associated with the known redeemscript\n"
            "    [\n"
            "      \"address\"\n"
            "      ,...\n"
            "    ]\n"
            "  \"sigsrequired\" : xxxxx        (numeric, optional) Number of signatures required to spend multisig output\n"
            "  \"pubkey\" : \"publickeyhex\",    (string) The hex value of the raw public key\n"
            "  \"iscompressed\" : true|false,  (boolean) If the address is compressed\n"
            "  \"account\" : \"account\"         (string) DEPRECATED. The account associated with the address, \"\" is the default account\n"
            "  \"timestamp\" : timestamp,        (number, optional) The creation time of the key if available in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"hdkeypath\" : \"keypath\"       (string, optional) The HD keypath if the key is HD and available\n"
            "  \"hdmasterkeyid\" : \"<hash160>\" (string, optional) The Hash160 of the HD master pubkey\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("validateaddress", "\"1PSSGeFHDnKNxiEyFrD1wcEaHr9hrQDDWc\"")
            + HelpExampleRpc("validateaddress", "\"1PSSGeFHDnKNxiEyFrD1wcEaHr9hrQDDWc\"")
        );

#ifdef ENABLE_WALLET
    CWallet * const pwallet = GetWalletForJSONRPCRequest(request);

    LOCK2(cs_main, pwallet ? &pwallet->cs_wallet : nullptr);
#else
    LOCK(cs_main);
#endif

    CTxDestination dest = DecodeDestination(request.params[0].get_str());
    bool isValid = IsValidDestination(dest);

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("isvalid", isValid);
    if (isValid)
    {
        std::string currentAddress = EncodeDestination(dest);
        ret.pushKV("address", currentAddress);

        CScript scriptPubKey = GetScriptForDestination(dest);
        ret.pushKV("scriptPubKey", HexStr(scriptPubKey.begin(), scriptPubKey.end()));

#ifdef ENABLE_WALLET
        isminetype mine = pwallet ? IsMine(*pwallet, dest) : ISMINE_NO;
        ret.pushKV("ismine", bool(mine & ISMINE_SPENDABLE));
        ret.pushKV("iswatchonly", bool(mine & ISMINE_WATCH_ONLY));
        UniValue detail = boost::apply_visitor(DescribeAddressVisitor(pwallet), dest);
        ret.pushKVs(detail);
        if (pwallet && pwallet->mapAddressBook.count(dest)) {
            ret.pushKV("account", pwallet->mapAddressBook[dest].name);
        }
        if (pwallet) {
            const auto& meta = pwallet->mapKeyMetadata;
            const CKeyID *keyID = boost::get<CKeyID>(&dest);
            auto it = keyID ? meta.find(*keyID) : meta.end();
            if (it == meta.end()) {
                it = meta.find(CScriptID(scriptPubKey));
            }
            if (it != meta.end()) {
                ret.pushKV("timestamp", it->second.nCreateTime);
                if (!it->second.hdKeypath.empty()) {
                    ret.pushKV("hdkeypath", it->second.hdKeypath);
                    ret.pushKV("hdseedid", it->second.hd_seed_id.GetHex());
                }
            }
        }
#endif
    }
    return ret;
}

// Needed even with !ENABLE_WALLET, to pass (ignored) pointers around
class CWallet;

/**
 * Used by addmultisigaddress / createmultisig:
 */
CScript _createmultisig_redeemScript(CWallet * const pwallet, const UniValue& params)
{
    int nRequired = params[0].getInt<int>();
    const UniValue& keys = params[1].get_array();

    // Gather public keys
    if (nRequired < 1)
        throw std::runtime_error("a multisignature address must require at least one key to redeem");
    if ((int)keys.size() < nRequired)
        throw std::runtime_error(
            strprintf("not enough keys supplied "
                      "(got %u keys, but need at least %d to redeem)", keys.size(), nRequired));
    if (keys.size() > 16)
        throw std::runtime_error("Number of addresses involved in the multisignature address creation > 16\nReduce the number");
    std::vector<CPubKey> pubkeys;
    pubkeys.resize(keys.size());
    for (unsigned int i = 0; i < keys.size(); i++)
    {
        const std::string& ks = keys[i].get_str();
#ifdef ENABLE_WALLET
        // Case 1: UnemployCoin address and we have full public key:
        CTxDestination dest = DecodeDestination(ks);
        if (pwallet && IsValidDestination(dest)) {
            const CKeyID *keyID = boost::get<CKeyID>(&dest);
            if (!keyID) {
                throw std::runtime_error(strprintf("%s does not refer to a key", ks));
            }
            CPubKey vchPubKey;
            if (!pwallet->GetPubKey(*keyID, vchPubKey)) {
                throw std::runtime_error(strprintf("no full public key for address %s", ks));
            }
            if (!vchPubKey.IsFullyValid())
                throw std::runtime_error(" Invalid public key: "+ks);
            pubkeys[i] = vchPubKey;
        }

        // Case 2: hex public key
        else
#endif
        if (IsHex(ks))
        {
            CPubKey vchPubKey(ParseHex(ks));
            if (!vchPubKey.IsFullyValid())
                throw std::runtime_error(" Invalid public key: "+ks);
            pubkeys[i] = vchPubKey;
        }
        else
        {
            throw std::runtime_error(" Invalid public key: "+ks);
        }
    }
    CScript result = GetScriptForMultisig(nRequired, pubkeys);

    if (result.size() > MAX_SCRIPT_ELEMENT_SIZE)
        throw std::runtime_error(
                strprintf("redeemScript exceeds size limit: %d > %d", result.size(), MAX_SCRIPT_ELEMENT_SIZE));

    return result;
}

UniValue createmultisig(const JSONRPCRequest& request)
{
#ifdef ENABLE_WALLET
    CWallet * const pwallet = GetWalletForJSONRPCRequest(request);
#else
    CWallet * const pwallet = nullptr;
#endif

    if (request.fHelp || request.params.size() < 2 || request.params.size() > 2)
    {
        std::string msg = "createmultisig nrequired [\"key\",...]\n"
            "\nCreates a multi-signature address with n signature of m keys required.\n"
            "It returns a json object with the address and redeemScript.\n"

            "\nArguments:\n"
            "1. nrequired      (numeric, required) The number of required signatures out of the n keys or addresses.\n"
            "2. \"keys\"       (string, required) A json array of keys which are unemploycoin addresses or hex-encoded public keys\n"
            "     [\n"
            "       \"key\"    (string) unemploycoin address or hex-encoded public key\n"
            "       ,...\n"
            "     ]\n"

            "\nResult:\n"
            "{\n"
            "  \"address\":\"multisigaddress\",  (string) The value of the new multisig address.\n"
            "  \"redeemScript\":\"script\"       (string) The string value of the hex-encoded redemption script.\n"
            "}\n"

            "\nExamples:\n"
            "\nCreate a multisig address from 2 addresses\n"
            + HelpExampleCli("createmultisig", "2 \"[\\\"16sSauSf5pF2UkUwvKGq4qjNRzBZYqgEL5\\\",\\\"171sgjn4YtPu27adkKGrdDwzRTxnRkBfKV\\\"]\"") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("createmultisig", "2, \"[\\\"16sSauSf5pF2UkUwvKGq4qjNRzBZYqgEL5\\\",\\\"171sgjn4YtPu27adkKGrdDwzRTxnRkBfKV\\\"]\"")
        ;
        throw std::runtime_error(msg);
    }

    // Construct using pay-to-script-hash:
    CScript inner = _createmultisig_redeemScript(pwallet, request.params);
    CScriptID innerID(inner);

    UniValue result(UniValue::VOBJ);
    result.pushKV("address", EncodeDestination(innerID));
    result.pushKV("redeemScript", HexStr(inner.begin(), inner.end()));

    return result;
}

UniValue verifymessage(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 3)
        throw std::runtime_error(
            "verifymessage \"address\" \"signature\" \"message\"\n"
            "\nVerify a signed message\n"
            "\nArguments:\n"
            "1. \"address\"         (string, required) The unemploycoin address to use for the signature.\n"
            "2. \"signature\"       (string, required) The signature provided by the signer in base 64 encoding (see signmessage).\n"
            "3. \"message\"         (string, required) The message that was signed.\n"
            "\nResult:\n"
            "true|false   (boolean) If the signature is verified or not.\n"
            "\nExamples:\n"
            "\nUnlock the wallet for 30 seconds\n"
            + HelpExampleCli("walletpassphrase", "\"mypassphrase\" 30") +
            "\nCreate the signature\n"
            + HelpExampleCli("signmessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\" \"my message\"") +
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\" \"signature\" \"my message\"") +
            "\nAs json rpc\n"
            + HelpExampleRpc("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\", \"signature\", \"my message\"")
        );

    LOCK(cs_main);

    std::string strAddress  = request.params[0].get_str();
    std::string strSign     = request.params[1].get_str();
    std::string strMessage  = request.params[2].get_str();

    CTxDestination destination = DecodeDestination(strAddress);
    if (!IsValidDestination(destination)) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");
    }

    const CKeyID *keyID = boost::get<CKeyID>(&destination);
    if (!keyID) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");
    }

    bool fInvalid = false;
    std::vector<unsigned char> vchSig = DecodeBase64(strSign.c_str(), &fInvalid);

    if (fInvalid)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig))
        return false;

    return (pubkey.GetID() == *keyID);
}

UniValue signmessagewithprivkey(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        throw std::runtime_error(
            "signmessagewithprivkey \"privkey\" \"message\"\n"
            "\nSign a message with the private key of an address\n"
            "\nArguments:\n"
            "1. \"privkey\"         (string, required) The private key to sign the message with.\n"
            "2. \"message\"         (string, required) The message to create a signature of.\n"
            "\nResult:\n"
            "\"signature\"          (string) The signature of the message encoded in base 64\n"
            "\nExamples:\n"
            "\nCreate the signature\n"
            + HelpExampleCli("signmessagewithprivkey", "\"privkey\" \"my message\"") +
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\" \"signature\" \"my message\"") +
            "\nAs json rpc\n"
            + HelpExampleRpc("signmessagewithprivkey", "\"privkey\", \"my message\"")
        );

    std::string strPrivkey = request.params[0].get_str();
    std::string strMessage = request.params[1].get_str();

    CUnemployCoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strPrivkey);
    if (!fGood)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
    CKey key = vchSecret.GetKey();
    if (!key.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key outside allowed range");

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    std::vector<unsigned char> vchSig;
    if (!key.SignCompact(ss.GetHash(), vchSig))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sign failed");

    return EncodeBase64(vchSig.data(), vchSig.size());
}

UniValue setmocktime(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "setmocktime timestamp\n"
            "\nSet the local time to given timestamp (-regtest only)\n"
            "\nArguments:\n"
            "1. timestamp  (integer, required) Unix seconds-since-epoch timestamp\n"
            "   Pass 0 to go back to using the system time."
        );

    if (!GetParams().MineBlocksOnDemand())
        throw std::runtime_error("setmocktime for regression testing (-regtest mode) only");

    // For now, don't change mocktime if we're in the middle of validation, as
    // this could have an effect on mempool time-based eviction, as well as
    // IsCurrentForFeeEstimation() and IsInitialBlockDownload().
    // TODO: figure out the right way to synchronize around mocktime, and
    // ensure all call sites of GetTime() are accessing this safely.
    LOCK(cs_main);

    RPCTypeCheck(request.params, {UniValue::VNUM});
    SetMockTime(request.params[0].getInt<int64_t>());

    return NullUniValue;
}

static UniValue RPCLockedMemoryInfo()
{
    LockedPool::Stats stats = LockedPoolManager::Instance().stats();
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("used", uint64_t(stats.used));
    obj.pushKV("free", uint64_t(stats.free));
    obj.pushKV("total", uint64_t(stats.total));
    obj.pushKV("locked", uint64_t(stats.locked));
    obj.pushKV("chunks_used", uint64_t(stats.chunks_used));
    obj.pushKV("chunks_free", uint64_t(stats.chunks_free));
    return obj;
}

#ifdef HAVE_MALLOC_INFO
static std::string RPCMallocInfo()
{
    char *ptr = nullptr;
    size_t size = 0;
    FILE *f = open_memstream(&ptr, &size);
    if (f) {
        malloc_info(0, f);
        fclose(f);
        if (ptr) {
            std::string rv(ptr, size);
            free(ptr);
            return rv;
        }
    }
    return "";
}
#endif

UniValue getmemoryinfo(const JSONRPCRequest& request)
{
    /* Please, avoid using the word "pool" here in the RPC interface or help,
     * as users will undoubtedly confuse it with the other "memory pool"
     */
    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "getmemoryinfo (\"mode\")\n"
            "Returns an object containing information about memory usage.\n"
            "Arguments:\n"
            "1. \"mode\" determines what kind of information is returned. This argument is optional, the default mode is \"stats\".\n"
            "  - \"stats\" returns general statistics about memory usage in the daemon.\n"
            "  - \"mallocinfo\" returns an XML string describing low-level heap state (only available if compiled with glibc 2.10+).\n"
            "\nResult (mode \"stats\"):\n"
            "{\n"
            "  \"locked\": {               (json object) Information about locked memory manager\n"
            "    \"used\": xxxxx,          (numeric) Number of bytes used\n"
            "    \"free\": xxxxx,          (numeric) Number of bytes available in current arenas\n"
            "    \"total\": xxxxxxx,       (numeric) Total number of bytes managed\n"
            "    \"locked\": xxxxxx,       (numeric) Amount of bytes that succeeded locking. If this number is smaller than total, locking pages failed at some point and key data could be swapped to disk.\n"
            "    \"chunks_used\": xxxxx,   (numeric) Number allocated chunks\n"
            "    \"chunks_free\": xxxxx,   (numeric) Number unused chunks\n"
            "  }\n"
            "}\n"
            "\nResult (mode \"mallocinfo\"):\n"
            "\"<malloc version=\"1\">...\"\n"
            "\nExamples:\n"
            + HelpExampleCli("getmemoryinfo", "")
            + HelpExampleRpc("getmemoryinfo", "")
        );

    std::string mode = request.params[0].isNull() ? "stats" : request.params[0].get_str();
    if (mode == "stats") {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("locked", RPCLockedMemoryInfo());
        return obj;
    } else if (mode == "mallocinfo") {
#ifdef HAVE_MALLOC_INFO
        return RPCMallocInfo();
#else
        throw JSONRPCError(RPC_INVALID_PARAMETER, "mallocinfo is only available when compiled with glibc 2.10+");
#endif
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "unknown mode " + mode);
    }
}

uint32_t getCategoryMask(UniValue cats) {
    cats = cats.get_array();
    uint32_t mask = 0;
    for (unsigned int i = 0; i < cats.size(); ++i) {
        uint32_t flag = 0;
        std::string cat = cats[i].get_str();
        if (!GetLogCategory(&flag, &cat)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "unknown logging category " + cat);
        }
        mask |= flag;
    }
    return mask;
}

UniValue logging(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 2) {
        throw std::runtime_error(
            "logging [include,...] <exclude>\n"
            "Gets and sets the logging configuration.\n"
            "When called without an argument, returns the list of categories that are currently being debug logged.\n"
            "When called with arguments, adds or removes categories from debug logging.\n"
            "The valid logging categories are: " + ListLogCategories() + "\n"
            "libevent logging is configured on startup and cannot be modified by this RPC during runtime."
            "Arguments:\n"
            "1. \"include\" (array of strings) add debug logging for these categories.\n"
            "2. \"exclude\" (array of strings) remove debug logging for these categories.\n"
            "\nResult: <categories>  (string): a list of the logging categories that are active.\n"
            "\nExamples:\n"
            + HelpExampleCli("logging", "\"[\\\"all\\\"]\" \"[\\\"http\\\"]\"")
            + HelpExampleRpc("logging", "[\"all\"], \"[libevent]\"")
        );
    }

    uint32_t originalLogCategories = logCategories;
    if (request.params[0].isArray()) {
        logCategories |= getCategoryMask(request.params[0]);
    }

    if (request.params[1].isArray()) {
        logCategories &= ~getCategoryMask(request.params[1]);
    }

    // Update libevent logging if BCLog::LIBEVENT has changed.
    // If the library version doesn't allow it, UpdateHTTPServerLogging() returns false,
    // in which case we should clear the BCLog::LIBEVENT flag.
    // Throw an error if the user has explicitly asked to change only the libevent
    // flag and it failed.
    uint32_t changedLogCategories = originalLogCategories ^ logCategories;
    if (changedLogCategories & BCLog::LIBEVENT) {
        if (!UpdateHTTPServerLogging(logCategories & BCLog::LIBEVENT)) {
            logCategories &= ~BCLog::LIBEVENT;
            if (changedLogCategories == BCLog::LIBEVENT) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "libevent logging cannot be updated when using libevent before v2.1.1.");
            }
        }
    }

    UniValue result(UniValue::VOBJ);
    std::vector<CLogCategoryActive> vLogCatActive = ListActiveLogCategories();
    for (const auto& logCatActive : vLogCatActive) {
        result.pushKV(logCatActive.category, logCatActive.active);
    }

    return result;
}

UniValue echo(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "echo|echojson \"message\" ...\n"
            "\nSimply echo back the input arguments. This command is for testing.\n"
            "\nThe difference between echo and echojson is that echojson has argument conversion enabled in the client-side table in"
            "unemploycoin-cli and the GUI. There is no server-side difference."
        );

    return request.params;
}

bool getAddressFromIndex(const int &type, const uint160 &hash, std::string &address)
{
    if (type == 2) {
        address = CUnemployCoinAddress(CScriptID(hash)).ToString();
    } else if (type == 1) {
        address = CUnemployCoinAddress(CKeyID(hash)).ToString();
    } else {
        return false;
    }
    return true;
}

bool getAddressesFromParams(const UniValue& params, std::vector<std::pair<uint160, int> > &addresses)
{
    if (params[0].isStr()) {
        CUnemployCoinAddress address(params[0].get_str());
        uint160 hashBytes;
        int type = 0;
        if (!address.GetIndexKey(hashBytes, type)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
        }
        addresses.push_back(std::make_pair(hashBytes, type));
    } else if (params[0].isObject()) {

        UniValue addressValues = find_value(params[0].get_obj(), "addresses");
        if (!addressValues.isArray()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Addresses is expected to be an array");
        }

        std::vector<UniValue> values = addressValues.getValues();

        for (std::vector<UniValue>::iterator it = values.begin(); it != values.end(); ++it) {

            CUnemployCoinAddress address(it->get_str());
            uint160 hashBytes;
            int type = 0;
            if (!address.GetIndexKey(hashBytes, type)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            }
            addresses.push_back(std::make_pair(hashBytes, type));
        }
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    return true;
}

bool heightSort(std::pair<CAddressUnspentKey, CAddressUnspentValue> a,
                std::pair<CAddressUnspentKey, CAddressUnspentValue> b) {
    return a.second.blockHeight < b.second.blockHeight;
}

bool timestampSort(std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> a,
                   std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> b) {
    return a.second.time < b.second.time;
}

UniValue getaddressmempool(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 2)
        throw std::runtime_error(
            "getaddressmempool\n"
            "\nReturns all mempool deltas for an address (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ]\n"
            "},\n"
            "\"includeAssets\" (boolean, optional, default false)  If true this will return an expanded result which includes asset deltas\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"address\"  (string) The base58check encoded address\n"
            "    \"assetName\"  (string) The name of the associated asset (UNEMP for UnemployCoin)\n"
            "    \"txid\"  (string) The related txid\n"
            "    \"index\"  (number) The related input or output index\n"
            "    \"satoshis\"  (number) The difference of satoshis\n"
            "    \"timestamp\"  (number) The time the transaction entered the mempool (seconds)\n"
            "    \"prevtxid\"  (string) The previous txid (if spending)\n"
            "    \"prevout\"  (string) The previous transaction output index (if spending)\n"
            "  }\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddressmempool", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
            + HelpExampleRpc("getaddressmempool", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
            + HelpExampleCli("getaddressmempool", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}', true")
            + HelpExampleRpc("getaddressmempool", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}, true")
        );

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    bool includeAssets = false;
    if (request.params.size() > 1) {
        includeAssets = request.params[1].get_bool();
    }

    if (includeAssets)
        if (!AreAssetsDeployed())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Assets aren't active.  includeAssets can't be true.");

    std::vector<std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> > indexes;

    if (includeAssets) {
        if (!mempool.getAddressIndex(addresses, indexes)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
        }
    } else {
        if (!mempool.getAddressIndex(addresses, UNEMP, indexes)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
        }
    }

    std::sort(indexes.begin(), indexes.end(), timestampSort);

    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> >::iterator it = indexes.begin();
         it != indexes.end(); it++) {

        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.addressBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        UniValue delta(UniValue::VOBJ);
        delta.pushKV("address", address);
        delta.pushKV("assetName", it->first.asset);
        delta.pushKV("txid", it->first.txhash.GetHex());
        delta.pushKV("index", (int)it->first.index);
        delta.pushKV("satoshis", it->second.amount);
        delta.pushKV("timestamp", it->second.time);
        if (it->second.amount < 0) {
            delta.pushKV("prevtxid", it->second.prevhash.GetHex());
            delta.pushKV("prevout", (int)it->second.prevout);
        }
        result.push_back(delta);
    }

    return result;
}

UniValue getaddressutxos(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getaddressutxos\n"
            "\nReturns all unspent outputs for an address (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ],\n"
            "  \"chainInfo\",  (boolean, optional, default false) Include chain info with results\n"
            "  \"assetName\"   (string, optional) Get UTXOs for a particular asset instead of UNEMP ('*' for all assets).\n"
            "}\n"
            "\nResult\n"
            "[\n"
            "  {\n"
            "    \"address\"  (string) The address base58check encoded\n"
            "    \"assetName\" (string) The asset associated with the UTXOs (UNEMP for UnemployCoin)\n"
            "    \"txid\"  (string) The output txid\n"
            "    \"height\"  (number) The block height\n"
            "    \"outputIndex\"  (number) The output index\n"
            "    \"script\"  (strin) The script hex encoded\n"
            "    \"satoshis\"  (number) The number of satoshis of the output\n"
            "  }\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddressutxos", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
            + HelpExampleRpc("getaddressutxos", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
            + HelpExampleCli("getaddressutxos", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"],\"assetName\":\"MY_ASSET\"}'")
            + HelpExampleRpc("getaddressutxos", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"],\"assetName\":\"MY_ASSET\"}")
            );

    bool includeChainInfo = false;
    std::string assetName = UNEMP;
    if (request.params[0].isObject()) {
        UniValue chainInfo = find_value(request.params[0].get_obj(), "chainInfo");
        if (chainInfo.isBool()) {
            includeChainInfo = chainInfo.get_bool();
        }
        UniValue assetNameParam = find_value(request.params[0].get_obj(), "assetName");
        if (assetNameParam.isStr()) {
            if (!AreAssetsDeployed())
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Assets aren't active.  assetName can't be specified.");
            assetName = assetNameParam.get_str();
        }
    }

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;

    for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (assetName == "*") {
            if (!GetAddressUnspent((*it).first, (*it).second, unspentOutputs)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        } else {
            if (!GetAddressUnspent((*it).first, (*it).second, assetName, unspentOutputs)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        }
    }

    std::sort(unspentOutputs.begin(), unspentOutputs.end(), heightSort);

    UniValue utxos(UniValue::VARR);

    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=unspentOutputs.begin(); it!=unspentOutputs.end(); it++) {
        UniValue output(UniValue::VOBJ);
        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.hashBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        std::string assetNameOut = "UNEMP";
        if (assetName != "UNEMP") {
            CAmount _amount;
            if (!GetAssetInfoFromScript(it->second.script, assetNameOut, _amount)) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Couldn't decode asset script");
            }
        }

        output.pushKV("address", address);
        output.pushKV("assetName", assetNameOut);
        output.pushKV("txid", it->first.txhash.GetHex());
        output.pushKV("outputIndex", (int)it->first.index);
        output.pushKV("script", HexStr(it->second.script.begin(), it->second.script.end()));
        output.pushKV("satoshis", it->second.satoshis);
        output.pushKV("height", it->second.blockHeight);
        utxos.push_back(output);
    }

    if (includeChainInfo) {
        UniValue result(UniValue::VOBJ);
        result.pushKV("utxos", utxos);

        LOCK(cs_main);
        result.pushKV("hash", chainActive.Tip()->GetBlockHash().GetHex());
        result.pushKV("height", (int)chainActive.Height());
        return result;
    } else {
        return utxos;
    }
}

UniValue getaddressdeltas(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1 || !request.params[0].isObject())
        throw std::runtime_error(
            "getaddressdeltas\n"
            "\nReturns all changes for an address (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ]\n"
            "  \"start\" (number) The start block height\n"
            "  \"end\" (number) The end block height\n"
            "  \"chainInfo\" (boolean) Include chain info in results, only applies if start and end specified\n"
            "  \"assetName\"   (string, optional) Get deltas for a particular asset instead of UNEMP.\n"
            "}\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"assetName\"  (string) The asset associated with the deltas (UNEMP for UnemployCoin)\n"
            "    \"satoshis\"  (number) The difference of satoshis\n"
            "    \"txid\"  (string) The related txid\n"
            "    \"index\"  (number) The related input or output index\n"
            "    \"height\"  (number) The block height\n"
            "    \"address\"  (string) The base58check encoded address\n"
            "  }\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddressdeltas", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
            + HelpExampleRpc("getaddressdeltas", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
            + HelpExampleCli("getaddressdeltas", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"],\"assetName\":\"MY_ASSET\"}'")
            + HelpExampleRpc("getaddressdeltas", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"],\"assetName\":\"MY_ASSET\"}")
        );


    UniValue startValue = find_value(request.params[0].get_obj(), "start");
    UniValue endValue = find_value(request.params[0].get_obj(), "end");

    UniValue chainInfo = find_value(request.params[0].get_obj(), "chainInfo");
    bool includeChainInfo = false;
    if (chainInfo.isBool()) {
        includeChainInfo = chainInfo.get_bool();
    }

    std::string assetName = UNEMP;
    UniValue assetNameParam = find_value(request.params[0].get_obj(), "assetName");
    if (assetNameParam.isStr()) {
        if (!AreAssetsDeployed())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Assets aren't active.  assetName can't be specified.");
        assetName = assetNameParam.get_str();
    }

    int start = 0;
    int end = 0;

    if (startValue.isNum() && endValue.isNum()) {
        start = startValue.getInt<int>();
        end = endValue.getInt<int>();
        if (start <= 0 || end <= 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Start and end is expected to be greater than zero");
        }
        if (end < start) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "End value is expected to be greater than start");
        }
    }

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;

    for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (start > 0 && end > 0) {
            if (!GetAddressIndex((*it).first, (*it).second, assetName, addressIndex, start, end)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        } else {
            if (!GetAddressIndex((*it).first, (*it).second, assetName, addressIndex)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        }
    }

    UniValue deltas(UniValue::VARR);

    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.hashBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        UniValue delta(UniValue::VOBJ);
        delta.pushKV("assetName", it->first.asset);
        delta.pushKV("satoshis", it->second);
        delta.pushKV("txid", it->first.txhash.GetHex());
        delta.pushKV("index", (int)it->first.index);
        delta.pushKV("blockindex", (int)it->first.txindex);
        delta.pushKV("height", it->first.blockHeight);
        delta.pushKV("address", address);
        deltas.push_back(delta);
    }

    UniValue result(UniValue::VOBJ);

    if (includeChainInfo && start > 0 && end > 0) {
        LOCK(cs_main);

        if (start > chainActive.Height() || end > chainActive.Height()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Start or end is outside chain range");
        }

        CBlockIndex* startIndex = chainActive[start];
        CBlockIndex* endIndex = chainActive[end];

        UniValue startInfo(UniValue::VOBJ);
        UniValue endInfo(UniValue::VOBJ);

        startInfo.pushKV("hash", startIndex->GetBlockHash().GetHex());
        startInfo.pushKV("height", start);

        endInfo.pushKV("hash", endIndex->GetBlockHash().GetHex());
        endInfo.pushKV("height", end);

        result.pushKV("deltas", deltas);
        result.pushKV("start", startInfo);
        result.pushKV("end", endInfo);

        return result;
    } else {
        return deltas;
    }
}

UniValue getaddressbalance(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 2)
        throw std::runtime_error(
            "getaddressbalance\n"
            "\nReturns the balance for an address(es) (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses:\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ]\n"
            "},\n"
            "\"includeAssets\" (boolean, optional, default false)  If true this will return an expanded result which includes asset balances\n"
            "\n"
            "\nResult:\n"
            "{\n"
            "  \"balance\"  (string) The current balance in satoshis\n"
            "  \"received\"  (string) The total number of satoshis received (including change)\n"
            "}\n"
            "OR\n"
            "[\n"
            "  {\n"
            "    \"assetName\"  (string) The asset associated with the balance (UNEMP for UnemployCoin)\n"
            "    \"balance\"  (string) The current balance in satoshis\n"
            "    \"received\"  (string) The total number of satoshis received (including change)\n"
            "  },...\n"
            "\n]"
            "\nExamples:\n"
            + HelpExampleCli("getaddressbalance", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
            + HelpExampleCli("getaddressbalance", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}', true")
            + HelpExampleRpc("getaddressbalance", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
            + HelpExampleRpc("getaddressbalance", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}, true")
        );

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    bool includeAssets = false;
    if (request.params.size() > 1) {
        includeAssets = request.params[1].get_bool();
    }

    if (includeAssets) {
        if (!AreAssetsDeployed())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Assets aren't active.  includeAssets can't be true.");

        std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;

        for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
            if (!GetAddressIndex((*it).first, (*it).second, addressIndex)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        }

        //assetName -> (received, balance)
        std::map<std::string, std::pair<CAmount, CAmount>> balances;

        for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it = addressIndex.begin();
             it != addressIndex.end(); it++) {
            std::string assetName = it->first.asset;
            if (balances.count(assetName) == 0) {
                balances[assetName] = std::make_pair(0, 0);
            }
            if (it->second > 0) {
                balances[assetName].first += it->second;
            }
            balances[assetName].second += it->second;
        }

        UniValue result(UniValue::VARR);

        for (std::map<std::string, std::pair<CAmount, CAmount>>::const_iterator it = balances.begin();
                it != balances.end(); it++) {
            UniValue balance(UniValue::VOBJ);
            balance.pushKV("assetName", it->first);
            balance.pushKV("balance", it->second.second);
            balance.pushKV("received", it->second.first);
            result.push_back(balance);
        }

        return result;

    } else {
        std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;

        for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
            if (!GetAddressIndex((*it).first, (*it).second, UNEMP, addressIndex)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        }

        CAmount balance = 0;
        CAmount received = 0;

        for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it = addressIndex.begin();
             it != addressIndex.end(); it++) {
            if (it->second > 0) {
                received += it->second;
            }
            balance += it->second;
        }

        UniValue result(UniValue::VOBJ);
        result.pushKV("balance", balance);
        result.pushKV("received", received);

        return result;
    }

}

UniValue getaddresstxids(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 2)
        throw std::runtime_error(
            "getaddresstxids\n"
            "\nReturns the txids for an address(es) (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ]\n"
            "  \"start\" (number, optional) The start block height\n"
            "  \"end\" (number, optional) The end block height\n"
            "},\n"
            "\"includeAssets\" (boolean, optional, default false)  If true this will return an expanded result which includes asset transactions\n"
            "\nResult:\n"
            "[\n"
            "  \"transactionid\"  (string) The transaction id\n"
            "  ,...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddresstxids", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
            + HelpExampleRpc("getaddresstxids", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
            + HelpExampleCli("getaddresstxids", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}', true")
            + HelpExampleRpc("getaddresstxids", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}, true")
        );

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    int start = 0;
    int end = 0;
    if (request.params[0].isObject()) {
        UniValue startValue = find_value(request.params[0].get_obj(), "start");
        UniValue endValue = find_value(request.params[0].get_obj(), "end");
        if (startValue.isNum() && endValue.isNum()) {
            start = startValue.getInt<int>();
            end = endValue.getInt<int>();
        }
    }

    bool includeAssets = false;
    if (request.params.size() > 1) {
        includeAssets = request.params[1].get_bool();
    }

    if (includeAssets)
        if (!AreAssetsDeployed())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Assets aren't active.  includeAssets can't be true.");

    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;

    for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (includeAssets) {
            if (start > 0 && end > 0) {
                if (!GetAddressIndex((*it).first, (*it).second, addressIndex, start, end)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
                }
            } else {
                if (!GetAddressIndex((*it).first, (*it).second, addressIndex)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
                }
            }
        } else {
            if (start > 0 && end > 0) {
                if (!GetAddressIndex((*it).first, (*it).second, UNEMP, addressIndex, start, end)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
                }
            } else {
                if (!GetAddressIndex((*it).first, (*it).second, UNEMP, addressIndex)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
                }
            }
        }
    }

    std::set<std::pair<int, std::string> > txids;
    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        int height = it->first.blockHeight;
        std::string txid = it->first.txhash.GetHex();

        if (addresses.size() > 1) {
            txids.insert(std::make_pair(height, txid));
        } else {
            if (txids.insert(std::make_pair(height, txid)).second) {
                result.push_back(txid);
            }
        }
    }

    if (addresses.size() > 1) {
        for (std::set<std::pair<int, std::string> >::const_iterator it=txids.begin(); it!=txids.end(); it++) {
            result.push_back(it->second);
        }
    }

    return result;

}

UniValue getspentinfo(const JSONRPCRequest& request)
{

    if (request.fHelp || request.params.size() != 1 || !request.params[0].isObject())
        throw std::runtime_error(
            "getspentinfo\n"
            "\nReturns the txid and index where an output is spent.\n"
            "\nArguments:\n"
            "{\n"
            "  \"txid\" (string) The hex string of the txid\n"
            "  \"index\" (number) The start block height\n"
            "}\n"
            "\nResult:\n"
            "{\n"
            "  \"txid\"  (string) The transaction id\n"
            "  \"index\"  (number) The spending input index\n"
            "  ,...\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getspentinfo", "'{\"txid\": \"0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9\", \"index\": 0}'")
            + HelpExampleRpc("getspentinfo", "{\"txid\": \"0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9\", \"index\": 0}")
        );

    UniValue txidValue = find_value(request.params[0].get_obj(), "txid");
    UniValue indexValue = find_value(request.params[0].get_obj(), "index");

    if (!txidValue.isStr() || !indexValue.isNum()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid txid or index");
    }

    uint256 txid = ParseHashV(txidValue, "txid");
    int outputIndex = indexValue.getInt<int>();

    CSpentIndexKey key(txid, outputIndex);
    CSpentIndexValue value;

    if (!GetSpentIndex(key, value)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unable to get spent info");
    }

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("txid", value.txid.GetHex());
    obj.pushKV("index", (int)value.inputIndex);
    obj.pushKV("height", value.blockHeight);

    return obj;
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
  //  --------------------- ------------------------  -----------------------  ----------
    { "control",            "getinfo",                &getinfo,                {} }, /* uses wallet if enabled */
    { "control",            "getmemoryinfo",          &getmemoryinfo,          {"mode"} },
    { "util",               "validateaddress",        &validateaddress,        {"address"} }, /* uses wallet if enabled */
    { "util",               "createmultisig",         &createmultisig,         {"nrequired","keys"} },
    { "util",               "verifymessage",          &verifymessage,          {"address","signature","message"} },
    { "util",               "signmessagewithprivkey", &signmessagewithprivkey, {"privkey","message"} },

    /* Address index */
    { "addressindex",       "getaddressmempool",      &getaddressmempool,      {"addresses","includeAssets"} },
    { "addressindex",       "getaddressutxos",        &getaddressutxos,        {"addresses"} },
    { "addressindex",       "getaddressdeltas",       &getaddressdeltas,       {"addresses"} },
    { "addressindex",       "getaddresstxids",        &getaddresstxids,        {"addresses","includeAssets"} },
    { "addressindex",       "getaddressbalance",      &getaddressbalance,      {"addresses","includeAssets"} },

    /* Blockchain */
    { "blockchain",         "getspentinfo",           &getspentinfo,           {} },

    /* Not shown in help */
    { "hidden",             "setmocktime",            &setmocktime,            {"timestamp"}},
    { "hidden",             "echo",                   &echo,                   {"arg0","arg1","arg2","arg3","arg4","arg5","arg6","arg7","arg8","arg9"}},
    { "hidden",             "echojson",               &echo,                   {"arg0","arg1","arg2","arg3","arg4","arg5","arg6","arg7","arg8","arg9"}},
    { "hidden",             "logging",                &logging,                {"include", "exclude"}},
};

void RegisterMiscRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
