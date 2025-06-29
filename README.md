# UnemployCoin Core

Welcome to the official source code for **UnemployCoin**, a community-driven cryptocurrency focused on fair distribution, transparency, and empowerment for the unemployed and underserved.

UnemployCoin Core is based on Ravencoin and carries forward a Proof-of-Work model with no ICO, no pre-mine, and a strong focus on decentralized development.

---

## 🎯 Key Features

- 💰 1 Billion fixed supply
- ⛏️ Proof-of-Work mining (GPU-friendly)
- 📉 Bitcoin-style halving to reduce inflation
- 🗳️ Community-focused development — no pre-sale, no ICO
- 💻 Full wallet support (CLI + GUI)
- 🌐 Built for transparency, utility, and long-term support

---

## ⚙️ Build Instructions

### Requirements

- Ubuntu (recommended)
- `build-essential`, `libssl-dev`, `libboost-all-dev`, `libevent-dev`, `qttools5-dev`, `libqrencode-dev`

### Build (Linux)

```bash
git clone https://github.com/UnemployCoin/UnemployCoin-Core.git
cd UnemployCoin-Core
./autogen.sh
./configure
make -j$(nproc)
```
### For the GUI wallet:

```bash
cd src/qt
make
```
### Contributing

We welcome open contributions from the community.

Fork the repo

Create a new branch

Submit a pull request with your changes

More info in CONTRIBUTING.md

---

### License
This project is licensed under the MIT license. See LICENSE for details.

---
### Useful Links

Website: https://unemploycoin.com
Chatbot: https://unemploycoin.com/UnemployCoinChatBot/
Support Us: https://buymeacoffee.com/unemploycoin

---

💬 Built by the people, for the people. Power to the Unemployed.
