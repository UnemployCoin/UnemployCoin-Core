#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Copyright (c) 2017-2020 The UnemployCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Test p2p mempool message.

Test that nodes are disconnected if they send mempool messages when bloom
filters are not enabled.
"""

from test_framework.mininode import NodeConn, NodeConnCB, NetworkThread, MsgMempool
from test_framework.test_framework import RavenTestFramework
from test_framework.util import p2p_port, assert_equal

class P2PMempoolTests(RavenTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-peerbloomfilters=0"]]

    def run_test(self):
        #connect a mininode
        aTestNode = NodeConnCB()
        node = NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], aTestNode)
        aTestNode.add_connection(node)
        NetworkThread().start()
        aTestNode.wait_for_verack()

        #request mempool
        aTestNode.send_message(MsgMempool())
        aTestNode.wait_for_disconnect()

        #mininode must be disconnected at this point
        assert_equal(len(self.nodes[0].getpeerinfo()), 0)
    
if __name__ == '__main__':
    P2PMempoolTests().main()
