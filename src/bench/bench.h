// Copyright (c) 2015-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The UnemployCoin Core developers
// Copyright (C) 2024-2025 UnemployCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UNEMPLOYCOIN_BENCH_BENCH_H
#define UNEMPLOYCOIN_BENCH_BENCH_H

#include <functional>
#include <limits>
#include <map>
#include <string>

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <cstdint>


// Simple micro-benchmarking framework; API mostly matches a subset of the Google Benchmark
// framework (see https://github.com/google/benchmark)
// Why not use the Google Benchmark framework? Because adding Yet Another Dependency
// (that uses cmake as its build system and has lots of features we don't need) isn't
// worth it.

/*
 * Usage:

static void CODE_TO_TIME(benchmark::State& state)
{
    ... do any setup needed...
    while (state.KeepRunning()) {
       ... do stuff you want to time...
    }
    ... do any cleanup needed...
}

BENCHMARK(CODE_TO_TIME);

 */
 
namespace benchmark {

    class State {
        std::string name;
        double maxElapsed;
        double beginTime;
        double lastTime, minTime, maxTime;
        uint64_t count;
        uint64_t countMask;
        uint64_t beginCycles;
        uint64_t lastCycles;
        uint64_t minCycles;
        uint64_t maxCycles;
    public:
        State(std::string _name, double _maxElapsed) : name(_name), maxElapsed(_maxElapsed), count(0) {
            minTime = std::numeric_limits<double>::max();
            maxTime = std::numeric_limits<double>::min();
            minCycles = std::numeric_limits<uint64_t>::max();
            maxCycles = std::numeric_limits<uint64_t>::min();
            countMask = 1;
        }
        bool KeepRunning();
    };

    typedef std::function<void(State&)> BenchFunction;

    class BenchRunner
    {
        typedef std::map<std::string, BenchFunction> BenchmarkMap;
        static BenchmarkMap &benchmarks();

    public:
        BenchRunner(std::string name, BenchFunction func);

        static void RunAll(double elapsedTimeForOne=1.0);
    };
}

// BENCHMARK(foo) expands to:  benchmark::BenchRunner bench_11foo("foo", foo);
#define BENCHMARK(n) \
    benchmark::BenchRunner BOOST_PP_CAT(bench_, BOOST_PP_CAT(__LINE__, n))(BOOST_PP_STRINGIZE(n), n);

#endif // UNEMPLOYCOIN_BENCH_BENCH_H
