// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2021 The UnemployCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/client.h"
#include "rpc/protocol.h"
#include "util.h"

#include <set>
#include <stdint.h>

#include <univalue.h>

class CRPCConvertParam
{
public:
    std::string methodName; //!< method whose params want conversion
    int paramIdx;           //!< 0-based idx of param to convert
    std::string paramName;  //!< parameter name
};

/**
 * Specify a (method, idx, name) here if the argument is a non-string RPC
 * argument and needs to be converted from JSON.
 *
 * @note Parameter indexes start from 0.
 */
static const CRPCConvertParam vRPCConvertParams[] =
{
    { "issue", 1, "qty" },
    { "issue", 4, "units" },
    { "issue", 5, "reissuable" },
    { "issue", 6, "has_ipfs" },
    { "issuerestrictedasset", 1, "qty" },
    { "issuerestrictedasset", 5, "units" },
    { "issuerestrictedasset", 6, "reissuable" },
    { "issuerestrictedasset", 7, "has_ipfs" },
    { "issuequalifierasset", 1,  "qty"},
    { "issuequalifierasset", 4, "has_ipfs" },
    { "reissuerestrictedasset", 1, "qty" },
    { "reissuerestrictedasset", 3, "change_verifier" },
    { "reissuerestrictedasset", 6, "new_unit" },
    { "reissuerestrictedasset", 7, "reissuable" },
    { "issueunique", 1, "asset_tags"},
    { "issueunique", 2, "ipfs_hashes"},
    { "transfer", 1, "qty"},
    { "transfer", 4, "expire_time"},
    { "transferfromaddress", 2, "qty"},
    { "transferfromaddress", 5, "expire_time"},
    { "transferfromaddresses", 1, "from_addresses"},
    { "transferfromaddresses", 2, "qty"},
    { "transferfromaddresses", 5, "expire_time"},
    { "transferqualifier", 1, "qty"},
    { "transferqualifier", 5, "expire_time"},
    { "reissue", 1, "qty"},
    { "reissue", 4, "reissuable"},
    { "reissue", 5, "new_unit"},
    { "listmyassets", 1, "verbose" },
    { "listmyassets", 2, "count" },
    { "listmyassets", 3, "start"},
    { "listmyassets", 4, "confs"},
    { "listassets", 1, "verbose" },
    { "listassets", 2, "count" },
    { "listassets", 3, "start" },
    { "setmocktime", 0, "timestamp" },
    { "generate", 0, "nblocks" },
    { "generate", 1, "maxtries" },
    { "setgenerate", 0, "generate" },
    { "setgenerate", 1, "genproclimit" },
    { "generatetoaddress", 0, "nblocks" },
    { "generatetoaddress", 2, "maxtries" },
    { "getnetworkhashps", 0, "nblocks" },
    { "getnetworkhashps", 1, "height" },
    { "sendtoaddress", 1, "amount" },
    { "sendtoaddress", 4, "subtractfeefromamount" },
//    { "sendtoaddress", 5 , "replaceable" },
    { "sendtoaddress", 5 , "conf_target" },
    { "sendfromaddress", 2, "amount" },
    { "sendfromaddress", 5, "subtractfeefromamount" },
    { "sendfromaddress", 6 , "conf_target" },
    { "settxfee", 0, "amount" },
    { "getreceivedbyaddress", 1, "minconf" },
    { "getreceivedbyaccount", 1, "minconf" },
    { "listreceivedbyaddress", 0, "minconf" },
    { "listreceivedbyaddress", 1, "include_empty" },
    { "listreceivedbyaddress", 2, "include_watchonly" },
    { "listreceivedbyaccount", 0, "minconf" },
    { "listreceivedbyaccount", 1, "include_empty" },
    { "listreceivedbyaccount", 2, "include_watchonly" },
    { "getbalance", 1, "minconf" },
    { "getbalance", 2, "include_watchonly" },
    { "getblockhash", 0, "height" },
    { "waitforblockheight", 0, "height" },
    { "waitforblockheight", 1, "timeout" },
    { "waitforblock", 1, "timeout" },
    { "waitfornewblock", 0, "timeout" },
    { "move", 2, "amount" },
    { "move", 3, "minconf" },
    { "sendfrom", 2, "amount" },
    { "sendfrom", 3, "minconf" },
    { "listtransactions", 1, "count" },
    { "listtransactions", 2, "skip" },
    { "listtransactions", 3, "include_watchonly" },
    { "listaccounts", 0, "minconf" },
    { "listaccounts", 1, "include_watchonly" },
    { "walletpassphrase", 1, "timeout" },
    { "getblocktemplate", 0, "template_request" },
    { "listsinceblock", 1, "target_confirmations" },
    { "listsinceblock", 2, "include_watchonly" },
    { "listsinceblock", 3, "include_removed" },
    { "sendmany", 1, "amounts" },
    { "sendmany", 2, "minconf" },
    { "sendmany", 4, "subtractfeefrom" },
//    { "sendmany", 5 , "replaceable" },
    { "sendmany", 5 , "conf_target" },
    { "addmultisigaddress", 0, "nrequired" },
    { "addmultisigaddress", 1, "keys" },
    { "createmultisig", 0, "nrequired" },
    { "createmultisig", 1, "keys" },
    { "listunspent", 0, "minconf" },
    { "listunspent", 1, "maxconf" },
    { "listunspent", 2, "addresses" },
    { "listunspent", 3, "include_unsafe" },
    { "listunspent", 4, "query_options" },
    { "getblock", 1, "verbosity" },
    { "getblock", 1, "verbose" },
    { "getblockheader", 1, "verbose" },
    { "getchaintxstats", 0, "nblocks" },
    { "gettransaction", 1, "include_watchonly" },
    { "getrawtransaction", 1, "verbose" },
    { "createrawtransaction", 0, "inputs" },
    { "createrawtransaction", 1, "outputs" },
    { "createrawtransaction", 2, "locktime" },
//    { "createrawtransaction", 3, "replaceable" },
    { "signrawtransaction", 1, "prevtxs" },
    { "signrawtransaction", 2, "privkeys" },
    { "sendrawtransaction", 1, "allowhighfees" },
    { "testmempoolaccept", 0, "rawtxs" },
    { "testmempoolaccept", 1, "allowhighfees" },
    { "combinerawtransaction", 0, "txs" },
    { "fundrawtransaction", 1, "options" },
    { "gettxout", 1, "n" },
    { "gettxout", 2, "include_mempool" },
    { "gettxoutproof", 0, "txids" },
    { "lockunspent", 0, "unlock" },
    { "lockunspent", 1, "transactions" },
    { "importprivkey", 2, "rescan" },
    { "importaddress", 2, "rescan" },
    { "importaddress", 3, "p2sh" },
    { "importpubkey", 2, "rescan" },
    { "importmulti", 0, "requests" },
    { "importmulti", 1, "options" },
    { "verifychain", 0, "checklevel" },
    { "verifychain", 1, "nblocks" },
    { "pruneblockchain", 0, "height" },
    { "keypoolrefill", 0, "newsize" },
    { "getrawmempool", 0, "verbose" },
    { "estimatefee", 0, "nblocks" },
    { "estimatesmartfee", 0, "conf_target" },
    { "estimaterawfee", 0, "conf_target" },
    { "estimaterawfee", 1, "threshold" },
    { "prioritisetransaction", 1, "dummy" },
    { "prioritisetransaction", 2, "fee_delta" },
    { "setban", 2, "bantime" },
    { "setban", 3, "absolute" },
    { "setnetworkactive", 0, "state" },
    { "getmempoolancestors", 1, "verbose" },
    { "getmempooldescendants", 1, "verbose" },
    { "getblockhashes", 0 , "high"},
    { "getblockhashes", 1, "low"},
    { "getblockhashes", 2, "options" },
    { "getspentinfo", 0, "txid_index"},
    { "getaddresstxids", 0, "addresses"},
    { "getaddresstxids", 1, "includeAssets"},
    { "getaddressbalance", 0, "addresses"},
    { "getaddressbalance", 1, "includeAssets"},
    { "getaddressdeltas", 0, "addresses"},
    { "getaddressutxos", 0, "addresses"},
    { "getaddressmempool", 0, "addresses"},
    { "getaddressmempool", 1, "includeAssets"},
    { "bumpfee", 1, "options" },
    { "logging", 0, "include" },
    { "logging", 1, "exclude" },
    { "disconnectnode", 1, "nodeid" },
    // Echo with conversion (For testing only)
    { "echojson", 0, "arg0" },
    { "echojson", 1, "arg1" },
    { "echojson", 2, "arg2" },
    { "echojson", 3, "arg3" },
    { "echojson", 4, "arg4" },
    { "echojson", 5, "arg5" },
    { "echojson", 6, "arg6" },
    { "echojson", 7, "arg7" },
    { "echojson", 8, "arg8" },
    { "echojson", 9, "arg9" },
    { "rescanblockchain", 0, "start_height"},
    { "rescanblockchain", 1, "stop_height"},
    { "listaddressesbyasset", 1, "totalonly"},
    { "listaddressesbyasset", 2, "count"},
    { "listaddressesbyasset", 3, "start"},
    { "listassetbalancesbyaddress", 1, "totalonly"},
    { "listassetbalancesbyaddress", 2, "count"},
    { "listassetbalancesbyaddress", 3, "start"},
    { "sendmessage", 2, "expire_time"},
    { "requestsnapshot", 1, "block_height"},
    { "getsnapshotrequest", 1, "block_height"},
    { "listsnapshotrequests", 1, "block_height"},
    { "cancelsnapshotrequest", 1, "block_height"},
    { "distributereward", 1, "snapshot_height"},
    { "distributereward", 3, "gross_distribution_amount"},
    { "getdistributestatus", 1, "snapshot_height"},
    { "getdistributestatus", 3, "gross_distribution_amount"},
    { "getsnapshot", 1, "block_height"},
    { "purgesnapshot", 1, "block_height"},
    { "stop", 0, "wait"},
    { "getkawpowhash", 3, "height"},
};

