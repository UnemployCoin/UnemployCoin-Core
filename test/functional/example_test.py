#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Copyright (c) 2017-2020 The UnemployCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
An example functional test

The module-level docstring should include a high-level description of
what the test is doing. It's the first thing people see when they open
the file and should give the reader information about *what* the test
is testing and *how* it's being tested
"""

# Imports should be in PEP8 ordering (std library first, then third party
# libraries then local imports).

from collections import defaultdict

# Avoid wildcard * imports if possible
from test_framework.mininode import CInv, NetworkThread, NodeConn, NodeConnCB, mininode_lock, MsgGetdata
from test_framework.test_framework import RavenTestFramework
from test_framework.util import assert_equal, connect_nodes, p2p_port

# NodeConnCB is a class containing callbacks to be executed when a P2P
# message is received from the node-under-test. Subclass NodeConnCB and
# override the on_*() methods if you need custom behaviour.
class BaseNode(NodeConnCB):
    def __init__(self):
        """Initialize the NodeConnCB

        Used to initialize custom properties for the Node that aren't
        included by default in the base class. Be aware that the NodeConnCB
        base class already stores a counter for each P2P message type and the
        last received message of each type, which should be sufficient for the
        needs of most tests.

        Call super().__init__() first for standard initialization and then
        initialize custom properties."""
        super().__init__()
        # Stores a dictionary of all blocks received
        self.block_receive_map = defaultdict(int)

    def on_block(self, conn, message):
        """Override the standard on_block callback

        Store the hash of a received block in the dictionary."""
        message.block.calc_x16r()
        self.block_receive_map[message.block.calc_x16r] += 1

    def on_inv(self, conn, message):
        """Override the standard on_inv callback"""
        pass

def custom_function():
    """Do some custom behaviour

    If this function is more generally useful for other tests, consider
    moving it to a module in test_framework."""
    # self.log.info("running custom_function")  # Oops! Can't run self.log outside the RavenTestFramework
    pass

class ExampleTest(RavenTestFramework):
    # Each functional test is a subclass of the RavenTestFramework class.

    # Override the set_test_params(), add_options(), setup_chain(), setup_network()
    # and setup_nodes() methods to customize the test setup as required.

    def set_test_params(self):
        """Override test parameters for your individual test.

        This method must be overridden and num_nodes must be explicitly set."""
        self.setup_clean_chain = True
        self.num_nodes = 3
        # Use self.extra_args to change command-line arguments for the nodes
        self.extra_args = [[], ["-logips"], []]

        # self.log.info("I've finished set_test_params")  # Oops! Can't run self.log before run_test()

    # Use add_options() to add specific command-line options for your test.
    # In practice this is not used very much, since the tests are mostly written
    # to be run in automated environments without command-line options.
    # def add_options()
    #     pass

    # Use setup_chain() to customize the node data directories. In practice
    # this is not used very much since the default behaviour is almost always
    # fine
    # def setup_chain():
    #     pass

    def setup_network(self):
        """Setup the test network topology

        Often you won't need to override this, since the standard network topology
        (linear: node0 <-> node1 <-> node2 <-> ...) is fine for most tests.

        If you do override this method, remember to start the nodes, assign
        them to self.nodes, connect them and then sync."""

        self.setup_nodes()

        # In this test, we're not connecting node2 to node0 or node1. Calls to
        # sync_all() should not include node2, since we're not expecting it to
        # sync.
        connect_nodes(self.nodes[0], 1)
        self.sync_all([self.nodes[0:1]])

    # Use setup_nodes() to customize the node start behaviour (for example if
    # you don't want to start all nodes at the start of the test).
    # def setup_nodes():
    #     pass

    def custom_method(self):
        """Do some custom behaviour for this test

        Define it in a method here because you're going to use it repeatedly.
        If you think it's useful in general, consider moving it to the base
        RavenTestFramework class so other tests can use it."""

        self.log.info("Running custom_method")

    def run_test(self):
        """Main test logic"""

        # Create a P2P connection to one of the nodes
        node0 = BaseNode()
        connections = [NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], node0)]
        node0.add_connection(connections[0])

        # Start up network handling in another thread. This needs to be called
        # after the P2P connections have been created.
        NetworkThread().start()
        # wait_for_verack ensures that the P2P connection is fully up.
        node0.wait_for_verack()

        # Generating a block on one of the nodes will get us out of IBD
        blocks = [int(self.nodes[0].generate(nblocks=1)[0], 16)]
        self.sync_all([self.nodes[0:1]])

        # Notice above how we called an RPC by calling a method with the same
        # name on the node object. Notice also how we used a keyword argument
        # to specify a named RPC argument. Neither of those are defined on the
        # node object. Instead there's some __getattr__() magic going on under
        # the covers to dispatch unrecognised attribute calls to the RPC
        # interface.

        # Logs are nice. Do plenty of them. They can be used in place of comments for
        # breaking the test into sub-sections.
        self.log.info("Starting test!")

        self.log.info("Calling a custom function")
        custom_function()

        self.log.info("Calling a custom method")
        self.custom_method()

        self.log.info("Create some blocks")
        self.tip = int(self.nodes[0].getbestblockhash(), 16)
        self.block_time = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['time'] + 1

        self.nodes[0].generate(10)

        self.log.info("Wait for node1 to reach current tip (height 11) using RPC")
        self.nodes[1].waitforblockheight(11)

        self.log.info("Connect node2 and node1")
        connect_nodes(self.nodes[1], 2)

        self.log.info("Add P2P connection to node2")
        node2 = BaseNode()
        connections.append(NodeConn('127.0.0.1', p2p_port(2), self.nodes[2], node2))
        node2.add_connection(connections[1])
        node2.wait_for_verack()

        self.log.info("Wait for node2 reach current tip. Test that it has propagated all the blocks to us")

        getdata_request = MsgGetdata()
        for block in blocks:
            getdata_request.inv.append(CInv(2, block))
        node2.send_message(getdata_request)
        self.sync_all([self.nodes[1:2]])

        self.log.info("Check that each block was received only once")
        # The network thread uses a global lock on data access to the NodeConn objects when sending and receiving
        # messages. The test thread should acquire the global lock before accessing any NodeConn data to avoid locking
        # and synchronization issues. Note wait_until() acquires this global lock when testing the predicate.
        with mininode_lock:
            for block in node2.block_receive_map.values():
                assert_equal(block, 1)

if __name__ == '__main__':
    ExampleTest().main()
