// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Copyright (c) 2017-2020 The UnemployCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet/init.h"

#include "net.h"
#include "util.h"
#include "utilmoneystr.h"
#include "validation.h"
#include "wallet/wallet.h"
#include "wallet/rpcwallet.h"

std::string GetWalletHelpString(bool showDebug)
{
    std::string strUsage = HelpMessageGroup(_("Wallet options:"));
    strUsage += HelpMessageOpt("-bip44=<n>", strprintf(_("Sets the wallet to use/not use bip44 12-words, non-bip44=0 or bip44=1 (default: 1). "
                                                                 "Note: By default 12-words will automatically be generated for you (random word selection). See -mnemonic and -mnemonicpassphrase "
                                                                 "below to create a wallet using a specific word list (use an existing bip-44 wallet word-list), or use the RPC/CLI getmywords or "
                                                                 "dumpwallet to retrieve the auto-generated word-list. This flag is ignored if there is already an existing non-bip44 wallet.")));
    strUsage += HelpMessageOpt("-discardfee=<amt>", strprintf(_("The fee rate (in %s/kB) that indicates your tolerance for discarding change by adding it to the fee (default: %s). "
                                                                            "Note: An output is discarded if it is dust at this rate, but we will always discard up to the dust relay fee and a discard "
                                                                            "fee above that is limited by the fee estimate for the longest target"),
                                                                            CURRENCY_UNIT, FormatMoney(DEFAULT_DISCARD_FEE)));
    strUsage += HelpMessageOpt("-disablewallet", _("Do not load the wallet and disable wallet RPC calls"));
    strUsage += HelpMessageOpt("-fallbackfee=<amt>", strprintf(_("A fee rate (in %s/kB) that will be used when fee estimation has insufficient data (default: %s)"), CURRENCY_UNIT, FormatMoney(DEFAULT_FALLBACK_FEE)));
    strUsage += HelpMessageOpt("-keypool=<n>", strprintf(_("Set key pool size to <n> (default: %u)"), DEFAULT_KEYPOOL_SIZE));
    strUsage += HelpMessageOpt("-mintxfee=<amt>", strprintf(_("Fees (in %s/kB) smaller than this are considered zero fee for transaction creation (default: %s)"), CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MINFEE)));
    strUsage += HelpMessageOpt("-mnemonic=<word-list>", strprintf(_("A space separated list of 12-words used to import a bip44 wallet")));
    strUsage += HelpMessageOpt("-mnemonicpassphrase=<passphrase>", strprintf(_("Passphrase securing your 12-word mnemonic word-list")));
    strUsage += HelpMessageOpt("-paytxfee=<amt>", strprintf(_("Fee (in %s/kB) to add to transactions you send (default: %s)"), CURRENCY_UNIT, FormatMoney(payTxFee.GetFeePerK())));
    strUsage += HelpMessageOpt("-rescan", _("Rescan the block chain for missing wallet transactions on startup"));
    strUsage += HelpMessageOpt("-salvagewallet", _("Attempt to recover private keys from a corrupt wallet on startup"));
    strUsage += HelpMessageOpt("-spendzeroconfchange", strprintf(_("Spend unconfirmed change when sending transactions (default: %u)"), DEFAULT_SPEND_ZEROCONF_CHANGE));
    strUsage += HelpMessageOpt("-txconfirmtarget=<n>", strprintf(_("If paytxfee is not set, include enough fee so transactions begin confirmation on average within n blocks (default: %u)"), DEFAULT_TX_CONFIRM_TARGET));
    strUsage += HelpMessageOpt("-upgradewallet", _("Upgrade wallet to latest format on startup"));
    strUsage += HelpMessageOpt("-walletrbf", strprintf(_("Send transactions with full-RBF opt-in enabled (default: %u)"), DEFAULT_WALLET_RBF));
    strUsage += HelpMessageOpt("-wallet=<file>", _("Specify wallet file (within data directory)") + " " + strprintf(_("(default: %s)"), DEFAULT_WALLET_DAT));
    strUsage += HelpMessageOpt("-walletbroadcast", _("Make the wallet broadcast transactions") + " " + strprintf(_("(default: %u)"), DEFAULT_WALLETBROADCAST));
    strUsage += HelpMessageOpt("-walletnotify=<cmd>", _("Execute command when a wallet transaction changes (%s in cmd is replaced by TxID)"));
    strUsage += HelpMessageOpt("-zapwallettxes=<mode>", _("Delete all wallet transactions and only recover those parts of the blockchain through -rescan on startup") +
                               " " + _("(1 = keep tx meta data e.g. account owner and payment request information, 2 = drop tx meta data)"));
    strUsage += HelpMessageOpt("-miningaddress=<address>", _("When getblocktemplate is called. It will create the coinbase transaction using this address(default: empty string)"));

    if (showDebug)
    {
        strUsage += HelpMessageGroup(_("Wallet debugging/testing options:"));

        strUsage += HelpMessageOpt("-dblogsize=<n>", strprintf("Flush wallet database activity from memory to disk log every <n> megabytes (default: %u)", DEFAULT_WALLET_DBLOGSIZE));
        strUsage += HelpMessageOpt("-flushwallet", strprintf("Run a thread to flush wallet periodically (default: %u)", DEFAULT_FLUSHWALLET));
        strUsage += HelpMessageOpt("-privdb", strprintf("Sets the DB_PRIVATE flag in the wallet db environment (default: %u)", DEFAULT_WALLET_PRIVDB));
        strUsage += HelpMessageOpt("-walletrejectlongchains", strprintf(_("Wallet will not create transactions that violate mempool chain limits (default: %u)"), DEFAULT_WALLET_REJECT_LONG_CHAINS));
    }

    strUsage += HelpMessageOpt("-minrewardheight=<n>", _("The default height that is required before rewards are allowed to be sent out") + " " + strprintf(_("(default: %u)"), MINIMUM_REWARDS_PAYOUT_HEIGHT));


    return strUsage;
}

