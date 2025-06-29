#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Copyright (c) 2017-2020 The UnemployCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Class for unemploycoind node under test"""

import decimal
import errno
import http.client
import json
import logging
import os
import re
import subprocess
import time

from .util import assert_equal, get_rpc_proxy, rpc_url, wait_until
from .authproxy import JSONRPCException, AuthServiceProxy

UNEMPLOYCOIND_PROC_WAIT_TIMEOUT = 60


class TestNode:
    """A class for representing a unemploycoind node under test.

    This class contains:

    - state about the node (whether it's running, etc)
    - a Python subprocess.Popen object representing the running process
    - an RPC connection to the node

    To make things easier for the test writer, a bit of magic is happening under the covers.
    Any unrecognised messages will be dispatched to the RPC connection."""

    def __init__(self, i, dirname, extra_args, rpchost, timewait, binary, stderr, mocktime, coverage_dir):
        self.index = i
        self.datadir = os.path.join(dirname, "node" + str(i))
        self.rpchost = rpchost
        if timewait:
            self.rpc_timeout = timewait
        else:
            # Wait for up to 60 seconds for the RPC server to respond
            self.rpc_timeout = 60
        if binary is None:
            self.binary = os.getenv("UNEMPLOYCOIND", "unemploycoind")
        else:
            self.binary = binary
        self.stderr = stderr
        self.coverage_dir = coverage_dir
        # Most callers will just need to add extra args to the standard list below. For those callers that need more flexibility, they can just set the args property directly.
        self.extra_args = extra_args
        self.args = [self.binary, "-datadir=" + self.datadir, "-server", "-keypool=2", "-discover=0", "-rest", "-logtimemicros", "-debug", "-debugexclude=libevent", "-debugexclude=leveldb", "-bip44=1", "-mocktime=" + str(mocktime), "-uacomment=testnode%d" % i]

        self.cli = TestNodeCLI(os.getenv("RAVENCLI", "unemploycoin-cli"), self.datadir)

        self.running = False
        AuthServiceProxy.running = False
        self.process = None
        self.rpc_connected = False
        self.rpc = None
        self.url = None
        self.log = logging.getLogger('TestFramework.node%d' % i)
        self.cleanup_on_exit = True  # Whether to kill the node when this object goes away
        self.p2ps = []

    def __del__(self):
        # Ensure that we don't leave any unemploycoind processes lying around after
        # the test ends
        if self.process and self.cleanup_on_exit:
            # Should only happen on test failure
            # Avoid using logger, as that may have already been shutdown when
            # this destructor is called.
            self.log.info("Cleaning up leftover process")
            self.process.kill()

    def __getattr__(self, *args, **kwargs):
        """Dispatches any unrecognised messages to the RPC connection."""
        assert self.rpc_connected and self.rpc is not None, "Error: no RPC connection"
        return self.rpc.__getattr__(*args, **kwargs)

    def start(self, extra_args=None, stderr=None):
        """Start the node."""
        if extra_args is None:
            extra_args = self.extra_args
        if stderr is None:
            stderr = self.stderr
        self.process = subprocess.Popen(self.args + extra_args, stderr=stderr)
        self.running = True
        AuthServiceProxy.running = True
        self.log.debug("unemploycoind started, waiting for RPC to come up")

    def wait_for_rpc_connection(self):
        """Sets up an RPC connection to the unemploycoind process. Returns False if unable to connect."""
        # Poll at a rate of four times per second
        poll_per_s = 4
        for _ in range(poll_per_s * self.rpc_timeout):
            assert self.process.poll() is None, "unemploycoind exited with status %i during initialization" % self.process.returncode
            try:
                self.rpc = get_rpc_proxy(rpc_url(self.datadir, self.index, self.rpchost), self.index, timeout=self.rpc_timeout, coverage_dir=self.coverage_dir)
                self.rpc.getblockcount()
                # If the call to getblockcount() succeeds then the RPC connection is up
                self.rpc_connected = True
                self.url = self.rpc.url
                self.log.debug("RPC successfully started")
                return
            except IOError as e:
                if e.errno != errno.ECONNREFUSED:  # Port not yet open?
                    raise  # unknown IO error
            except JSONRPCException as e:  # Initialization phase
                if e.error['code'] != -28:  # RPC in warmup?
                    raise  # unknown JSON RPC exception
            except ValueError as e:  # cookie file not found and no rpcuser or rpcassword. unemploycoind still starting
                if "No RPC credentials" not in str(e):
                    raise
            time.sleep(1.0 / poll_per_s)
        raise AssertionError("Unable to connect to unemploycoind")

    def get_wallet_rpc(self, wallet_name):
        assert self.rpc_connected
        assert self.rpc
        wallet_path = "wallet/%s" % wallet_name
        return self.rpc / wallet_path

    def stop_node(self):
        """Stop the node."""
        if not self.running:
            return
        self.log.debug("Stopping node")
        try:
            self.stop()
        except http.client.CannotSendRequest:
            self.log.exception("Unable to stop node.")

    def is_node_stopped(self):
        """Checks whether the node has stopped.

        Returns True if the node has stopped. False otherwise.
        This method is responsible for freeing resources (self.process)."""
        if not self.running:
            return True
        return_code = self.process.poll()
        if return_code is None:
            return False

        # process has stopped. Assert that it didn't return an error code.
        assert_equal(return_code, 0)
        self.running = False
        AuthServiceProxy.running = False
        self.process = None
        self.rpc_connected = False
        self.rpc = None
        self.log.debug("Node stopped")
        return True

    def wait_until_stopped(self, timeout=UNEMPLOYCOIND_PROC_WAIT_TIMEOUT):
        wait_until(self.is_node_stopped, err_msg="Wait until Stopped", timeout=timeout)

    def assert_debug_log(self, expected_msgs, timeout=2):
        time_end = time.time() + timeout
        debug_log = os.path.join(self.datadir, self.chain, 'debug.log')
        with open(debug_log, encoding='utf-8') as dl:
            dl.seek(0, 2)
            prev_size = dl.tell()

        yield

        while True:
            found = True
            with open(debug_log, encoding='utf-8') as dl:
                dl.seek(prev_size)
                log = dl.read()
            print_log = " - " + "\n - ".join(log.splitlines())
            for expected_msg in expected_msgs:
                if re.search(re.escape(expected_msg), log, flags=re.MULTILINE) is None:
                    found = False
            if found:
                return
            if time.time() >= time_end:
                break
            time.sleep(0.05)
        self._raise_assertion_error('Expected messages "{}" does not partially match log:\n\n{}\n\n'.format(str(expected_msgs), print_log))

    def node_encrypt_wallet(self, passphrase):
        """"Encrypts the wallet.

        This causes unemploycoind to shutdown, so this method takes
        care of cleaning up resources."""
        self.encryptwallet(passphrase)
        self.wait_until_stopped()


