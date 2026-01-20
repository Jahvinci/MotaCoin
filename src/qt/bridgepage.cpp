// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2018 The PIVX developers
// Copyright (c) 2018-2020 The MotaCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bridgepage.h"
#include <qt/forms/ui_bridgepage.h>

#include "walletmodel.h"
#include "clientmodel.h"
#include "guiutil.h"
#include "wallet.h"
#include "base58.h"
#include "script.h"
#include "util.h"
#include "main.h"

#include <QMessageBox>
#include <QClipboard>
#include <QApplication>

// Solana escrow address - bridge monitors this address for wMOTA transfers
const QString BridgePage::ESCROW_ADDRESS = "S2MfSdCSY9yAc8LfZ2SeorUqtEM4pKGsczUkXgPeysX";

// MotaCoin escrow address - bridge monitors this for MOTA deposits
const QString MOTA_ESCROW_ADDRESS = "MSo1C3RAQtKbeGtPPiLDpZZB1PaxcZPJY6";

// wMOTA token mint address on Solana
const QString WMOTA_TOKEN_MINT = "MotaiHCWSq1NV9xXio4xtqdjGkk3qpavs5uAk1TLspT";

BridgePage::BridgePage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BridgePage),
    walletModel(0),
    clientModel(0)
{
    ui->setupUi(this);

    // Set initial stack page to MOTA -> Solana
    ui->directionStack->setCurrentIndex(0);

    // Connect direction toggle buttons
    connect(ui->motaToSolButton, SIGNAL(clicked()), this, SLOT(onMotaToSolClicked()));
    connect(ui->solToMotaButton, SIGNAL(clicked()), this, SLOT(onSolToMotaClicked()));

    // Connect send button
    connect(ui->initiateButton, SIGNAL(clicked()), this, SLOT(on_initiateButton_clicked()));

    // Connect copy buttons
    connect(ui->copyEscrowButton, SIGNAL(clicked()), this, SLOT(onCopyEscrowClicked()));
    connect(ui->copyMemoButton, SIGNAL(clicked()), this, SLOT(onCopyMemoClicked()));

    // Connect address combo box
    connect(ui->motaAddressCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onMotaAddressChanged(int)));

    // Set escrow address in the read-only field
    ui->escrowAddressEdit->setText(ESCROW_ADDRESS);
}

BridgePage::~BridgePage()
{
    delete ui;
}

void BridgePage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if (model) {
        // Populate MOTA address combo box with wallet addresses
        populateMotaAddresses();
    }
}

void BridgePage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
}

void BridgePage::populateMotaAddresses()
{
    if (!walletModel || !walletModel->getWallet()) {
        return;
    }

    ui->motaAddressCombo->clear();

    CWallet *wallet = walletModel->getWallet();
    LOCK(wallet->cs_wallet);

    // Get all addresses from the wallet
    BOOST_FOREACH(const PAIRTYPE(CTxDestination, std::string)& item, wallet->mapAddressBook) {
        const CBitcoinAddress& address = item.first;
        const std::string& strName = item.second;

        // Only include receiving addresses (not change addresses)
        if (IsMine(*wallet, address.Get())) {
            QString displayText = QString::fromStdString(address.ToString());
            if (!strName.empty()) {
                displayText = QString::fromStdString(strName) + " (" + displayText + ")";
            }
            ui->motaAddressCombo->addItem(displayText, QString::fromStdString(address.ToString()));
        }
    }

    // Update memo field with first address if available
    if (ui->motaAddressCombo->count() > 0) {
        onMotaAddressChanged(0);
    }
}

void BridgePage::onMotaToSolClicked()
{
    ui->directionStack->setCurrentIndex(0);
}

void BridgePage::onSolToMotaClicked()
{
    ui->directionStack->setCurrentIndex(1);
}

void BridgePage::onCopyEscrowClicked()
{
    QApplication::clipboard()->setText(ui->escrowAddressEdit->text());
    QMessageBox::information(this, tr("Copied"), tr("Escrow address copied to clipboard"));
}

void BridgePage::onCopyMemoClicked()
{
    QString memo = ui->memoEdit->text();
    if (!memo.isEmpty()) {
        QApplication::clipboard()->setText(memo);
        QMessageBox::information(this, tr("Copied"), tr("Memo copied to clipboard"));
    }
}

void BridgePage::onMotaAddressChanged(int index)
{
    if (index < 0) return;

    QString address = ui->motaAddressCombo->itemData(index).toString();
    if (!address.isEmpty()) {
        ui->memoEdit->setText("MOTA:" + address);
    }
}

