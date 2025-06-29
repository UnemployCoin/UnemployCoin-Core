// Copyright (c) 2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The UnemployCoin Core developers
// Copyright (C) 2024-2025 UnemployCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench.h"

#include "chainparams.h"
#include "validation.h"
#include "streams.h"
#include "consensus/validation.h"

namespace block_bench {
#include "bench/data/block566553.raw.h"

} // namespace block_bench

// These are the two major time-sinks which happen after we have fully received
// a block off the wire, but before we can relay the block on to peers using
// compact block relay.

static void DeserializeBlockTest(benchmark::State& state)
{
    CDataStream stream((const char*)block_bench::block566553,
            (const char*)&block_bench::block566553[sizeof(block_bench::block566553)],
            SER_NETWORK, PROTOCOL_VERSION);
    char a = '\0';
    stream.write(&a, 1); // Prevent compaction

    while (state.KeepRunning()) {
        CBlock block;
        stream >> block;
        assert(stream.Rewind(sizeof(block_bench::block566553)));
    }
}

static void DeserializeAndCheckBlockTest(benchmark::State& state)
{
    CDataStream stream((const char*)block_bench::block566553,
            (const char*)&block_bench::block566553[sizeof(block_bench::block566553)],
            SER_NETWORK, PROTOCOL_VERSION);
    char a = '\0';
    stream.write(&a, 1); // Prevent compaction

    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);

    while (state.KeepRunning()) {
        CBlock block; // Note that CBlock caches its checked state, so we need to recreate it here
        stream >> block;
        assert(stream.Rewind(sizeof(block_bench::block566553)));

        CValidationState validationState;
        assert(CheckBlock(block, validationState, chainParams->GetConsensus()));
    }
}

BENCHMARK(DeserializeBlockTest);
BENCHMARK(DeserializeAndCheckBlockTest);