class TestNodeCLI:
    """Interface to unemploycoin-cli for an individual node"""

    def __init__(self, binary, datadir):
        self.args = []
        self.binary = binary
        self.datadir = datadir
        self.input = None

    def __call__(self, *args, input_data=None):
        # TestNodeCLI is callable with unemploycoin-cli command-line args
        self.args = [str(arg) for arg in args]
        self.input = input_data
        return self

    def __getattr__(self, command):
        def dispatcher(*args, **kwargs):
            return self.send_cli(command, *args, **kwargs)

        return dispatcher

    def send_cli(self, command, *args, **kwargs):
        """Run unemploycoin-cli command. Deserializes returned string as python object."""

        pos_args = [str(arg) for arg in args]
        named_args = [str(key) + "=" + str(value) for (key, value) in kwargs.items()]
        assert not (pos_args and named_args), "Cannot use positional arguments and named arguments in the same unemploycoin-cli call"
        p_args = [self.binary, "-datadir=" + self.datadir] + self.args
        if named_args:
            p_args += ["-named"]
        p_args += [command] + pos_args + named_args
        process = subprocess.Popen(p_args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
        cli_stdout, cli_stderr = process.communicate(input=self.input)
        returncode = process.poll()
        if returncode:
            # Ignore cli_stdout, raise with cli_stderr
            raise subprocess.CalledProcessError(returncode, self.binary, output=cli_stderr)
        return json.loads(cli_stdout, parse_float=decimal.Decimal)
