#!/usr/bin/env python3
# Copyright (c) 2014-2017 The Bitcoin Core developers
# Copyright (c) 2017-2020 The UnemployCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Test mempool persistence.

By default, unemploycoind will dump mempool on shutdown and
then reload it on startup. This can be overridden with
the -persistmempool=0 command line option.

Test is as follows:

  - start node0, node1 and node2. node1 has -persistmempool=0
  - create 5 transactions on node2 to its own address. Note that these
    are not sent to node0 or node1 addresses because we don't want
    them to be saved in the wallet.
  - check that node0 and node1 have 5 transactions in their mempools
  - shutdown all nodes.
  - startup node0. Verify that it still has 5 transactions
    in its mempool. Shutdown node0. This tests that by default the
    mempool is persistent.
  - startup node1. Verify that its mempool is empty. Shutdown node1.
    This tests that with -persistmempool=0, the mempool is not
    dumped to disk when the node is shut down.
  - Restart node0 with -persistmempool=0. Verify that its mempool is
    empty. Shutdown node0. This tests that with -persistmempool=0,
    the mempool is not loaded from disk on start up.
  - Restart node0 with -persistmempool. Verify that it has 5
    transactions in its mempool. This tests that -persistmempool=0
    does not overwrite a previously valid mempool stored on disk.
  - Remove node0 mempool.dat and verify savemempool RPC recreates it
    and verify that node1 can load it and has 5 transaction in its
    mempool.
  - Verify that savemempool throws when the RPC is called if
    node1 can't write to disk.
"""

import os
import time
from test_framework.test_framework import RavenTestFramework
from test_framework.util import assert_equal, Decimal, wait_until, assert_raises_rpc_error

class MempoolPersistTest(RavenTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [[], ["-persistmempool=0"], []]

    def run_test(self):
        chain_height = self.nodes[0].getblockcount()
        assert_equal(chain_height, 200)

        self.log.debug("Mine a single block to get out of IBD")
        self.nodes[0].generate(1)
        self.sync_all()

        self.log.debug("Send 5 transactions from node2 (to its own address)")
        for _ in range(5):
            self.nodes[2].sendtoaddress(self.nodes[2].getnewaddress(), Decimal("10"))
        self.sync_all()

        self.log.debug("Verify that node0 and node1 have 5 transactions in their mempools")
        assert_equal(len(self.nodes[0].getrawmempool()), 5)
        assert_equal(len(self.nodes[1].getrawmempool()), 5)

        self.log.debug("Stop-start node0 and node1. Verify that node0 has the transactions in its mempool and node1 does not.")
        self.stop_nodes()
        self.start_node(0)
        self.start_node(1)
        # Give unemploycoind a second to reload the mempool
        time.sleep(1)
        wait_until(lambda: len(self.nodes[0].getrawmempool()) == 5, err_msg="Wait for getRawMempool")
        assert_equal(len(self.nodes[1].getrawmempool()), 0)

        self.log.debug("Stop-start node0 with -persistmempool=0. Verify that it doesn't load its mempool.dat file.")
        self.stop_nodes()
        self.start_node(0, extra_args=["-persistmempool=0"])
        # Give unemploycoind a second to reload the mempool
        time.sleep(1)
        assert_equal(len(self.nodes[0].getrawmempool()), 0)

        self.log.debug("Stop-start node0. Verify that it has the transactions in its mempool.")
        self.stop_nodes()
        self.start_node(0)
        wait_until(lambda: len(self.nodes[0].getrawmempool()) == 5, err_msg="Wait for getRawMempool")

        mempooldat0 = os.path.join(self.options.tmpdir, 'node0', 'regtest', 'mempool.dat')
        mempooldat1 = os.path.join(self.options.tmpdir, 'node1', 'regtest', 'mempool.dat')
        self.log.debug("Remove the mempool.dat file. Verify that savemempool to disk via RPC re-creates it")
        os.remove(mempooldat0)
        self.nodes[0].savemempool()
        assert os.path.isfile(mempooldat0)

        self.log.debug("Stop nodes, make node1 use mempool.dat from node0. Verify it has 5 transactions")
        os.rename(mempooldat0, mempooldat1)
        self.stop_nodes()
        self.start_node(1, extra_args=[])
        wait_until(lambda: len(self.nodes[1].getrawmempool()) == 5, err_msg="Wait for getRawMempool")

        self.log.debug("Prevent unemploycoind from writing mempool.dat to disk. Verify that `savemempool` fails")
        # to test the exception we are setting bad permissions on a tmp file called mempool.dat.new
        # which is an implementation detail that could change and break this test
        mempooldotnew1 = mempooldat1 + '.new'
        with os.fdopen(os.open(mempooldotnew1, os.O_CREAT, 0o000), 'w'):
            pass
        assert_raises_rpc_error(-1, "Unable to dump mempool to disk", self.nodes[1].savemempool)
        os.remove(mempooldotnew1)

if __name__ == '__main__':
    MempoolPersistTest().main()
