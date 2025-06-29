// Copyright (c) 2015-2017 The Bitcoin Core developers
// Copyright (c) 2017-2019 The UnemployCoin Core developers
// Copyright (C) 2024-2025 UnemployCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench.h"
#include "prevector.h"

static void PrevectorDestructor(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (auto x = 0; x < 1000; ++x) {
            prevector<28, unsigned char> t0;
            prevector<28, unsigned char> t1;
            t0.resize(28);
            t1.resize(29);
        }
    }
}

static void PrevectorClear(benchmark::State& state)
{

    while (state.KeepRunning()) {
        for (auto x = 0; x < 1000; ++x) {
            prevector<28, unsigned char> t0;
            prevector<28, unsigned char> t1;
            t0.resize(28);
            t0.clear();
            t1.resize(29);
            t0.clear();
        }
    }
}

BENCHMARK(PrevectorDestructor);
BENCHMARK(PrevectorClear);