class CRPCConvertTable
{
private:
    std::set<std::pair<std::string, int>> members;
    std::set<std::pair<std::string, std::string>> membersByName;

public:
    CRPCConvertTable();

    bool convert(const std::string& method, int idx) {
        return (members.count(std::make_pair(method, idx)) > 0);
    }
    bool convert(const std::string& method, const std::string& name) {
        return (membersByName.count(std::make_pair(method, name)) > 0);
    }
};

CRPCConvertTable::CRPCConvertTable()
{
    const unsigned int n_elem =
        (sizeof(vRPCConvertParams) / sizeof(vRPCConvertParams[0]));

    for (unsigned int i = 0; i < n_elem; i++) {
        members.insert(std::make_pair(vRPCConvertParams[i].methodName,
                                      vRPCConvertParams[i].paramIdx));
        membersByName.insert(std::make_pair(vRPCConvertParams[i].methodName,
                                            vRPCConvertParams[i].paramName));
    }
}

static CRPCConvertTable rpcCvtTable;

/** Non-RFC4627 JSON parser, accepts internal values (such as numbers, true, false, null)
 * as well as objects and arrays.
 */
UniValue ParseNonRFCJSONValue(const std::string& strVal)
{
    UniValue jVal;
    if (!jVal.read(std::string("[")+strVal+std::string("]")) ||
        !jVal.isArray() || jVal.size()!=1)
        throw std::runtime_error(std::string("Error parsing JSON:")+strVal);
    return jVal[0];
}

UniValue RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams)
{
    UniValue params(UniValue::VARR);

    for (unsigned int idx = 0; idx < strParams.size(); idx++) {
        const std::string& strVal = strParams[idx];

        if (!rpcCvtTable.convert(strMethod, idx)) {
            // insert string value directly
            params.push_back(strVal);
        } else {
            // parse string as JSON, insert bool/number/object/etc. value
            params.push_back(ParseNonRFCJSONValue(strVal));
        }
    }

    return params;
}

UniValue RPCConvertNamedValues(const std::string &strMethod, const std::vector<std::string> &strParams)
{
    UniValue params(UniValue::VOBJ);

    for (const std::string &s: strParams) {
        size_t pos = s.find("=");
        if (pos == std::string::npos) {
            throw(std::runtime_error("No '=' in named argument '"+s+"', this needs to be present for every argument (even if it is empty)"));
        }

        std::string name = s.substr(0, pos);
        std::string value = s.substr(pos+1);

        if (!rpcCvtTable.convert(strMethod, name)) {
            // insert string value directly
            params.pushKV(name, value);
        } else {
            // parse string as JSON, insert bool/number/object/etc. value
            params.pushKV(name, ParseNonRFCJSONValue(value));
        }
    }

    return params;
}
