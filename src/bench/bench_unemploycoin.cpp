// Copyright (c) 2015-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The UnemployCoin Core developers
// Copyright (C) 2024-2025 UnemployCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparamsbase.h>
#include <chainparams.h>
#include "bench.h"
#include "crypto/sha256.h"
#include "key.h"
#include "validation.h"
#include "util.h"
#include "random.h"

int
main(int argc, char **argv)
{
    SHA256AutoDetect();
    RandomInit();
    ECC_Start();
    SetupEnvironment();
    fPrintToDebugLog = false; // don't want to write to debug.log file

    SelectParams(CBaseChainParams::MAIN);
    benchmark::BenchRunner::RunAll();

    ECC_Stop();
}
