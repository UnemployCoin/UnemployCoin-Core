// Copyright (c) 2011-2014 The Bitcoin Core developers
// Copyright (c) 2017-2019 The UnemployCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UNEMPLOYCOIN_QT_UNEMPLOYCOINADDRESSVALIDATOR_H
#define UNEMPLOYCOIN_QT_UNEMPLOYCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class RavenAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit RavenAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** UnemployCoin address widget validator, checks for a valid unemploycoin address.
 */
class RavenAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit RavenAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // UNEMPLOYCOIN_QT_UNEMPLOYCOINADDRESSVALIDATOR_H
