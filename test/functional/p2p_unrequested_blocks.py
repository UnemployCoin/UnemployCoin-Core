#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Copyright (c) 2017-2020 The UnemployCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Test processing of unrequested blocks.

Since behavior differs when receiving unrequested blocks from whitelisted peers
versus non-whitelisted peers, this tests the behavior of both (effectively two
separate tests running in parallel).

Setup: three nodes, node0+node1+node2, not connected to each other.  Node0 does not
whitelist localhost, but node1 does. They will each be on their own chain for
this test.  Node2 will have nMinimumChainWork set to 0x10, so it won't process
low-work unrequested blocks.

We have one NodeConn connection to each, test_node, white_node, and min_work_node,
respectively.

The test:
1. Generate one block on each node, to leave IBD.

2. Mine a new block on each tip, and deliver to each node from node's peer.
   The tip should advance for node0 and node1, but node2 should skip processing
   due to nMinimumChainWork.

Node2 is unused in tests 3-7:

3. Mine a block that forks the previous block, and deliver to each node from
   corresponding peer.
   Node0 should not process this block (just accept the header), because it is
   unrequested and doesn't have more work than the tip.
   Node1 should process because this is coming from a whitelisted peer.

4. Send another block that builds on the forking block.
   Node0 should process this block but be stuck on the shorter chain, because
   it's missing an intermediate block.
   Node1 should reorg to this longer chain.

4b.Send 288 more blocks on the longer chain.
   Node0 should process all but the last block (too far ahead in height).
   Send all headers to Node1, and then send the last block in that chain.
   Node1 should accept the block because it's coming from a whitelisted peer.

5. Send a duplicate of the block in #3 to Node0.
   Node0 should not process the block because it is unrequested, and stay on
   the shorter chain.

6. Send Node0 an inv for the height 3 block produced in #4 above.
   Node0 should figure out that Node0 has the missing height 2 block and send a
   getdata.

7. Send Node0 the missing block again.
   Node0 should process and the tip should advance.

8. Test Node2 is able to sync when connected to node0 (which should have sufficient
work on its chain).

