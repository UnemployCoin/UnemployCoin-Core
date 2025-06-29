<h1 align="center">
  UnemployCoin: A Peer-to-Peer Electronic System for the Creation and Transfer of Assets<br>
</h1>


<p align="center">
UnemployCoin Core Devs (ME)  
https://www.unemploycoin.com  
June 22, 2025
</p>
<p align="center"><i>
"In a world that left us behind, we built a system no one can take from us."
</i></p> 

Thank you to the Bitcoin founder and developers. The UnemployCoin project was launched based on the hard work and continuous effort of over 430 Bitcoin developers who made over 14,000 commits by the date of the UnemployCoin code fork. We are eternally grateful to you for your diligence in making a secure network and for your support of free and open source software development. The UnemployCoin project is built on the foundation you built.

> Abstract

UnemployCoin is a blockchain and platform optimized for transferring assets — such as tokens — between holders, securely and transparently. Built on a fork of the Bitcoin codebase, it uses the proven UTXO model while introducing unique enhancements: 1-minute block times, modified coin supply, and native support for user-defined asset creation and messaging.

UnemployCoin uses the Proof-of-Work (PoW) x16r algorithm, designed to resist centralization. All UNEMP coins are fairly mined — no premine, no dev fund, no backdoor allocation. Every token exists transparently on-chain. The platform emphasizes privacy, censorship resistance, and user control, while enabling assets to be created and moved without relying on intermediaries.

UnemployCoin is free and open source, built for people — not institutions — and open to innovation anywhere in the world.

## 1. Introduction

A blockchain is a public ledger that records ownership. It enables anyone to 
transfer control of that ownership to others. Among all possible blockchain use 
cases, tracking who owns what has proven to be foundational — evidenced by 
Bitcoin’s unmatched resilience since 2009.

Ethereum and its standards (ERC20, ERC223, ERC721) demonstrated asset 
tokenization, offering speed, flexibility, and user control. Yet, neither 
Bitcoin nor Ethereum were originally built to handle user-defined assets 
natively.

UnemployCoin focuses on doing one thing exceptionally well: transferring assets 
between users, securely and efficiently. The protocol offers tools for asset 
issuance, transfer, and communication — all in a native environment where the 
wallet, node, and asset logic are aligned.

In a world moving toward decentralized capital and global borderless trade, 
platforms like UnemployCoin give individuals a fair shot to create and exchange 
value — no gatekeepers, no restrictions.

## 2. Background Tokens and Other Assets

On January 3, 2009, Bitcoin was launched as a peer-to-peer electronic cash system. Years later, after it achieved a notable level of security, it was recognized that assets could be created "on top of" or embedded in the Bitcoin blockchain. New assets can be added to the Bitcoin blockchain by creating secure, signed, immutable bitcoin transactions which also carry information on asset issuance, and transfer.

There were several projects that added tokens to the Bitcoin blockchain. The first was Mastercoin[2] by JR Willett, followed by Counterparty[3] and other projects. One category of protocols developed to facilitate the creation of assets on the Bitcoin blockchain became known as ColoredCoins[4], as they mark bitcoin transactions with specially crafted transactions in the OP_RETURN[5], which is like a comment field in the Bitcoin protocol.

The advantage of embedding assets in the Bitcoin blockchain is the high level of security. Bitcoin is considered by many to be the most secure blockchain because there is a tremendous amount of distributed mining power that secures each block with a "high difficulty hash"[6]. Because the distributed Bitcoin nodes recognize the level of effort to create a high difficulty hash, this makes it nearly impossible to re-write, or modify the blockchain without prohibitively high mining investment. To tamper with the Bitcoin blockchain, to re-write or modify its ledger, would take significant efforts from an investor at the level of a nation state.

The disadvantage of embedding assets in the Bitcoin blockchain is that the Bitcoin rules must be followed as originally written, and the Bitcoin nodes are unaware that assets are being embedded. This means that a Bitcoin transaction must be used for every asset transaction, and it must send enough bitcoin to be considered a valid transaction, even though the primary purpose of the transaction is to send the asset. That is inconvenient, but a major disadvantage is that a Bitcoin client that spends that bitcoin without being aware of the embedded asset transaction will destroy the asset. For example, a holder of the Bitcoin private keys to Bitcoin which hold the Counterparty assets, could accidentally send that Bitcoin to an exchange or wallet and lose those assets. A partial solution to solving this issue is to create a special address format that is used for the asset, but that doesn't prevent the mistake that may destroy the asset. It just provides more clues that there is an asset embedded in the transaction.