void BridgePage::on_initiateButton_clicked()
{
    if (!walletModel) {
        showErrorMessage(tr("Bridge Error"), tr("Wallet not available"));
        return;
    }

    // Validate inputs
    if (!validateInputs()) {
        return;
    }

    QString solanaAddress = ui->destinationAddressEdit->text().trimmed();
    double amount = ui->amountEdit->text().toDouble();

    // Create and send transaction
    QString txid = createMotaCoinTransaction(solanaAddress, amount);

    if (!txid.isEmpty()) {
        showSuccessMessage(tr("Transfer Initiated"),
            tr("Transaction sent successfully!\n\n"
               "Transaction ID:\n%1\n\n"
               "Amount: %2 MOTA\n"
               "Destination: %3\n\n"
               "The bridge will mint wMOTA to your Solana address after confirmations.")
               .arg(txid)
               .arg(QString::number(amount, 'f', 2))
               .arg(solanaAddress));

        // Clear form
        ui->amountEdit->clear();
        ui->destinationAddressEdit->clear();
    }
}

bool BridgePage::validateInputs()
{
    // Validate amount
    QString amountStr = ui->amountEdit->text();
    bool ok;
    double amount = amountStr.toDouble(&ok);

    if (!ok || amount <= 0) {
        showErrorMessage(tr("Invalid Input"), tr("Please enter a valid amount"));
        return false;
    }

    // Check minimum amount
    if (amount < 100.0) {
        showErrorMessage(tr("Amount Too Small"), tr("Minimum transfer amount is 100 MOTA"));
        return false;
    }

    // Validate Solana address
    QString solanaAddress = ui->destinationAddressEdit->text().trimmed();
    if (solanaAddress.isEmpty()) {
        showErrorMessage(tr("Invalid Address"), tr("Please enter a Solana address"));
        return false;
    }

    // Basic Solana address validation (base58, 32-44 chars)
    if (solanaAddress.length() < 32 || solanaAddress.length() > 44) {
        showErrorMessage(tr("Invalid Solana Address"),
            tr("Solana addresses are typically 32-44 characters long"));
        return false;
    }

    // Check for valid base58 characters
    QRegExp base58Regex("^[1-9A-HJ-NP-Za-km-z]+$");
    if (!base58Regex.exactMatch(solanaAddress)) {
        showErrorMessage(tr("Invalid Solana Address"),
            tr("Solana address contains invalid characters"));
        return false;
    }

    return true;
}

QString BridgePage::createMotaCoinTransaction(const QString &solanaAddress, double amount)
{
    if (!walletModel) {
        return QString();
    }

    // Lock wallet if needed
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid()) {
        showErrorMessage(tr("Wallet Locked"), tr("Please unlock your wallet to send transactions"));
        return QString();
    }

    // Convert amount to satoshis
    int64_t nAmount = amount * COIN;

    // Create destination for escrow
    CBitcoinAddress address(MOTA_ESCROW_ADDRESS.toStdString());
    if (!address.IsValid()) {
        showErrorMessage(tr("Configuration Error"), tr("Invalid escrow address"));
        return QString();
    }

    CScript scriptPubKey;
    scriptPubKey.SetDestination(address.Get());

    // Create OP_RETURN output with Solana address
    std::string opReturnData = "SOL:" + solanaAddress.toStdString();
    CScript opReturnScript;
    opReturnScript << OP_RETURN << std::vector<unsigned char>(opReturnData.begin(), opReturnData.end());

    // Create transaction
    CWalletTx wtx;

    // Prepare recipients
    std::vector<std::pair<CScript, int64_t> > vecSend;
    vecSend.push_back(std::make_pair(scriptPubKey, nAmount));     // Escrow output
    vecSend.push_back(std::make_pair(opReturnScript, 0));          // OP_RETURN output

    // Create and send transaction
    CReserveKey reservekey(walletModel->getWallet());
    int64_t nFeeRequired = 0;
    int nSplitBlock = 1;

    if (!walletModel->getWallet()->CreateTransaction(vecSend, wtx, reservekey, nFeeRequired, nSplitBlock, NULL)) {
        showErrorMessage(tr("Transaction Error"), tr("Failed to create transaction. Check your balance."));
        return QString();
    }

    if (!walletModel->getWallet()->CommitTransaction(wtx, reservekey)) {
        showErrorMessage(tr("Transaction Error"), tr("Failed to broadcast transaction"));
        return QString();
    }

    return QString::fromStdString(wtx.GetHash().GetHex());
}

void BridgePage::showErrorMessage(const QString &title, const QString &message)
{
    QMessageBox::warning(this, title, message);
}

void BridgePage::showSuccessMessage(const QString &title, const QString &message)
{
    QMessageBox::information(this, title, message);
}
