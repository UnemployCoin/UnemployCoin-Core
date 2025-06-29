#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Copyright (c) 2017-2020 The UnemployCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Test behavior of -maxuploadtarget.

* Verify that getdata requests for old blocks (>1week) are dropped
if uploadtarget has been reached.
* Verify that getdata requests for recent blocks are respecteved even
if uploadtarget has been reached.
* Verify that the upload counters are reset after 24 hours.
"""

from collections import defaultdict
import time
from test_framework.mininode import NodeConn, NodeConnCB, NetworkThread, MsgGetdata, CInv
from test_framework.test_framework import RavenTestFramework
from test_framework.util import p2p_port, mine_large_block, assert_equal


class TestNode(NodeConnCB):
    def __init__(self):
        super().__init__()
        self.block_receive_map = defaultdict(int)

    def on_inv(self, conn, message):
        pass

    def on_block(self, conn, message):
        message.block.calc_x16r()
        self.block_receive_map[message.block.x16r] += 1


class MaxUploadTest(RavenTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.maxuploadtarget = 11200
        self.extra_args = [["-maxuploadtarget=%s" % self.maxuploadtarget, "-blockmaxsize=999000"]]

        # Cache for utxos, as the listunspent may take a long time later in the test
        self.utxo_cache = []

    def run_test(self):
        # Before we connect anything, we first set the time on the node
        # to be in the past, otherwise things break because the CNode
        # time counters can't be reset backward after initialization
        old_time = int(time.time() - 2 * 60 * 60 * 24 * 7)
        self.nodes[0].setmocktime(old_time)

        # Generate some old blocks
        self.nodes[0].generate(130)

        # test_nodes[0] will only request old blocks
        # test_nodes[1] will only request new blocks
        # test_nodes[2] will test resetting the counters
        test_nodes = []
        connections = []

        for i in range(3):
            test_nodes.append(TestNode())
            connections.append(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], test_nodes[i]))
            test_nodes[i].add_connection(connections[i])

        NetworkThread().start()  # Start up network handling in another thread
        [x.wait_for_verack() for x in test_nodes]

        # Test logic begins here

        # Now mine a big block
        mine_large_block(self.nodes[0], self.utxo_cache)

        # Store the hash; we'll request this later
        big_old_block = self.nodes[0].getbestblockhash()
        old_block_size = self.nodes[0].getblock(big_old_block, True)['size']
        big_old_block = int(big_old_block, 16)

        # Advance to two days ago
        self.nodes[0].setmocktime(int(time.time()) - 2 * 60 * 60 * 24)

        # Mine one more block, so that the prior block looks old
        mine_large_block(self.nodes[0], self.utxo_cache)

        # We'll be requesting this new block too
        big_new_block = self.nodes[0].getbestblockhash()
        big_new_block = int(big_new_block, 16)

        # test_nodes[0] will test what happens if we just keep requesting the
        # the same big old block too many times (expect: disconnect)

        getdata_request = MsgGetdata()
        getdata_request.inv.append(CInv(2, big_old_block))

        block_rate_minutes = 1
        blocks_per_day = 24 * 60 / block_rate_minutes
        max_block_serialized_size = 8000000  # This is MAX_BLOCK_SERIALIZED_SIZE_RIP2
        max_bytes_per_day = self.maxuploadtarget * 1024 * 1024
        daily_buffer = blocks_per_day * max_block_serialized_size
        max_bytes_available = max_bytes_per_day - daily_buffer
        success_count = max_bytes_available // old_block_size

        # 224051200B will be reserved for relaying new blocks, so expect this to
        # succeed for ~236 tries.
        for i in range(int(success_count)):
            test_nodes[0].send_message(getdata_request)
            test_nodes[0].sync_with_ping()
            assert_equal(test_nodes[0].block_receive_map[big_old_block], i + 1)

        assert_equal(len(self.nodes[0].getpeerinfo()), 3)
        # At most a couple more tries should succeed (depending on how long 
        # the test has been running so far).
        for i in range(3):
            test_nodes[0].send_message(getdata_request)
        test_nodes[0].wait_for_disconnect()
        assert_equal(len(self.nodes[0].getpeerinfo()), 2)
        self.log.info("Peer 0 disconnected after downloading old block too many times")

        # Requesting the current block on test_nodes[1] should succeed indefinitely,
        # even when over the max upload target.
        # We'll try lots of times
        getdata_request.inv = [CInv(2, big_new_block)]
        for i in range(500):
            test_nodes[1].send_message(getdata_request)
            test_nodes[1].sync_with_ping()
            assert_equal(test_nodes[1].block_receive_map[big_new_block], i + 1)

        self.log.info("Peer 1 able to repeatedly download new block")

        # But if test_nodes[1] tries for an old block, it gets disconnected too.
        getdata_request.inv = [CInv(2, big_old_block)]
        test_nodes[1].send_message(getdata_request)
        test_nodes[1].wait_for_disconnect()
        assert_equal(len(self.nodes[0].getpeerinfo()), 1)

        self.log.info("Peer 1 disconnected after trying to download old block")

        self.log.info("Advancing system time on node to clear counters...")

        # If we advance the time by 24 hours, then the counters should reset,
        # and test_nodes[2] should be able to retrieve the old block.
        self.nodes[0].setmocktime(int(time.time()))
        test_nodes[2].sync_with_ping()
        test_nodes[2].send_message(getdata_request)
        test_nodes[2].sync_with_ping()
        assert_equal(test_nodes[2].block_receive_map[big_old_block], 1)

        self.log.info("Peer 2 able to download old block")

        [c.disconnect_node() for c in connections]

        # stop and start node 0 with 1MB maxuploadtarget, whitelist 127.0.0.1
        self.log.info("Restarting nodes with -whitelist=127.0.0.1")
        self.stop_node(0)
        self.start_node(0, ["-whitelist=127.0.0.1", "-maxuploadtarget=1", "-blockmaxsize=999000"])

        # recreate/reconnect a test node
        test_nodes = [TestNode()]
        connections = [NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], test_nodes[0])]
        test_nodes[0].add_connection(connections[0])

        NetworkThread().start()  # Start up network handling in another thread
        test_nodes[0].wait_for_verack()

        # retrieve 20 blocks which should be enough to break the 1MB limit
        getdata_request.inv = [CInv(2, big_new_block)]
        for i in range(20):
            test_nodes[0].send_message(getdata_request)
            test_nodes[0].sync_with_ping()
            assert_equal(test_nodes[0].block_receive_map[big_new_block], i + 1)

        getdata_request.inv = [CInv(2, big_old_block)]
        test_nodes[0].send_and_ping(getdata_request)
        assert_equal(len(self.nodes[0].getpeerinfo()), 1)  # node is still connected because of the whitelist

        self.log.info("Peer still connected after trying to download old block (whitelisted)")


if __name__ == '__main__':
    MaxUploadTest().main()