Other token standards like ERC20, ERC721 and ERC223 are built on Ethereum or other blockchains that support smart contracts. A different problem exists when using these smart contracts. Since the Ethereum network does not natively recognize these smart contract tokens, it is currently unable to protect against some common problems. Smart contracts can be confusing for users as there can be multiple ERC20 tokens with identical names. The only distinction between contracts with identical
names is the contract hash.

## 3. Full Asset Aware Protocol Level System

<p align="center"><i>
Who trades truth for convenience deserves neither security nor freedom."  
– William Shakespeare
</i></p>
The solution is to create a Bitcoin-like system that is fully asset-aware. 
Being asset-aware offers two key advantages: 

1. It protects assets from accidental destruction via built-in awareness.
2. It enables a unified client to issue, track, and transfer assets natively.

To secure these assets, UnemployCoin relies on an active mining community, fair 
distribution, and a decentralized network supported by Proof-of-Work.  
  
**Assets**  

Assets are user-issued tokens on the UnemployCoin blockchain. They are not mined, 
but created by users who define their name, quantity, divisibility, and other rules.

Assets function like bitcoin — they are secured, verifiable, and transferable to 
any valid UnemployCoin address. Unlike ERC tokens or colored coins, these assets 
are fully integrated into the chain and client.

**Uses for Assets**  

Examples of what assets can represent:

- Real-world items: Gold bars, silver, art, land deeds
- Securities: Company shares, co-op participation, royalty rights
- Digital goods: Game items, event tickets, software licenses
- Credits: Gift cards, airline miles, loyalty points
- In-game or virtual currencies

UnemployCoin's native design allows creators to enforce their own rules and define 
the nature of the tokens, while benefiting from the transparency and immutability 
of blockchain technology.

## 4. UnemployCoin Launch and Algorithm

UnemployCoin officially launched June 22, 2025.

This launch introduces a Bitcoin-like system enhanced with native asset handling, 
fast block times, and a new algorithm: **x16r**.

The x16r algorithm was designed to be ASIC-resistant by cycling through sixteen 
different hashing algorithms randomly based on the previous block hash. This helps 
prevent mining centralization and encourages a wide, fair distribution.

----------------------------------------
KEY LAUNCH PRINCIPLES
----------------------------------------

- 🚫 No premine  
- ✅ Fair launch  
- 🔐 Open source from day one  
- ⛏️ Proof-of-Work using x16r  
- 🌐 Global accessibility

Proof-of-Work isn't just about burning electricity — it’s about building a wall 
of time that cannot be rewritten, securing every asset and message forever.

----------------------------------------
COIN DISTRIBUTION
----------------------------------------

All coins are mined by the community. No founders’ reward, no early allocations, 
no privileged wallets. Every UNEMP starts the same: earned.

The system mirrors Bitcoin’s issuance model with adjusted timing and halving to 
fit its asset-focused mission.

## 5. Asset Issuance & Transfer

<p align="center"><i>
“Build something the system can’t take from you — then give it away.”  
\- Edgar Allan Poe, The UnemployCoin
</i></p>

UnemployCoin allows any user to issue their own token by burning 500 UNEMP. 
Each token must have a unique name and customizable properties like quantity, 
decimals, and reissuability.

----------------------------------------
ASSET CREATION
----------------------------------------

- 🔥 Requires burning 500 UNEMP  
- 🔤 Unique token name (e.g., LEMONADE, GOLDVAULT, GAMEKEY)  
- 🔢 Define quantity and divisibility  
- ♻️ Reissuable or fixed supply  

Once issued, tokens can be transferred like native currency and fully managed 
within the GUI wallet or by using RPC calls.

----------------------------------------
USE CASE EXAMPLES
----------------------------------------

Token types can include:

