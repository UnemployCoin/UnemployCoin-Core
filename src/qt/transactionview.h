// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2021 The UnemployCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UNEMPLOYCOIN_QT_TRANSACTIONVIEW_H
#define UNEMPLOYCOIN_QT_TRANSACTIONVIEW_H

#include "guiutil.h"

#include <QWidget>
#include <QKeyEvent>

class PlatformStyle;
class TransactionFilterProxy;
class WalletModel;

QT_BEGIN_NAMESPACE
class QComboBox;
class QDateTimeEdit;
class QFrame;
class QLineEdit;
class QMenu;
class QModelIndex;
class QSignalMapper;
class QTableView;
QT_END_NAMESPACE

/** Widget showing the transaction list for a wallet, including a filter row.
    Using the filter row, the user can view or export a subset of the transactions.
  */
class TransactionView : public QWidget
{
    Q_OBJECT

public:
    explicit TransactionView(const PlatformStyle *platformStyle, QWidget *parent = 0);

    void setModel(WalletModel *model);
    void showAssets();

    // Date ranges for filter
    enum DateEnum
    {
        All,
        Today,
        ThisWeek,
        ThisMonth,
        LastMonth,
        ThisYear,
        Range
    };

    enum ColumnWidths {
        STATUS_COLUMN_WIDTH = 30,
        WATCHONLY_COLUMN_WIDTH = 23,
        DATE_COLUMN_WIDTH = 120,
        TYPE_COLUMN_WIDTH = 113,
        ASSET_NAME_MINIMUM_COLUMN_WIDTH = 200,
        AMOUNT_MINIMUM_COLUMN_WIDTH = 120,
        MINIMUM_COLUMN_WIDTH = 23
    };

private:
    WalletModel *model;
    TransactionFilterProxy *transactionProxyModel;
    QTableView *transactionView;

    QComboBox *dateWidget;
    QComboBox *typeWidget;
    QComboBox *watchOnlyWidget;
    QLineEdit *addressWidget;
    QLineEdit *amountWidget;
    QLineEdit *assetNameWidget;

    bool showingAssets;

    QMenu *contextMenu;
    QSignalMapper *mapperThirdPartyTxUrls;

    QFrame *dateRangeWidget;
    QDateTimeEdit *dateFrom;
    QDateTimeEdit *dateTo;
    QAction *abandonAction;
    QAction *bumpFeeAction;

    QWidget *createDateRangeWidget();

    bool eventFilter(QObject *obj, QEvent *event);

private Q_SLOTS:
    void contextualMenu(const QPoint &);
    void dateRangeChanged();
    void showDetails();
    void copyAddress();
    void editLabel();
    void copyLabel();
    void copyAssetName();
    void copyAmount();
    void copyTxID();
    void copyTxHex();
    void copyTxPlainText();
    void openThirdPartyTxUrl(QString url);
    void updateWatchOnlyColumn(bool fHaveWatchOnly);
    void abandonTx();
    void bumpFee();

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);

    /**  Fired when a message should be reported to the user */
    void message(const QString &title, const QString &message, unsigned int style);

public Q_SLOTS:
    void chooseDate(int idx);
    void chooseType(int idx);
    void chooseWatchonly(int idx);
    void changedAmount();
    void changedAssetName();
    void changedPrefix();
    void exportClicked();
    void focusTransaction(const QModelIndex&);

};

#endif // UNEMPLOYCOIN_QT_TRANSACTIONVIEW_H
