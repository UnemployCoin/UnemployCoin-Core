#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Copyright (c) 2017-2020 The UnemployCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test wallet group functionality."""

from test_framework.test_framework import RavenTestFramework
from test_framework.mininode import CTransaction, from_hex, to_hex
from test_framework.util import assert_approx, assert_equal

class WalletGroupTest(RavenTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.rpc_timeout = 120


    def run_test(self):
        # Mine some coins
        self.nodes[0].generate(110)

        # Get some addresses from the two nodes
        addr1 = [self.nodes[1].getnewaddress() for _ in range(3)]
        addr2 = [self.nodes[2].getnewaddress() for _ in range(3)]
        addrs = addr1 + addr2

        # Send 1 + 0.5 coin to each address
        [self.nodes[0].sendtoaddress(addr, 1.0) for addr in addrs]
        [self.nodes[0].sendtoaddress(addr, 0.5) for addr in addrs]

        self.nodes[0].generate(1)
        self.sync_all()

        # For each node, send 0.2 coins back to 0;
        # - node[1] should pick one 0.5 UTXO and leave the rest
        # - node[2] should pick one (1.0 + 0.5) UTXO group corresponding to a
        #   given address, and leave the rest
        txid1 = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 0.2)
        tx1 = self.nodes[1].getrawtransaction(txid1, True)
        # txid1 should have 1 input and 2 outputs
        assert_equal(1, len(tx1["vin"]))
        assert_equal(2, len(tx1["vout"]))
        # one output should be 0.2, the other should be ~0.3
        v = [vout["value"] for vout in tx1["vout"]]
        v.sort()
        assert_approx(v[0], 0.2)
        assert_approx(v[1], 0.3, 0.01)

        txid2 = self.nodes[2].sendtoaddress(self.nodes[0].getnewaddress(), 0.2)
        tx2 = self.nodes[2].getrawtransaction(txid2, True)
        # txid2 should have 1 inputs and 2 outputs
        assert_equal(1, len(tx2["vin"]))
        assert_equal(2, len(tx2["vout"]))
        # one output should be 0.2, the other should be ~0.3
        v = [vout["value"] for vout in tx2["vout"]]
        v.sort()
        assert_approx(v[0], 0.2)
        assert_approx(v[1], 0.3, 0.1)

        # Empty out node2's wallet
        self.nodes[2].sendtoaddress(address=self.nodes[0].getnewaddress(), amount=self.nodes[2].getbalance(), subtractfeefromamount=True)
        self.sync_all()
        self.nodes[0].generate(1)

        # Fill node2's wallet with 10000 outputs corresponding to the same
        # scriptPubKey
        for i in range(5):
            inputs = [{"txid":"0"*64, "vout":0}]
            outputs = {addr2[0]: 0.05}
            raw_tx = self.nodes[0].createrawtransaction(inputs, outputs)
            tx = from_hex(CTransaction(), raw_tx)
            tx.vin = []
            tx.vout = [tx.vout[0]] * 2000
            funded_tx = self.nodes[0].fundrawtransaction(to_hex(tx))
            signed_tx = self.nodes[0].signrawtransaction(funded_tx['hex'])
            self.nodes[0].sendrawtransaction(signed_tx['hex'])
            self.nodes[0].generate(1)

        self.sync_all()

        # Check that we can create a transaction that only requires ~100 of our
        # utxos, without pulling in all outputs and creating a transaction that
        # is way too big.
        assert self.nodes[2].sendtoaddress(address=addr2[0], amount=5)

if __name__ == '__main__':
    WalletGroupTest().main ()
