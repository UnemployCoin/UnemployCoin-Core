# Functional tests

### Writing Functional Tests

#### Example test

The [example_test.py](example_test.py) is a heavily commented example of a test case that uses both
the RPC and P2P interfaces. If you are writing your first test, copy that file
and modify to fit your needs.

#### Coverage

Running `test_runner.py` with the `--coverage` argument tracks which RPCs are
called by the tests and prints a report of uncovered RPCs in the summary. This
can be used (along with the `--extended` argument) to find out which RPCs we
don't have test cases for.

#### Style guidelines

- Where possible, try to adhere to [PEP-8 guidelines]([https://www.python.org/dev/peps/pep-0008/)
- Use a python linter like flake8 before submitting PRs to catch common style
  nits (eg trailing whitespace, unused imports, etc)
- Avoid wildcard imports where possible
- Use a module-level docstring to describe what the test is testing, and how it
  is testing it.
- When subclassing the RavenTestFramwork, place overrides for the
  `set_test_params()`, `add_options()` and `setup_xxxx()` methods at the top of
  the subclass, then locally-defined helper methods, then the `run_test()` method.

#### General test-writing advice

- Set `self.num_nodes` to the minimum number of nodes necessary for the test.
  Having additional unrequired nodes adds to the execution time of the test as
  well as memory/CPU/disk requirements (which is important when running tests in
  parallel or on Travis).
- Avoid stop-starting the nodes multiple times during the test if possible. A
  stop-start takes several seconds, so doing it several times blows up the
  runtime of the test.
- Set the `self.setup_clean_chain` variable in `set_test_params()` to control whether
  or not to use the cached data directories. The cached data directories
  contain a 200-block pre-mined blockchain and wallets for four nodes. Each node
  has 25 mature blocks (25x5000=125000 UNEMP) in its wallet.
- When calling RPCs with lots of arguments, consider using named keyword
  arguments instead of positional arguments to make the intent of the call
  clear to readers.

#### RPC and P2P definitions

Test writers may find it helpful to refer to the definitions for the RPC and
P2P messages. These can be found in the following source files:

- `/src/rpc/*` for RPCs
- `/src/wallet/rpc*` for wallet RPCs
- `ProcessMessage()` in `/src/net_processing.cpp` for parsing P2P messages

#### Using the P2P interface

- `mininode.py` contains all the definitions for objects that pass
over the network (`CBlock`, `CTransaction`, etc, along with the network-level
wrappers for them, `msg_block`, `msg_tx`, etc).

- P2P tests have two threads. One thread handles all network communication
with the unemploycoind(s) being tested (using python's asyncore package); the other
implements the test logic.

- `NodeConn` is the class used to connect to a unemploycoind.  If you implement
a callback class that derives from `NodeConnCB` and pass that to the
`NodeConn` object, your code will receive the appropriate callbacks when
events of interest arrive.

- Call `NetworkThread.start()` after all `NodeConn` objects are created to
start the networking thread.  (Continue with the test logic in your existing
thread.)

- Can be used to write tests where specific P2P protocol behavior is tested.
Examples tests are `p2p-accept-block.py`, `p2p-compactblocks.py`.

### test-framework modules

#### [test_framework/authproxy.py](test_framework/authproxy.py)
Taken from the [python-ravenrpc repository](https://github.com/jgarzik/python-ravenrpc).

#### [test_framework/test_framework.py](test_framework/test_framework.py)
Base class for functional tests.

#### [test_framework/util.py](test_framework/util.py)
Generally useful functions.

#### [test_framework/mininode.py](test_framework/mininode.py)
Basic code to support P2P connectivity to a unemploycoind.

#### [test_framework/script.py](test_framework/script.py)
Utilities for manipulating transaction scripts (originally from python-ravenlib)

#### [test_framework/key.py](test_framework/key.py)
Wrapper around OpenSSL EC_Key (originally from python-ravenlib)

#### [test_framework/bignum.py](test_framework/bignum.py)
Helpers for script.py

#### [test_framework/blocktools.py](test_framework/blocktools.py)
Helper functions for creating blocks and transactions.
