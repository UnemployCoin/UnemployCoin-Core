// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2021 The UnemployCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UNEMPLOYCOIN_QT_OPTIONSMODEL_H
#define UNEMPLOYCOIN_QT_OPTIONSMODEL_H

#include "amount.h"

#include <QAbstractListModel>

QT_BEGIN_NAMESPACE
class QNetworkProxy;
QT_END_NAMESPACE

/** Interface from Qt to configuration data structure for UnemployCoin client.
   To Qt, the options are presented as a list with the different options
   laid out vertically.
   This can be changed to a tree once the settings become sufficiently
   complex.
 */
class OptionsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit OptionsModel(QObject *parent = 0, bool resetSettings = false);

    enum OptionID {
        StartAtStartup,         // bool
        HideTrayIcon,           // bool
        MinimizeToTray,         // bool
        ToolbarIconsOnly,       // bool
        MapPortUPnP,            // bool
        MinimizeOnClose,        // bool
        ProxyUse,               // bool
        ProxyIP,                // QString
        ProxyPort,              // int
        ProxyUseTor,            // bool
        ProxyIPTor,             // QString
        ProxyPortTor,           // int
        DisplayUnit,            // RavenUnits::Unit
        DisplayCurrencyIndex,   // int
        ThirdPartyTxUrls,       // QString
        IpfsUrl,                // QString
        Language,               // QString
        CoinControlFeatures,    // bool
        ThreadsScriptVerif,     // int
        DatabaseCache,          // int
        SpendZeroConfChange,    // bool
        Listen,                 // bool
        CustomFeeFeatures,      // bool
        DarkModeEnabled,        // bool
        OptionIDRowCount,
    };

    void Init(bool resetSettings = false);
    void Reset();

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    /** Updates current unit in memory, settings and emits displayUnitChanged(newUnit) signal */
    void setDisplayUnit(const QVariant &value);
    /** Updates current unit in memory, settings and emits displayCurrencyIndexChanged(newIndex) signal */
    void setDisplayCurrencyIndex(const QVariant &value);

    /* Explicit getters */
    bool getHideTrayIcon() const { return fHideTrayIcon; }
    bool getMinimizeToTray() const { return fMinimizeToTray; }
    bool getMinimizeOnClose() const { return fMinimizeOnClose; }
    int getDisplayUnit() const { return nDisplayUnit; }
    int getDisplayCurrencyIndex() const { return nDisplayCurrencyIndex; }
    QString getThirdPartyTxUrls() const { return strThirdPartyTxUrls; }
    QString getIpfsUrl() const { return strIpfsUrl; }
    bool getProxySettings(QNetworkProxy& proxy) const;
    bool getCoinControlFeatures() const { return fCoinControlFeatures; }
    bool getCustomFeeFeatures() const { return fCustomFeeFeatures; }
    bool getDarkModeEnabled() const { return fDarkModeEnabled; }
    const QString& getOverriddenByCommandLine() { return strOverriddenByCommandLine; }

    /* Restart flag helper */
    void setRestartRequired(bool fRequired);
    bool isRestartRequired() const;

private:
    /* Qt-only settings */
    bool fHideTrayIcon;
    bool fMinimizeToTray;
    bool fMinimizeOnClose;
    bool fToolbarIconsOnly;
    QString language;
    int nDisplayUnit;
    int nDisplayCurrencyIndex;
    QString strThirdPartyTxUrls;
    QString strIpfsUrl;
    bool fCoinControlFeatures;
    /** UNEMP START*/
    bool fCustomFeeFeatures;
    bool fDarkModeEnabled;
    /** UNEMP END*/
    /* settings that were overridden by command-line */
    QString strOverriddenByCommandLine;

    // Add option to list of GUI options overridden through command line/config file
    void addOverriddenOption(const std::string &option);

    // Check settings version and upgrade default values if required
    void checkAndMigrate();
Q_SIGNALS:
    void displayUnitChanged(int unit);
    void displayCurrencyIndexChanged(int unit);
    void coinControlFeaturesChanged(bool);
    void customFeeFeaturesChanged(bool);
    void hideTrayIconChanged(bool);
    void updateIconsOnlyToolbar(bool);
};

#endif // UNEMPLOYCOIN_QT_OPTIONSMODEL_H