bool WalletParameterInteraction()
{
    gArgs.SoftSetArg("-wallet", DEFAULT_WALLET_DAT);
    const bool is_multiwallet = gArgs.GetArgs("-wallet").size() > 1;

    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET))
        return true;

    if (gArgs.GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY) && gArgs.SoftSetBoolArg("-walletbroadcast", false)) {
        LogPrintf("%s: parameter interaction: -blocksonly=1 -> setting -walletbroadcast=0\n", __func__);
    }

    if (gArgs.GetBoolArg("-salvagewallet", false)) {
        if (is_multiwallet) {
            return InitError(strprintf("%s is only allowed with a single wallet file", "-salvagewallet"));
        }
        // Rewrite just private keys: rescan to find transactions
        if (gArgs.SoftSetBoolArg("-rescan", true)) {
            LogPrintf("%s: parameter interaction: -salvagewallet=1 -> setting -rescan=1\n", __func__);
        }
    }

    int zapwallettxes = gArgs.GetArg("-zapwallettxes", 0);
    // -zapwallettxes implies dropping the mempool on startup
    if (zapwallettxes != 0 && gArgs.SoftSetBoolArg("-persistmempool", false)) {
        LogPrintf("%s: parameter interaction: -zapwallettxes=%s -> setting -persistmempool=0\n", __func__, zapwallettxes);
    }

    // -zapwallettxes implies a rescan
    if (zapwallettxes != 0) {
        if (is_multiwallet) {
            return InitError(strprintf("%s is only allowed with a single wallet file", "-zapwallettxes"));
        }
        if (gArgs.SoftSetBoolArg("-rescan", true)) {
            LogPrintf("%s: parameter interaction: -zapwallettxes=%s -> setting -rescan=1\n", __func__, zapwallettxes);
        }
    }

    if (is_multiwallet) {
        if (gArgs.GetBoolArg("-upgradewallet", false)) {
            return InitError(strprintf("%s is only allowed with a single wallet file", "-upgradewallet"));
        }
    }

    if (gArgs.GetBoolArg("-sysperms", false))
        return InitError("-sysperms is not allowed in combination with enabled wallet functionality");
    if (gArgs.GetArg("-prune", 0) && gArgs.GetBoolArg("-rescan", false))
        return InitError(_("Rescans are not possible in pruned mode. You will need to use -reindex which will download the whole blockchain again."));

    if (::minRelayTxFee.GetFeePerK() > HIGH_TX_FEE_PER_KB)
        InitWarning(AmountHighWarn("-minrelaytxfee") + " " +
                    _("The wallet will avoid paying less than the minimum relay fee."));

    if (gArgs.IsArgSet("-mintxfee"))
    {
        CAmount n = 0;
        if (!ParseMoney(gArgs.GetArg("-mintxfee", ""), n) || 0 == n)
            return InitError(AmountErrMsg("mintxfee", gArgs.GetArg("-mintxfee", "")));
        if (n > HIGH_TX_FEE_PER_KB)
            InitWarning(AmountHighWarn("-mintxfee") + " " +
                        _("This is the minimum transaction fee you pay on every transaction."));
        CWallet::minTxFee = CFeeRate(n);
    }
    if (gArgs.IsArgSet("-fallbackfee"))
    {
        CAmount nFeePerK = 0;
        if (!ParseMoney(gArgs.GetArg("-fallbackfee", ""), nFeePerK))
            return InitError(strprintf(_("Invalid amount for -fallbackfee=<amount>: '%s'"), gArgs.GetArg("-fallbackfee", "")));
        if (nFeePerK > HIGH_TX_FEE_PER_KB)
            InitWarning(AmountHighWarn("-fallbackfee") + " " +
                        _("This is the transaction fee you may pay when fee estimates are not available."));
        CWallet::fallbackFee = CFeeRate(nFeePerK);
    }
    if (gArgs.IsArgSet("-discardfee"))
    {
        CAmount nFeePerK = 0;
        if (!ParseMoney(gArgs.GetArg("-discardfee", ""), nFeePerK))
            return InitError(strprintf(_("Invalid amount for -discardfee=<amount>: '%s'"), gArgs.GetArg("-discardfee", "")));
        if (nFeePerK > HIGH_TX_FEE_PER_KB)
            InitWarning(AmountHighWarn("-discardfee") + " " +
                        _("This is the transaction fee you may discard if change is smaller than dust at this level"));
        CWallet::m_discard_rate = CFeeRate(nFeePerK);
    }
    if (gArgs.IsArgSet("-paytxfee"))
    {
        CAmount nFeePerK = 0;
        if (!ParseMoney(gArgs.GetArg("-paytxfee", ""), nFeePerK))
            return InitError(AmountErrMsg("paytxfee", gArgs.GetArg("-paytxfee", "")));
        if (nFeePerK > HIGH_TX_FEE_PER_KB)
            InitWarning(AmountHighWarn("-paytxfee") + " " +
                        _("This is the transaction fee you will pay if you send a transaction."));

        payTxFee = CFeeRate(nFeePerK, 1000);
        if (payTxFee < ::minRelayTxFee)
        {
            return InitError(strprintf(_("Invalid amount for -paytxfee=<amount>: '%s' (must be at least %s)"),
                                       gArgs.GetArg("-paytxfee", ""), ::minRelayTxFee.ToString()));
        }
    }
    if (gArgs.IsArgSet("-maxtxfee"))
    {
        CAmount nMaxFee = 0;
        if (!ParseMoney(gArgs.GetArg("-maxtxfee", ""), nMaxFee))
            return InitError(AmountErrMsg("maxtxfee", gArgs.GetArg("-maxtxfee", "")));
        if (nMaxFee > HIGH_MAX_TX_FEE)
            InitWarning(_("-maxtxfee is set very high! Fees this large could be paid on a single transaction."));
        maxTxFee = nMaxFee;
        if (CFeeRate(maxTxFee, 1000) < ::minRelayTxFee)
        {
            return InitError(strprintf(_("Invalid amount for -maxtxfee=<amount>: '%s' (must be at least the minrelay fee of %s to prevent stuck transactions)"),
                                       gArgs.GetArg("-maxtxfee", ""), ::minRelayTxFee.ToString()));
        }
    }
    nTxConfirmTarget = gArgs.GetArg("-txconfirmtarget", DEFAULT_TX_CONFIRM_TARGET);
    bSpendZeroConfChange = gArgs.GetBoolArg("-spendzeroconfchange", DEFAULT_SPEND_ZEROCONF_CHANGE);
    fWalletRbf = gArgs.GetBoolArg("-walletrbf", DEFAULT_WALLET_RBF);

    return true;
}