- Real-world assets: GOLDVAULT:444322 (serial-tagged gold bars)
- Digital ownership: ART:MonaLisa (unique NFT-style art tokens)
- Access or licenses: GAMEKEY:00001 (software license)
- Tickets and passes: EVENT:2025PASS (event admissions)
- Shares and co-ops: COOP:MEMBER001 (proof of membership)

----------------------------------------
A NEW ECONOMIC LAYER
----------------------------------------

Tokenization opens new paths for entrepreneurs, communities, and even 
governments. From lemonade stands to multinational DAOs, the same simple process 
can launch value-backed tokens that are tradable worldwide — censorship-resistant 
and platform-native.

Fair open-source projects like UnemployCoin let people create value on their own 
terms. No legal team. No paperwork. Just code and consensus.

## 6. Rewards

UnemployCoin supports built-in rewards to asset holders. This lets project 
creators distribute profits, donations, or dividends automatically using the 
protocol.

----------------------------------------
EXAMPLE:
----------------------------------------

> A user creates 10,000 LEMONADE tokens to raise funds for their small business.  
> Later, they succeed and wish to pay back token holders.  
> One command sends UNEMP, pro-rata, to all LEMONADE holders.

This is how UnemployCoin reinvents capital: no borders, no friction, no broker.

----------------------------------------
BUILT-IN MECHANISM
----------------------------------------

- Rewards in native UNEMP  
- Single RPC command for distribution  
- Proportional to asset ownership  
- No need for smart contracts or external services

This enables micro-funding and micro-payments at scale, with ease of use that 
works on desktop and mobile.

Rewards make UnemployCoin more than just a platform — it’s a full economic toolkit.

## 7. Unique Tokens

Some tokens should only exist once. For that, UnemployCoin supports **unique 
tokens** — individual, non-fungible assets built into the protocol.

Unique tokens use a colon `:` namespace and must be created under a parent token.  
For example:

    ART:MonaLisa  
    GAME:SwordOfTruth001  
    CAR:19UYA31581L000000  

These are not interchangeable and represent something singular.

----------------------------------------
EXAMPLES OF UNIQUE TOKEN USE:
----------------------------------------

🎨 Digital Art  
  ART:VenusDeMilo (verified individual art pieces)

🛠️ Software Licenses  
  GAMEKEY:ABC-001 (personal game licenses)

🚗 Vehicles  
  CAR:VIN0011223344 (linked to physical vehicle titles)

🎮 In-Game Assets  
  ZYX_GAME:ThorHammer001 (exclusive virtual items)

🔐 Identity + Proof  
  LICENSE:TX2025DRIVER (unique token ID)

🎁 Collectibles & Merch  
  COINDROP:001 (limited edition physical token companion)

By design, these tokens can only exist once and can be freely transferred — 
allowing real-world or digital ownership to be represented securely on-chain.

## 8. Messaging Stakeholders

<p align="center"><i>
"If the Tower of London ravens are lost or fly away, the Crown will fall and Britain with it." -
Unknown
</i></p>
One major problem in the token space: issuers can’t easily reach holders.

UnemployCoin introduces a native **messaging system** that uses token-linked 
channels for communication. Token holders can receive messages directly from the 
issuer via dedicated, opt-in channels.

----------------------------------------
HOW IT WORKS
----------------------------------------

Each token has a reserved messaging channel, denoted with a tilde (`~`) prefix.  
For example:

    COMPANY → ~COMPANY:Alert  
    ART → ~ART:News

These sub-assets allow authorized accounts to broadcast messages to holders.

----------------------------------------
BENEFITS
----------------------------------------

- 🗣️ Native channel for holders only  
- ❌ Spam-resistant by design  
- ✅ Easy opt-out (client-level)  
- 💬 One-way or authorized sender control  

Messaging turns UnemployCoin assets into two-way systems for more than just 
ownership — they become communication layers as well.

## 9. Voting

With UnemployCoin, any project can create decentralized votes with precision and 
zero intermediaries.

For example, when a vote is triggered:

1. A VOTE token is generated and distributed to holders of an asset.
2. Each VOTE token equals one vote.
3. Users send their vote to a tally address.
4. Vote count is trustless, on-chain, and transparent.

----------------------------------------
ADVANTAGES
----------------------------------------

