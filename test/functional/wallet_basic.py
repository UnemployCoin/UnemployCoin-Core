#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Copyright (c) 2017-2020 The UnemployCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test the wallet."""

from test_framework.test_framework import RavenTestFramework
from test_framework.util import connect_nodes_bi, assert_fee_amount, assert_equal, assert_raises_rpc_error, Decimal, count_bytes, sync_mempools, sync_blocks, time, assert_array_result

class WalletTest(RavenTestFramework):
    def set_test_params(self):
        self.num_nodes = 4
        self.setup_clean_chain = True

    def setup_network(self):
        self.add_nodes(4)
        self.start_node(0)
        self.start_node(1)
        self.start_node(2)
        connect_nodes_bi(self.nodes,0,1, True)
        connect_nodes_bi(self.nodes,1,2, True)
        connect_nodes_bi(self.nodes,0,2, True)
        self.sync_all([self.nodes[0:3]])

    @staticmethod
    def check_fee_amount(curr_balance, balance_with_fee, fee_per_byte, tx_size):
        """Return curr_balance after asserting the fee was in range"""
        fee = balance_with_fee - curr_balance
        assert_fee_amount(fee, tx_size, fee_per_byte * 10000)
        return curr_balance

    def run_test(self):
        # Check that there's no UTXO on none of the nodes
        assert_equal(len(self.nodes[0].listunspent()), 0)
        assert_equal(len(self.nodes[1].listunspent()), 0)
        assert_equal(len(self.nodes[2].listunspent()), 0)

        self.log.info("Mining blocks...")

        self.nodes[0].generate(1)

        wallet_info = self.nodes[0].getwalletinfo()
        assert_equal(wallet_info['immature_balance'], 5000)
        assert_equal(wallet_info['balance'], 0)

        self.sync_all([self.nodes[0:3]])
        self.nodes[1].generate(101)
        self.sync_all([self.nodes[0:3]])

        assert_equal(self.nodes[0].getbalance(), 5000)
        assert_equal(self.nodes[1].getbalance(), 5000)
        assert_equal(self.nodes[2].getbalance(), 0)

        # Check that only first and second nodes have UTXOs
        utxos = self.nodes[0].listunspent()
        assert_equal(len(utxos), 1)
        assert_equal(len(self.nodes[1].listunspent()), 1)
        assert_equal(len(self.nodes[2].listunspent()), 0)

        self.log.info("test gettxout")
        confirmed_txid, confirmed_index = utxos[0]["txid"], utxos[0]["vout"]
        # First, outputs that are unspent both in the chain and in the
        # mempool should appear with or without include_mempool
        txout = self.nodes[0].gettxout(txid=confirmed_txid, n=confirmed_index, include_mempool=False)
        assert_equal(txout['value'], 5000)
        txout = self.nodes[0].gettxout(txid=confirmed_txid, n=confirmed_index, include_mempool=True)
        assert_equal(txout['value'], 5000)
        
        # Send 21 UNEMP from 0 to 2 using sendtoaddress call.
        # Locked memory should use at least 32 bytes to sign each transaction
        self.log.info("test getmemoryinfo")
        memory_before = self.nodes[0].getmemoryinfo()
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 11)
        mempool_txid = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 10)
        memory_after = self.nodes[0].getmemoryinfo()
        assert(memory_before['locked']['used'] + 64 <= memory_after['locked']['used'])

        self.log.info("test gettxout (second part)")
        # utxo spent in mempool should be visible if you exclude mempool
        # but invisible if you include mempool
        txout = self.nodes[0].gettxout(confirmed_txid, confirmed_index, False)
        assert_equal(txout['value'], 5000)
        txout = self.nodes[0].gettxout(confirmed_txid, confirmed_index, True)
        assert txout is None
        # new utxo from mempool should be invisible if you exclude mempool
        # but visible if you include mempool
        txout = self.nodes[0].gettxout(mempool_txid, 0, False)
        assert txout is None
        txout1 = self.nodes[0].gettxout(mempool_txid, 0, True)
        txout2 = self.nodes[0].gettxout(mempool_txid, 1, True)
        # note the mempool tx will have randomly assigned indices
        # but 10 will go to node2 and the rest will go to node0
        balance = self.nodes[0].getbalance()
        assert_equal({txout1['value'], txout2['value']}, {10, balance})
        wallet_info = self.nodes[0].getwalletinfo()
        assert_equal(wallet_info['immature_balance'], 0)

        # Have node0 mine a block, thus it will collect its own fee.
        self.nodes[0].generate(1)
        self.sync_all([self.nodes[0:3]])

        # Exercise locking of unspent outputs
        unspent_0 = self.nodes[2].listunspent()[0]
        unspent_0 = {"txid": unspent_0["txid"], "vout": unspent_0["vout"]}
        self.nodes[2].lockunspent(False, [unspent_0])
        assert_raises_rpc_error(-4, "Insufficient funds", self.nodes[2].sendtoaddress, self.nodes[2].getnewaddress(), 20)
        assert_equal([unspent_0], self.nodes[2].listlockunspent())
        self.nodes[2].lockunspent(True, [unspent_0])
        assert_equal(len(self.nodes[2].listlockunspent()), 0)

        # Have node1 generate 100 blocks (so node0 can recover the fee)
        self.nodes[1].generate(100)
        self.sync_all([self.nodes[0:3]])

        # node0 should end up with 100 btc in block rewards plus fees, but
        # minus the 21 plus fees sent to node2
        assert_equal(self.nodes[0].getbalance(), 10000-21)
        assert_equal(self.nodes[2].getbalance(), 21)

        # Node0 should have two unspent outputs.
        # Create a couple of transactions to send them to node2, submit them through
        # node1, and make sure both node0 and node2 pick them up properly:
        node0utxos = self.nodes[0].listunspent(1)
        assert_equal(len(node0utxos), 2)

        # create both transactions
        txns_to_send = []
        for utxo in node0utxos:
            inputs = []
            outputs = {}
            inputs.append({ "txid" : utxo["txid"], "vout" : utxo["vout"]})
            outputs[self.nodes[2].getnewaddress("from1")] = utxo["amount"] - 3
            raw_tx = self.nodes[0].createrawtransaction(inputs, outputs)
            txns_to_send.append(self.nodes[0].signrawtransaction(raw_tx))

        # Have node 1 (miner) send the transactions
        self.nodes[1].sendrawtransaction(txns_to_send[0]["hex"], True)
        self.nodes[1].sendrawtransaction(txns_to_send[1]["hex"], True)

        # Have node1 mine a block to confirm transactions:
        self.nodes[1].generate(1)
        self.sync_all([self.nodes[0:3]])

        assert_equal(self.nodes[0].getbalance(), 0)
        assert_equal(self.nodes[2].getbalance(), 9994)
        assert_equal(self.nodes[2].getbalance("from1"), 9994-21)

        # Send 10 UNEMP normal
        address = self.nodes[0].getnewaddress("test")
        fee_per_byte = Decimal('0.001') / 1000
        self.nodes[2].settxfee(fee_per_byte * 1000)
        txid = self.nodes[2].sendtoaddress(address, 10, "", "", False)
        self.nodes[2].generate(1)
        self.sync_all([self.nodes[0:3]])
        node_2_bal = self.check_fee_amount(self.nodes[2].getbalance(), Decimal('9984'), fee_per_byte, count_bytes(self.nodes[2].getrawtransaction(txid)))
        assert_equal(self.nodes[0].getbalance(), Decimal('10'))

        # Send 10 UNEMP with subtract fee from amount
        txid = self.nodes[2].sendtoaddress(address, 10, "", "", True)
        self.nodes[2].generate(1)
        self.sync_all([self.nodes[0:3]])
        node_2_bal -= Decimal('10')
        assert_equal(self.nodes[2].getbalance(), node_2_bal)
        node_0_bal = self.check_fee_amount(self.nodes[0].getbalance(), Decimal('20'), fee_per_byte, count_bytes(self.nodes[2].getrawtransaction(txid)))

        # Sendmany 10 UNEMP
        txid = self.nodes[2].sendmany('from1', {address: 10}, 0, "", [])
        self.nodes[2].generate(1)
        self.sync_all([self.nodes[0:3]])
        node_0_bal += Decimal('10')
        node_2_bal = self.check_fee_amount(self.nodes[2].getbalance(), node_2_bal - Decimal('10'), fee_per_byte, count_bytes(self.nodes[2].getrawtransaction(txid)))
        assert_equal(self.nodes[0].getbalance(), node_0_bal)

        # Sendmany 10 UNEMP with subtract fee from amount
        txid = self.nodes[2].sendmany('from1', {address: 10}, 0, "", [address])
        self.nodes[2].generate(1)
        self.sync_all([self.nodes[0:3]])
        node_2_bal -= Decimal('10')
        assert_equal(self.nodes[2].getbalance(), node_2_bal)
        self.check_fee_amount(self.nodes[0].getbalance(), node_0_bal + Decimal('10'), fee_per_byte, count_bytes(self.nodes[2].getrawtransaction(txid)))

        # Test ResendWalletTransactions:
        # Create a couple of transactions, then start up a fourth
        # node (nodes[3]) and ask nodes[0] to rebroadcast.
        # EXPECT: nodes[3] should have those transactions in its mempool.
        txid1 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
        txid2 = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        sync_mempools(self.nodes[0:2])

        self.start_node(3)
        connect_nodes_bi(self.nodes, 0, 3)
        sync_blocks(self.nodes)

        relayed = self.nodes[0].resendwallettransactions()
        assert_equal(set(relayed), {txid1, txid2})
        sync_mempools(self.nodes)

        assert(txid1 in self.nodes[3].getrawmempool())

        # Exercise balance rpcs
        assert_equal(self.nodes[0].getwalletinfo()["unconfirmed_balance"], 1)
        assert_equal(self.nodes[0].getunconfirmedbalance(), 1)

        #check if we can list zero value tx as available coins
        #1. create rawtx
        #2. hex-changed one output to 0.0
        #3. sign and send
        #4. check if recipient (node0) can list the zero value tx
        usp = self.nodes[1].listunspent()
        inputs = [{"txid":usp[0]['txid'], "vout":usp[0]['vout']}]
        outputs = {self.nodes[1].getnewaddress(): 4999.998, self.nodes[0].getnewaddress(): 1111.11}

        raw_tx = self.nodes[1].createrawtransaction(inputs, outputs)
        raw_tx = raw_tx.replace("c04fbbde19", "0000000000") #replace 1111.11 with 0.0 (int32)
        self.nodes[1].decoderawtransaction(raw_tx)
        signed_raw_tx = self.nodes[1].signrawtransaction(raw_tx)
        dec_raw_tx = self.nodes[1].decoderawtransaction(signed_raw_tx['hex'])
        zero_value_txid = dec_raw_tx['txid']
        self.nodes[1].sendrawtransaction(signed_raw_tx['hex'])

        self.sync_all()
        self.nodes[1].generate(1) #mine a block
        self.sync_all()

        unspent_txs = self.nodes[0].listunspent() #zero value tx must be in listunspents output
        found = False
        for uTx in unspent_txs:
            if uTx['txid'] == zero_value_txid:
                found = True
                assert_equal(uTx['amount'], Decimal('0'))
        assert found

        #do some -walletbroadcast tests
        self.stop_nodes()
        self.start_node(0, ["-walletbroadcast=0"])
        self.start_node(1, ["-walletbroadcast=0"])
        self.start_node(2, ["-walletbroadcast=0"])
        connect_nodes_bi(self.nodes,0,1)
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)
        self.sync_all([self.nodes[0:3]])

        tx_id_not_broadcasted  = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 2)
        tx_obj_not_broadcasted = self.nodes[0].gettransaction(tx_id_not_broadcasted)
        self.nodes[1].generate(1) #mine a block, tx should not be in there
        self.sync_all([self.nodes[0:3]])
        assert_equal(self.nodes[2].getbalance(), node_2_bal) #should not be changed because tx was not broadcasted

        #now broadcast from another node, mine a block, sync, and check the balance
        self.nodes[1].sendrawtransaction(tx_obj_not_broadcasted['hex'])
        self.nodes[1].generate(1)
        self.sync_all([self.nodes[0:3]])
        node_2_bal += 2
        self.nodes[0].gettransaction(tx_id_not_broadcasted)
        assert_equal(self.nodes[2].getbalance(), node_2_bal)

        #create another tx
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 2)

        #restart the nodes with -walletbroadcast=1
        self.stop_nodes()
        self.start_node(0)
        self.start_node(1)
        self.start_node(2)
        connect_nodes_bi(self.nodes,0,1)
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)
        sync_blocks(self.nodes[0:3])

        self.nodes[0].generate(1)
        sync_blocks(self.nodes[0:3])
        node_2_bal += 2

        #tx should be added to balance because after restarting the nodes tx should be broadcasted
        assert_equal(self.nodes[2].getbalance(), node_2_bal)

        #send a tx with value in a string (PR#6380 +)
        tx_id  = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), "2")
        tx_obj = self.nodes[0].gettransaction(tx_id)
        assert_equal(tx_obj['amount'], Decimal('-2'))

        tx_id  = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), "0.0001")
        tx_obj = self.nodes[0].gettransaction(tx_id)
        assert_equal(tx_obj['amount'], Decimal('-0.0001'))

        #check if JSON parser can handle scientific notation in strings
        tx_id  = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), "1e-4")
        tx_obj = self.nodes[0].gettransaction(tx_id)
        assert_equal(tx_obj['amount'], Decimal('-0.0001'))

        # This will raise an exception because the amount type is wrong
        assert_raises_rpc_error(-3, "Invalid amount", self.nodes[0].sendtoaddress, self.nodes[2].getnewaddress(), "1f-4")

        # This will raise an exception since generate does not accept a string
        assert_raises_rpc_error(-1, "not an integer", self.nodes[0].generate, "2")

        # Import address and private key to check correct behavior of spendable unspents
        # 1. Send some coins to generate new UTXO
        address_to_import = self.nodes[2].getnewaddress()
        self.nodes[0].sendtoaddress(address_to_import, 1)
        self.nodes[0].generate(1)
        self.sync_all([self.nodes[0:3]])

        # 2. Import address from node2 to node1
        self.nodes[1].importaddress(address_to_import)

        # 3. Validate that the imported address is watch-only on node1
        assert(self.nodes[1].validateaddress(address_to_import)["iswatchonly"])

        # 4. Check that the unspents after import are not spendable
        assert_array_result(self.nodes[1].listunspent(),
                           {"address": address_to_import},
                           {"spendable": False})

        # 5. Import private key of the previously imported address on node1
        priv_key = self.nodes[2].dumpprivkey(address_to_import)
        self.nodes[1].importprivkey(priv_key)

        # 6. Check that the unspents are now spendable on node1
        assert_array_result(self.nodes[1].listunspent(),
                           {"address": address_to_import},
                           {"spendable": True})

        # Mine a block from node0 to an address from node1
        cb_address = self.nodes[1].getnewaddress()
        block_hash = self.nodes[0].generatetoaddress(1, cb_address)[0]
        cb_tx_id = self.nodes[0].getblock(block_hash)['tx'][0]
        self.sync_all([self.nodes[0:3]])

        # Check that the txid and balance is found by node1
        self.nodes[1].gettransaction(cb_tx_id)

        # check if wallet or blockchain maintenance changes the balance
        self.sync_all([self.nodes[0:3]])
        blocks = self.nodes[0].generate(2)
        self.sync_all([self.nodes[0:3]])
        balance_nodes = [self.nodes[i].getbalance() for i in range(3)]
        self.nodes[0].getblockcount()

        # Check modes:
        #   - True: unicode escaped as \u....
        #   - False: unicode directly as UTF-8
        for mode in [True, False]:
            self.nodes[0].ensure_ascii = mode
            # unicode check: Basic Multilingual Plane, Supplementary Plane respectively
            for s in [u'рыба', u'𝅘𝅥𝅯']:
                addr = self.nodes[0].getaccountaddress(s)
                label = self.nodes[0].getaccount(addr)
                assert_equal(label, s)
                assert(s in self.nodes[0].listaccounts().keys())
        self.nodes[0].ensure_ascii = True # restore to default

        # maintenance tests
        maintenance = "-rescan -reindex -zapwallettxes=1 -zapwallettxes=2"
        chain_limit = 6
        self.log.info("check " + maintenance)
        self.stop_nodes()
        # set lower ancestor limit for later
        self.start_node(0, [maintenance, "-limitancestorcount="+str(chain_limit)])
        self.start_node(1, [maintenance, "-limitancestorcount="+str(chain_limit)])
        self.start_node(2, [maintenance, "-limitancestorcount="+str(chain_limit)])
        assert_equal(balance_nodes, [self.nodes[i].getbalance() for i in range(3)])

        # Exercise listsinceblock with the last two blocks
        coinbase_tx_1 = self.nodes[0].listsinceblock(blocks[0])
        assert_equal(coinbase_tx_1["lastblock"], blocks[1])
        assert_equal(len(coinbase_tx_1["transactions"]), 1)
        assert_equal(coinbase_tx_1["transactions"][0]["blockhash"], blocks[1])
        assert_equal(len(self.nodes[0].listsinceblock(blocks[1])["transactions"]), 0)

        # Check that wallet prefers to use coins that don't exceed mempool limits =====

        # Get all non-zero utxos together
        chain_addrs = [self.nodes[0].getnewaddress(), self.nodes[0].getnewaddress()]
        single_tx_id = self.nodes[0].sendtoaddress(chain_addrs[0], self.nodes[0].getbalance(), "", "", True)
        self.nodes[0].generate(1)
        node0_balance = self.nodes[0].getbalance()
        # Split into two chains
        rawtx = self.nodes[0].createrawtransaction([{"txid":single_tx_id, "vout":0}], {chain_addrs[0]:node0_balance/2-Decimal('0.01'), chain_addrs[1]:node0_balance/2-Decimal('0.01')})
        signedtx = self.nodes[0].signrawtransaction(rawtx)
        self.nodes[0].sendrawtransaction(signedtx["hex"])
        self.nodes[0].generate(1)

        # Make a long chain of unconfirmed payments without hitting mempool limit
        # Each tx we make leaves only one output of change on a chain 1 longer
        # Since the amount to send is always much less than the outputs, we only ever need one output
        # So we should be able to generate exactly chainlimit txs for each original output
        sending_addr = self.nodes[1].getnewaddress()
        txid_list = []
        for _ in range(chain_limit*2):
            txid_list.append(self.nodes[0].sendtoaddress(sending_addr, Decimal('0.0001')))
        assert_equal(self.nodes[0].getmempoolinfo()['size'], chain_limit*2)
        assert_equal(len(txid_list), chain_limit*2)

        # Without walletrejectlongchains, we will still generate a txid
        # The tx will be stored in the wallet but not accepted to the mempool
        assert_raises_rpc_error(-4, "Error: The transaction was rejected! Reason given: too-long-mempool-chain", self.nodes[0].sendtoaddress, sending_addr, Decimal('0.0001'))
        # Get the last transaction and verify it is not in the mempool
        trans_count = len(self.nodes[0].listtransactions("*",99999))
        extra_txid = (self.nodes[0].listtransactions("*",1, trans_count-1))[0]['txid']
        assert(extra_txid not in self.nodes[0].getrawmempool())
        total_txs = len(self.nodes[0].listtransactions("*",99999))

        # Try with walletrejectlongchains
        # Double chain limit but require combining inputs, so we pass SelectCoinsMinConf
        self.stop_node(0)
        self.start_node(0, extra_args=["-walletrejectlongchains", "-limitancestorcount="+str(2*chain_limit)])

        # wait for loadmempool
        timeout = 10
        while timeout > 0 and len(self.nodes[0].getrawmempool()) < chain_limit*2:
            time.sleep(0.5)
            timeout -= 0.5
        assert_equal(len(self.nodes[0].getrawmempool()), chain_limit*2)

        node0_balance = self.nodes[0].getbalance()
        # With walletrejectlongchains we will not create the tx and store it in our wallet.
        assert_raises_rpc_error(-4, "Transaction has too long of a mempool chain", self.nodes[0].sendtoaddress, sending_addr, node0_balance - Decimal('0.01'))

        # Verify nothing new in wallet
        assert_equal(total_txs, len(self.nodes[0].listtransactions("*",99999)))

if __name__ == '__main__':
    WalletTest().main()
