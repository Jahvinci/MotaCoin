// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2018 The PIVX developers
// Copyright (c) 2018-2026 The MotaCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_BRIDGEPAGE_H
#define BITCOIN_QT_BRIDGEPAGE_H

#include <QWidget>

namespace Ui {
    class BridgePage;
}

class WalletModel;
class ClientModel;

/** Bridge page widget */
class BridgePage : public QWidget
{
    Q_OBJECT

public:
    explicit BridgePage(QWidget *parent = 0);
    ~BridgePage();

    void setWalletModel(WalletModel *model);
    void setClientModel(ClientModel *model);

private Q_SLOTS:
    void on_initiateButton_clicked();
    void onMotaToSolClicked();
    void onSolToMotaClicked();
    void onCopyEscrowClicked();
    void onCopyMemoClicked();
    void onMotaAddressChanged(int index);

private:
    Ui::BridgePage *ui;
    WalletModel *walletModel;
    ClientModel *clientModel;

    // Static constants
    static const QString ESCROW_ADDRESS;

    // Helper functions
    void populateMotaAddresses();
    bool validateInputs();
    QString createMotaCoinTransaction(const QString &solanaAddress, double amount);
    void showErrorMessage(const QString &title, const QString &message);
    void showSuccessMessage(const QString &title, const QString &message);
};

#endif // BITCOIN_QT_BRIDGEPAGE_H