"""

import time
from test_framework.mininode import NodeConn, NodeConnCB, NetworkThread, MsgBlock, MsgHeaders, CBlockHeader, mininode_lock, MsgInv, CInv
from test_framework.test_framework import RavenTestFramework
from test_framework.util import os, p2p_port, assert_equal, assert_raises_rpc_error, connect_nodes, sync_blocks
from test_framework.blocktools import create_block, create_coinbase

class AcceptBlockTest(RavenTestFramework):
    def add_options(self, parser):
        parser.add_option("--testbinary", dest="testbinary",
                          default=os.getenv("UNEMPLOYCOIND", "unemploycoind"),
                          help="unemploycoind binary to test")

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [[], ["-whitelist=127.0.0.1"], ["-minimumchainwork=0x10"]]

    def setup_network(self):
        # Node0 will be used to test behavior of processing unrequested blocks
        # from peers which are not whitelisted, while Node1 will be used for
        # the whitelisted case.
        # Node2 will be used for non-whitelisted peers to test the interaction
        # with nMinimumChainWork.
        self.setup_nodes()

    def run_test(self):
        # Setup the p2p connections and start up the network thread.
        test_node = NodeConnCB()   # connects to node0 (not whitelisted)
        white_node = NodeConnCB()  # connects to node1 (whitelisted)
        min_work_node = NodeConnCB()  # connects to node2 (not whitelisted)

        connections = [NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], test_node),
                       NodeConn('127.0.0.1', p2p_port(1), self.nodes[1], white_node),
                       NodeConn('127.0.0.1', p2p_port(2), self.nodes[2], min_work_node)]
        test_node.add_connection(connections[0])
        white_node.add_connection(connections[1])
        min_work_node.add_connection(connections[2])

        NetworkThread().start() # Start up network handling in another thread

        # Test logic begins here
        test_node.wait_for_verack()
        white_node.wait_for_verack()
        min_work_node.wait_for_verack()

        # 1. Have nodes mine a block (nodes1/2 leave IBD)
        [ n.generate(1) for n in self.nodes ]
        tips = [ int("0x" + n.getbestblockhash(), 0) for n in self.nodes ]

        # 2. Send one block that builds on each tip.
        # This should be accepted by nodes 1/2
        blocks_h2 = []  # the height 2 blocks on each node's chain
        block_time = int(time.time()) + 1
        for i in range(3):
            blocks_h2.append(create_block(tips[i], create_coinbase(2), block_time))
            blocks_h2[i].solve()
            block_time += 1
        test_node.send_message(MsgBlock(blocks_h2[0]))
        white_node.send_message(MsgBlock(blocks_h2[1]))
        min_work_node.send_message(MsgBlock(blocks_h2[2]))

        for x in [test_node, white_node, min_work_node]:
            x.sync_with_ping()
        assert_equal(self.nodes[0].getblockcount(), 2)
        assert_equal(self.nodes[1].getblockcount(), 2)
        assert_equal(self.nodes[2].getblockcount(), 1)
        self.log.info("First height 2 block accepted by node0/node1; correctly rejected by node2")

        # 3. Send another block that builds on the original tip.
        blocks_h2f = []  # Blocks at height 2 that fork off the main chain
        for i in range(2):
            blocks_h2f.append(create_block(tips[i], create_coinbase(2), blocks_h2[i].nTime+1))
            blocks_h2f[i].solve()
        test_node.send_message(MsgBlock(blocks_h2f[0]))
        white_node.send_message(MsgBlock(blocks_h2f[1]))

        for x in [test_node, white_node]:
            x.sync_with_ping()
        for x in self.nodes[0].getchaintips():
            if x['hash'] == blocks_h2f[0].hash:
                assert_equal(x['status'], "headers-only")

        for x in self.nodes[1].getchaintips():
            if x['hash'] == blocks_h2f[1].hash:
                assert_equal(x['status'], "valid-headers")

        self.log.info("Second height 2 block accepted only from whitelisted peer")

        # 4. Now send another block that builds on the forking chain.
        blocks_h3 = []
        for i in range(2):
            blocks_h3.append(create_block(blocks_h2f[i].sha256, create_coinbase(3), blocks_h2f[i].nTime+1))
            blocks_h3[i].solve()
        test_node.send_message(MsgBlock(blocks_h3[0]))
        white_node.send_message(MsgBlock(blocks_h3[1]))

        for x in [test_node, white_node]:
            x.sync_with_ping()
        # Since the earlier block was not processed by node0, the new block
        # can't be fully validated.
        for x in self.nodes[0].getchaintips():
            if x['hash'] == blocks_h3[0].hash:
                assert_equal(x['status'], "headers-only")

        # But this block should be accepted by node0 since it has more work.
        self.nodes[0].getblock(blocks_h3[0].hash)
        self.log.info("Unrequested more-work block accepted from non-whitelisted peer")

        # Node1 should have accepted and reorged.
        assert_equal(self.nodes[1].getblockcount(), 3)
        self.log.info("Successfully reorged to length 3 chain from whitelisted peer")

        # 4b. Now mine 288 more blocks and deliver; all should be processed but
        # the last (height-too-high) on node0.  Node1 should process the tip if
        # we give it the headers chain leading to the tip.
        tips = blocks_h3
        headers_message = MsgHeaders()
        all_blocks = []   # node0's blocks
        for j in range(2):
            for i in range(288):
                next_block = create_block(tips[j].sha256, create_coinbase(i + 4), tips[j].nTime+1)
                next_block.solve()
                if j==0:
                    test_node.send_message(MsgBlock(next_block))
                    all_blocks.append(next_block)
                else:
                    headers_message.headers.append(CBlockHeader(next_block))
                tips[j] = next_block

        time.sleep(2)
        # Blocks 1-287 should be accepted, block 288 should be ignored because it's too far ahead
        for x in all_blocks[:-1]:
            self.nodes[0].getblock(x.hash)
        assert_raises_rpc_error(-1, "Block not found on disk", self.nodes[0].getblock, all_blocks[-1].hash)

        headers_message.headers.pop() # Ensure the last block is unrequested
        white_node.send_message(headers_message) # Send headers leading to tip
        white_node.send_message(MsgBlock(tips[1]))  # Now deliver the tip
        white_node.sync_with_ping()
        self.nodes[1].getblock(tips[1].hash)
        self.log.info("Unrequested block far ahead of tip accepted from whitelisted peer")

        # 5. Test handling of unrequested block on the node that didn't process
        # Should still not be processed (even though it has a child that has more
        # work).
        test_node.send_message(MsgBlock(blocks_h2f[0]))

        # Here, if the sleep is too short, the test could falsely succeed (if the
        # node hasn't processed the block by the time the sleep returns, and then
        # the node processes it and incorrectly advances the tip).
        # But this would be caught later on, when we verify that an inv triggers
        # a getdata request for this block.
        test_node.sync_with_ping()
        assert_equal(self.nodes[0].getblockcount(), 2)
        self.log.info("Unrequested block that would complete more-work chain was ignored")

        # 6. Try to get node to request the missing block.
        # Poke the node with an inv for block at height 3 and see if that
        # triggers a getdata on block 2 (it should if block 2 is missing).
        with mininode_lock:
            # Clear state so we can check the getdata request
            test_node.last_message.pop("getdata", None)
            test_node.send_message(MsgInv([CInv(2, blocks_h3[0].sha256)]))

        test_node.sync_with_ping()
        with mininode_lock:
            getdata = test_node.last_message["getdata"]

        # Check that the getdata includes the right block
        assert_equal(getdata.inv[0].hash, blocks_h2f[0].sha256)
        self.log.info("Inv at tip triggered getdata for unprocessed block")

        # 7. Send the missing block for the third time (now it is requested)
        test_node.send_message(MsgBlock(blocks_h2f[0]))

        test_node.sync_with_ping()
        assert_equal(self.nodes[0].getblockcount(), 290)
        self.log.info("Successfully reorged to longer chain from non-whitelisted peer")

        # 8. Connect node2 to node0 and ensure it is able to sync
        connect_nodes(self.nodes[0], 2)
        sync_blocks([self.nodes[0], self.nodes[2]])
        self.log.info("Successfully synced nodes 2 and 0")

        [ c.disconnect_node() for c in connections ]

if __name__ == '__main__':
    AcceptBlockTest().main()
