#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Copyright (c) 2017-2020 The UnemployCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test deprecation of RPC calls."""

from test_framework.test_framework import RavenTestFramework
from test_framework.util import assert_raises_rpc_error

class DeprecatedRpcTest(RavenTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [[], ["-deprecatedrpc=estimatefee"]]

    def run_test(self):
        self.log.info("estimatefee: Shows deprecated message")
        assert_raises_rpc_error(-32, 'estimatefee is deprecated', self.nodes[0].estimatefee, 1)

        self.log.info("Using -deprecatedrpc=estimatefee bypasses the error")
        self.nodes[1].estimatefee(1)

if __name__ == '__main__':
    DeprecatedRpcTest().main()