- 📊 Real shareholder voting for tokenized shares  
- 🧩 Governance models for DAOs or co-ops  
- 🔄 Support for delegation and liquid democracy  
- 🗳️ Votes via wallet, RPC, or mobile tools

Traditional voting is slow, expensive, and gatekept. UnemployCoin’s built-in 
mechanism makes participation decentralized and immediate.

## 10. Privacy

<p align="center"><i>
"It's not about hiding. It's about not being controlled."  
— Anonymous
</i></p>  
Privacy is essential for fungibility, freedom, and frictionless economic exchange.  
UnemployCoin follows the same UTXO model as Bitcoin, which naturally separates 
identity from transactions.

As more features are added — messaging, voting, rewards — preserving privacy 
remains a top priority. No personal info is tied to assets. No account creation 
is needed. The system is open, transparent, and pseudonymous by design.

----------------------------------------
PRIVACY GOALS
----------------------------------------

- 🔐 Protect user identity  
- 🕵️‍♂️ Anonymous asset creation and transfer  
- 🔍 Optional selective disclosure  
- 🔄 Transaction-level privacy for messages and rewards

As technology improves, future updates may enhance privacy through additional 
layered tools, but the foundation is already built on strong anonymity principles.

## 11. EXTENSIBILITY

UnemployCoin is not just a token chain — it's a **platform**.

Any project that forks Bitcoin can leverage UnemployCoin’s asset-aware codebase, 
or even build on top of it with second-layer solutions.

----------------------------------------
POSSIBLE EXTENSIONS INCLUDE:
----------------------------------------

- ⚡ Lightning Network  
- 🔐 Confidential Transactions  
- 🧱 RSK or sidechain integrations  
- 🧪 Custom RPC frameworks  
- 📡 Messaging overlays or mobile interfaces

UnemployCoin is designed to be open to modification, experimentation, and 
collaboration — whether for small teams or global communities.


================================================================================
                               12. CONCLUSION
================================================================================

UnemployCoin is a purpose-built, fair-launch blockchain optimized for the creation 
and transfer of digital assets. With no premine, no VC backing, and no centralized 
authority, it exists to empower individuals.

Using the UTXO model and built on battle-tested Bitcoin infrastructure, UnemployCoin 
adds native asset support, rewards, unique tokens, governance, and messaging —
everything needed to launch decentralized value systems that can’t be shut down.

----------------------------------------
A MESSAGE TO THE COMMUNITY
----------------------------------------

UnemployCoin is for the builders, the laid-off, the forgotten, the creators, 
and the ones who refuse to be told they can't. We don’t beg the system to work 
for us — we build something outside of it.

Whether you’re launching a project, starting a business, or just holding some 
UNEMP for what it stands for — this platform is yours now.

Welcome to the network.

---

References

[1] S. Nakamoto, “Bitcoin: A Peer-to-Peer Electronic Cash System”  
    https://bitcoin.org/bitcoin.pdf  
[2] https://bravenewcoin.com/assets/Whitepapers/2ndBitcoinWhitepaper.pdf  
[3] https://counterparty.io/  
[4] https://en.bitcoin.it/wiki/Colored_Coins  
[5] https://en.bitcoin.it/wiki/OP_RETURN  
[6] https://bitcoinwisdom.com/bitcoin/difficulty  
[7] https://theethereum.wiki/w/index.php/ERC20_Token_Standard  
[8] https://github.com/Dexaran/ERC223-token-standard  
[9] https://www.ethereum.org/  
[10] W. Dai, “B-Money” http://www.weidai.com/bmoney.txt  
[11] https://medium.com/@unemploycoin/unemploycoin-4683cd00f83c  
[12] https://github.com/UnemployCoin  
[13] “X16R Algorithm White Paper” https://unemploycoin.org/x16r-whitepaper  
[14] http://coinspark.org/developers/assets-introduction/  
[15] N. Szabo, “Secure Property Titles with Owner Authority”  
     http://nakamotoinstitute.org/secure-property-titles/  
[16] https://www.forbes.com/2008/09/23/naked-shorting-trades  
[17] https://en.wikipedia.org/wiki/Delegative_democracy  
[18] E. Hughes, “A Cypherpunk’s Manifesto” https://www.activism.net/cypherpunk  
[19] https://bitcoin.org/en/glossary/unspent-transaction-output