void RegisterWalletRPC(CRPCTable &t)
{
    if (gArgs.GetBoolArg("-disablewallet", false)) return;

    RegisterWalletRPCCommands(t);
}

bool VerifyWallets()
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET))
        return true;

    uiInterface.InitMessage(_("Verifying wallet(s)..."));

    // Keep track of each wallet absolute path to detect duplicates.
    std::set<fs::path> wallet_paths;

    for (const std::string& walletFile : gArgs.GetArgs("-wallet")) {
        if (boost::filesystem::path(walletFile).filename() != walletFile) {
            return InitError(strprintf(_("Error loading wallet %s. -wallet parameter must only specify a filename (not a path)."), walletFile));
        }

        if (SanitizeString(walletFile, SAFE_CHARS_FILENAME) != walletFile) {
            return InitError(strprintf(_("Error loading wallet %s. Invalid characters in -wallet filename."), walletFile));
        }

        fs::path wallet_path = fs::absolute(walletFile, GetDataDir());

        if (fs::exists(wallet_path) && (!fs::is_regular_file(wallet_path) || fs::is_symlink(wallet_path))) {
            return InitError(strprintf(_("Error loading wallet %s. -wallet filename must be a regular file."), walletFile));
        }

        if (!wallet_paths.insert(wallet_path).second) {
            return InitError(strprintf(_("Error loading wallet %s. Duplicate -wallet filename specified."), walletFile));
        }

        std::string strError;
        if (!CWalletDB::VerifyEnvironment(walletFile, GetDataDir().string(), strError)) {
            return InitError(strError);
        }

        if (gArgs.GetBoolArg("-salvagewallet", false)) {
            // Recover readable keypairs:
            CWallet dummyWallet;
            std::string backup_filename;
            if (!CWalletDB::Recover(walletFile, (void *)&dummyWallet, CWalletDB::RecoverKeysOnlyFilter, backup_filename)) {
                return false;
            }
        }

        std::string strWarning;
        bool dbV = CWalletDB::VerifyDatabaseFile(walletFile, GetDataDir().string(), strWarning, strError);
        if (!strWarning.empty()) {
            InitWarning(strWarning);
        }
        if (!dbV) {
            InitError(strError);
            return false;
        }
    }

    return true;
}

bool OpenWallets()
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        LogPrintf("Wallet disabled!\n");
        return true;
    }

    for (const std::string& walletFile : gArgs.GetArgs("-wallet")) {
        CWallet * const pwallet = CWallet::CreateWalletFromFile(walletFile);
        if (!pwallet) {
            return false;
        }
        vpwallets.push_back(pwallet);
    }

    return true;
}

void StartWallets(CScheduler& scheduler) {
    for (CWalletRef pwallet : vpwallets) {
        pwallet->postInitProcess(scheduler);
    }
}

void FlushWallets() {
    for (CWalletRef pwallet : vpwallets) {
        pwallet->Flush(false);
    }
}

void StopWallets() {
    for (CWalletRef pwallet : vpwallets) {
        pwallet->Flush(true);
    }
}

void CloseWallets() {
    for (CWalletRef pwallet : vpwallets) {
        delete pwallet;
    }
    vpwallets.clear();
}
