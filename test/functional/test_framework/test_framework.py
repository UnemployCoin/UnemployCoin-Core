#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Copyright (c) 2017-2020 The UnemployCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Base class for RPC testing."""

from enum import Enum
from random import randint
import logging
import optparse
import os
import pdb
import shutil
import sys
import tempfile
import time

from .authproxy import JSONRPCException
from . import coverage
from .test_node import TestNode
from .util import (MAX_NODES, PortSeed, assert_equal, check_json_precision, connect_nodes_bi, disconnect_nodes,
                   initialize_data_dir, log_filename, p2p_port, set_node_times, sync_blocks, sync_mempools)


class TestStatus(Enum):
    PASSED = 1
    FAILED = 2
    SKIPPED = 3


TEST_EXIT_PASSED = 0
TEST_EXIT_FAILED = 1
TEST_EXIT_SKIPPED = 77


class RavenTestFramework:
    """Base class for a unemploycoin test script.

    Individual unemploycoin test scripts should subclass this class and override the set_test_params() and run_test() methods.

    Individual tests can also override the following methods to customize the test setup:

    - add_options()
    - setup_chain()
    - setup_network()
    - setup_nodes()

    The __init__() and main() methods should not be overridden.

    This class also contains various public and private helper methods."""

    def __init__(self):
        """Sets test framework defaults. Do not override this method. Instead, override the set_test_params() method"""
        self.num_nodes = None
        self.setup_clean_chain = False
        self.nodes = []
        self.mocktime = 0
        self.set_test_params()

        assert hasattr(self, "num_nodes"), "Test must set self.num_nodes in set_test_params()"

    def main(self):
        """Main function. This should not be overridden by the subclass test scripts."""
        parser = optparse.OptionParser(usage="%prog [options]")
        parser.add_option("--cachedir", dest="cachedir", default=os.path.normpath(os.path.dirname(os.path.realpath(__file__)) + "/../../cache"), help="Directory for caching pregenerated datadirs")
        parser.add_option("--coveragedir", dest="coveragedir", help="Write tested RPC commands into this directory")
        parser.add_option("--configfile", dest="configfile", help="Location of the test framework config file")
        parser.add_option("--loglevel", dest="loglevel", default="INFO", help="log events at this level and higher to the console. Can be set to DEBUG, INFO, WARNING, ERROR or CRITICAL. Passing --loglevel DEBUG will output all logs to console. Note that logs at all levels are always written to the test_framework.log file in the temporary test directory.")
        parser.add_option("--nocleanup", dest="nocleanup", default=False, action="store_true", help="Leave unemploycoinds and test.* datadir on exit or error")
        parser.add_option("--noshutdown", dest="noshutdown", default=False, action="store_true", help="Don't stop unemploycoinds after the test execution")
        parser.add_option("--pdbonfailure", dest="pdbonfailure", default=False, action="store_true", help="Attach a python debugger if test fails")
        parser.add_option("--portseed", dest="port_seed", default=os.getpid(), type='int', help="The seed to use for assigning port numbers (default: current process id)")
        parser.add_option("--srcdir", dest="srcdir", default=os.path.normpath(os.path.dirname(os.path.realpath(__file__)) + "/../../../src"), help="Source directory containing unemploycoind/unemploycoin-cli (default: %default)")
        parser.add_option("--tmpdir", dest="tmpdir", help="Root directory for datadirs")
        parser.add_option("--tracerpc", dest="trace_rpc", default=False, action="store_true", help="Print out all RPC calls as they are made")

        self.add_options(parser)
        (self.options, self.args) = parser.parse_args()

        PortSeed.n = self.options.port_seed

        os.environ['PATH'] = self.options.srcdir + ":" + self.options.srcdir + "/qt:" + os.environ['PATH']

        check_json_precision()

        self.options.cachedir = os.path.abspath(self.options.cachedir)

        # Set up temp directory and start logging
        # When looping tests using the --loop=n argument, when a test fails the tmpdir will not be deleted.
        # This will cause that specific test to fail on every loop after since this directory will exist.
        # Now when it fails to create the tmpdir it will try to create a new directory with a random int
        # appended to the path. This will allow looped tests to pass and keep the log and state data from the previous failure.
        if self.options.tmpdir:
            self.options.tmpdir = os.path.abspath(self.options.tmpdir)
            try:
                os.makedirs(self.options.tmpdir, exist_ok=False)
            except OSError:
                self.options.tmpdir = os.path.abspath(self.options.tmpdir + str(randint(0, 48)))
                os.makedirs(self.options.tmpdir, exist_ok=False)
        else:
            self.options.tmpdir = tempfile.mkdtemp(prefix="test")
        self._start_logging()

        success = TestStatus.FAILED

        try:
            self.setup_chain()
            self.setup_network()
            self.run_test()
            success = TestStatus.PASSED
        except JSONRPCException as e:
            self.log.exception("JSONRPC error: %s", e)
        except SkipTest as e:
            self.log.warning("Test Skipped: %s" % e.message)
            success = TestStatus.SKIPPED
        except AssertionError as e:
            self.log.exception("Assertion failed: %s", e)
        except KeyError as e:
            self.log.exception("Key error: %s", e)
        except Exception as e:
            self.log.exception("Unexpected exception caught during testing: %s", e)
        except KeyboardInterrupt as e:
            self.log.warning("Exiting after keyboard interrupt: %s", e)

        if success == TestStatus.FAILED and self.options.pdbonfailure:
            self.log.info("Testcase failed. Attaching python debugger. Enter ? for help")
            pdb.set_trace()

        if not self.options.noshutdown:
            self.log.info("Stopping nodes")
            if self.nodes:
                self.stop_nodes()
        else:
            for node in self.nodes:
                node.cleanup_on_exit = False
            self.log.info("Note: unemploycoind's were not stopped and may still be running")

        if not self.options.nocleanup and not self.options.noshutdown and success != TestStatus.FAILED:
            self.log.info("Cleaning up")
            shutil.rmtree(self.options.tmpdir)
        else:
            self.log.warning("Not cleaning up dir %s" % self.options.tmpdir)

        if success == TestStatus.PASSED:
            self.log.info("Tests successful")
            sys.exit(TEST_EXIT_PASSED)
        elif success == TestStatus.SKIPPED:
            self.log.info("Test skipped")
            sys.exit(TEST_EXIT_SKIPPED)
        else:
            self.log.error("Test failed. Test logging available at %s/test_framework.log", self.options.tmpdir)
            logging.shutdown()
            sys.exit(TEST_EXIT_FAILED)

    # Methods to override in subclass test scripts.
    def set_test_params(self):
        """Tests must this method to change default values for number of nodes, topology, etc"""
        raise NotImplementedError

    def add_options(self, parser):
        """Override this method to add command-line options to the test"""
        pass

    def setup_chain(self):
        """Override this method to customize blockchain setup"""
        self.log.info("Initializing test directory " + self.options.tmpdir)
        if self.setup_clean_chain:
            self._initialize_chain_clean()
        else:
            self._initialize_chain()

    def setup_network(self):
        """Override this method to customize test network topology"""
        self.log.info("Running setup_network")
        self.setup_nodes()

        # Connect the nodes as a "chain".  This allows us
        # to split the network between nodes 1 and 2 to get
        # two halves that can work on competing chains.
        for i in range(self.num_nodes - 1):
            connect_nodes_bi(self.nodes, i, i + 1)
        self.sync_all()

    def setup_nodes(self):
        """Override this method to customize test node setup"""
        self.log.info("Running setup_nodes")
        extra_args = None
        if hasattr(self, "extra_args"):
            extra_args = self.extra_args
        self.add_nodes(self.num_nodes, extra_args, False)
        self.start_nodes()

    def run_test(self):
        """Tests must override this method to define test logic"""
        raise NotImplementedError

    # Public helper methods. These can be accessed by the subclass test scripts.

    def add_nodes(self, num_nodes, extra_args=None, rpchost=None, timewait=None, binary=None):
        """Instantiate TestNode objects"""

        if extra_args is None:
            extra_args = [[]] * num_nodes
        if binary is None:
            binary = [None] * num_nodes
        assert_equal(len(extra_args), num_nodes)
        assert_equal(len(binary), num_nodes)
        for i in range(num_nodes):
            self.nodes.append(
                TestNode(i, self.options.tmpdir, extra_args[i], rpchost, timewait=timewait, binary=binary[i],
                         stderr=None, mocktime=self.mocktime, coverage_dir=self.options.coveragedir))

    def start_node(self, i, extra_args=None, stderr=None):
        """Start a unemploycoind"""

        node = self.nodes[i]

        node.start(extra_args, stderr)
        node.wait_for_rpc_connection()

        if self.options.coveragedir is not None:
            coverage.write_all_rpc_commands(self.options.coveragedir, node.rpc)

    def start_nodes(self, extra_args=None):
        """Start multiple unemploycoinds"""

        if extra_args is None:
            extra_args = [None] * self.num_nodes
        assert_equal(len(extra_args), self.num_nodes)
        try:
            for i, node in enumerate(self.nodes):
                node.start(extra_args[i])
            for node in self.nodes:
                node.wait_for_rpc_connection()
        except Exception:
            # If one node failed to start, stop the others
            self.stop_nodes()
            raise

        if self.options.coveragedir is not None:
            for node in self.nodes:
                coverage.write_all_rpc_commands(self.options.coveragedir, node.rpc)

    def stop_node(self, i):
        """Stop a unemploycoind test node"""
        self.nodes[i].stop_node()
        self.nodes[i].wait_until_stopped()

    def stop_nodes(self):
        """Stop multiple unemploycoind test nodes"""
        for node in self.nodes:
            # Issue RPC to stop nodes
            node.stop_node()

        for node in self.nodes:
            # Wait for nodes to stop
            node.wait_until_stopped()

    def restart_node(self, i, extra_args=None):
        """Stop and start a test node"""
        self.stop_node(i)
        self.start_node(i, extra_args)

    def assert_start_raises_init_error(self, i, extra_args=None, expected_msg=None):
        with tempfile.SpooledTemporaryFile(max_size=2 ** 16) as log_stderr:
            try:
                self.start_node(i, extra_args, stderr=log_stderr)
                self.stop_node(i)
            except Exception as e:
                assert 'unemploycoind exited' in str(e)  # node must have shutdown
                self.nodes[i].running = False
                self.nodes[i].process = None
                if expected_msg is not None:
                    log_stderr.seek(0)
                    stderr = log_stderr.read().decode('utf-8')
                    if expected_msg not in stderr:
                        raise AssertionError("Expected error \"" + expected_msg + "\" not found in:\n" + stderr)
            else:
                if expected_msg is None:
                    assert_msg = "unemploycoind should have exited with an error"
                else:
                    assert_msg = "unemploycoind should have exited with expected error " + expected_msg
                raise AssertionError(assert_msg)

    def wait_for_node_exit(self, i, timeout):
        self.log.debug("Waiting for node %d exit start: %s", i, time.ctime())
        self.nodes[i].process.wait(timeout)
        self.log.debug("Waiting for node %d exit end: %s", i, time.ctime())

    def split_network(self):
        """
        Split the network of four nodes into nodes 0/1 and 2/3.
        """
        disconnect_nodes(self.nodes[1], 2)
        disconnect_nodes(self.nodes[2], 1)
        self.sync_all([self.nodes[:2], self.nodes[2:]])

    def join_network(self):
        """
        Join the (previously split) network halves together.
        """
        connect_nodes_bi(self.nodes, 1, 2)
        self.sync_all()

    def sync_all(self, node_groups=None):
        if not node_groups:
            node_groups = [self.nodes]

        for group in node_groups:
            sync_blocks(group)
            sync_mempools(group)

    def enable_mocktime(self):
        """Enable mocktime for the script.

        mocktime may be needed for scripts that use the cached version of the
        blockchain.  If the cached version of the blockchain is used without
        mocktime then the mempools will not sync due to IBD.

        For backwared compatibility of the python scripts with previous
        versions of the cache, this helper function sets mocktime to November 7,
        2017 + (201 * 1 * 60)

        NOTE: the timestamp should match time of genesis block.
        NOTE: the timestamp could be retrieved via "getblockhash 0" + "getblock <hash>"."""
        self.mocktime = 1524179366 + (201 * 1 * 60)

    def disable_mocktime(self):
        self.mocktime = 0

    # Private helper methods. These should not be accessed by the subclass test scripts.

    def _start_logging(self):
        # Add logger and logging handlers
        self.log = logging.getLogger('TestFramework')
        self.log.setLevel(logging.DEBUG)
        # Create file handler to log all messages
        fh = logging.FileHandler(self.options.tmpdir + '/test_framework.log')
        fh.setLevel(logging.DEBUG)
        # Create console handler to log messages to stderr. By default this logs only error messages, but can be configured with --loglevel.
        ch = logging.StreamHandler(sys.stdout)
        # User can provide log level as a number or string (eg DEBUG). loglevel was caught as a string, so try to convert it to an int
        ll = int(self.options.loglevel) if self.options.loglevel.isdigit() else self.options.loglevel.upper()
        ch.setLevel(ll)
        # Format logs the same as unemploycoind's debug.log with microprecision (so log files can be concatenated and sorted)
        formatter = logging.Formatter(fmt='%(asctime)s.%(msecs)03d000 %(name)s (%(levelname)s): %(message)s',
                                      datefmt='%Y-%m-%d %H:%M:%S')
        formatter.converter = time.gmtime
        fh.setFormatter(formatter)
        ch.setFormatter(formatter)
        # add the handlers to the logger
        self.log.addHandler(fh)
        self.log.addHandler(ch)

        if self.options.trace_rpc:
            rpc_logger = logging.getLogger("RavenRPC")
            rpc_logger.setLevel(logging.DEBUG)
            rpc_handler = logging.StreamHandler(sys.stdout)
            rpc_handler.setLevel(logging.DEBUG)
            rpc_logger.addHandler(rpc_handler)

    def _initialize_chain(self):
        """Initialize a pre-mined blockchain for use by the test.

        Create a cache of a 200-block-long chain (with wallet) for MAX_NODES
        Afterward, create num_nodes copies from the cache."""

        assert self.num_nodes <= MAX_NODES
        create_cache = False
        for i in range(MAX_NODES):
            if not os.path.isdir(os.path.join(self.options.cachedir, 'node' + str(i))):
                create_cache = True
                break

        if create_cache:
            self.log.debug("Creating data directories from cached datadir")

            # find and delete old cache directories if any exist
            for i in range(MAX_NODES):
                if os.path.isdir(os.path.join(self.options.cachedir, "node" + str(i))):
                    shutil.rmtree(os.path.join(self.options.cachedir, "node" + str(i)))

            # Create cache directories, run unemploycoinds:
            for i in range(MAX_NODES):
                datadir = initialize_data_dir(self.options.cachedir, i)
                args = [os.getenv("UNEMPLOYCOIND", "unemploycoind"), "-server", "-keypool=1", "-datadir=" + datadir, "-discover=0"]
                if i > 0:
                    args.append("-connect=127.0.0.1:" + str(p2p_port(0)))
                self.nodes.append(
                    TestNode(i, self.options.cachedir, extra_args=[], rpchost=None, timewait=None, binary=None,
                             stderr=None, mocktime=self.mocktime, coverage_dir=None))
                self.nodes[i].args = args
                self.start_node(i)

            # Wait for RPC connections to be ready
            for node in self.nodes:
                node.wait_for_rpc_connection()

            # Create a 200-block-long chain; each of the 4 first nodes
            # gets 25 mature blocks and 25 immature.
            # Note: To preserve compatibility with older versions of
            # initialize_chain, only 4 nodes will generate coins.
            #
            # blocks are created with timestamps 10 minutes apart
            # starting from 2010 minutes in the past
            self.enable_mocktime()
            block_time = self.mocktime - (201 * 1 * 60)
            for i in range(2):
                for peer in range(4):
                    for _ in range(25):
                        set_node_times(self.nodes, block_time)
                        self.nodes[peer].generate(1)
                        block_time += 1 * 60
                    # Must sync before next peer starts generating blocks
                    sync_blocks(self.nodes)

            # Shut them down, and clean up cache directories:
            self.stop_nodes()
            self.nodes = []
            self.disable_mocktime()
            for i in range(MAX_NODES):
                os.remove(log_filename(self.options.cachedir, i, "debug.log"))
                os.remove(log_filename(self.options.cachedir, i, "db.log"))
                os.remove(log_filename(self.options.cachedir, i, "peers.dat"))
                os.remove(log_filename(self.options.cachedir, i, "fee_estimates.dat"))

        for i in range(self.num_nodes):
            from_dir = os.path.join(self.options.cachedir, "node" + str(i))
            to_dir = os.path.join(self.options.tmpdir, "node" + str(i))
            shutil.copytree(from_dir, to_dir)
            initialize_data_dir(self.options.tmpdir, i)  # Overwrite port/rpcport in unemploycoin.conf

    def _initialize_chain_clean(self):
        """Initialize empty blockchain for use by the test.

        Create an empty blockchain and num_nodes wallets.
        Useful if a test case wants complete control over initialization."""
        for i in range(self.num_nodes):
            initialize_data_dir(self.options.tmpdir, i)


class SkipTest(Exception):
    """This exception is raised to skip a test"""

    def __init__(self, message):
        self.message = message
