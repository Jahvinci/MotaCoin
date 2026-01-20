/*
 * Qt4 bitcoin GUI.
 *
 * W.J. van der Laan 2011-2012
 * The Bitcoin Developers 2011-2012
 */
#include "bitcoingui.h"
#include "transactiontablemodel.h"
#include "addressbookpage.h"
#include "sendcoinsdialog.h"
#include "signverifymessagedialog.h"
#include "optionsdialog.h"
#include "aboutdialog.h"
#include "charitydialog.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "editaddressdialog.h"
#include "optionsmodel.h"
#include "transactiondescdialog.h"
#include "addresstablemodel.h"
#include "transactionview.h"
#include "overviewpage.h"
#include "statisticspage.h"
#include "blockbrowser.h"
#include "chatwindow.h"
#include "bridgepage.h"
#include "bitcoinunits.h"
#include "guiconstants.h"
#include "askpassphrasedialog.h"
#include "notificator.h"
#include "guiutil.h"
#include "rpcconsole.h"
#include "wallet.h"
#include "util.h"

#ifdef Q_OS_MAC
#include "macdockiconhandler.h"
#endif

#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QIcon>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QLocale>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QStackedWidget>
#include <QDateTime>
#include <QMovie>
#include <QFileDialog>
#include <QDesktopServices>
#include <QTimer>
#include <QDragEnterEvent>
#include <QUrl>
#include <QStyle>
#include <QFile>
#include <QTextStream>
#include <QSignalMapper>
#include <QSettings>
#include <QTableView>


extern CWallet* pwalletMain;
extern int64_t nLastCoinStakeSearchInterval;
extern unsigned int nTargetSpacing;
double GetPoSKernelPS();

BitcoinGUI::BitcoinGUI(QWidget *parent):
    QMainWindow(parent),
    clientModel(0),
    walletModel(0),
    encryptWalletAction(0),
	unlockWalletAction(0),
    changePassphraseAction(0),
    lockWalletToggleAction(0),
    aboutQtAction(0),
    trayIcon(0),
    notificator(0),
    rpcConsole(0),
    systemDefaultThemeAction(0),
    toolbar(0)
{
    setMinimumSize(870,620);
            resize(870,620);
    // Window title will be set with version info in setClientModel
    setWindowTitle(tr("MotaCoin"));
#ifndef Q_OS_MAC
    qApp->setWindowIcon(QIcon(":icons/bitcoin"));
    setWindowIcon(QIcon(":icons/bitcoin"));
#else
    //setUnifiedTitleAndToolBarOnMac(true);
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif
    // Accept D&D of URIs
    setAcceptDrops(true);

        /* zeewolf: Hot swappable wallet themes */
    // Discover themes
    listThemes(themesList);
    /* /zeewolf: Hot swappable wallet themes */

    // Bridge feature is always enabled
    fBridgeEnabled = true;

    // Create actions for the toolbar, menu bar and tray/dock icon
    createActions();

    // Create application menu bar
    createMenuBar();
    
    // Update theme checkmarks now that menu is created
    updateThemeCheckmarks();

    // Create the toolbars
    createToolBars();

    // Create the tray icon (or setup the dock icon)
    createTrayIcon();
	
    // Create tabs
    overviewPage = new OverviewPage();
	statisticsPage = new StatisticsPage(this);
    chatWindow = new ChatWindow(this);
	blockBrowser = new BlockBrowser(this);
	if (fBridgeEnabled) {
		bridgePage = new BridgePage(this);
	} else {
		bridgePage = 0;
	}

    transactionsPage = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout();

	// export for transactionsPage
	QHBoxLayout *hBoxLayout = new QHBoxLayout();
	historyCopyButton = new QPushButton(tr("&Copy Address"));
	historyExportButton = new QPushButton(tr("&Export"));
	historyQRButton = new QPushButton(tr("Show &QR Code"));
	#ifndef Q_OS_MAC // Icons on push buttons are very uncommon on Mac but ok for others
		historyCopyButton->setIcon(QIcon(":/icons/editcopy"));
		historyExportButton->setIcon(QIcon(":/icons/export"));
	    historyQRButton->setIcon(QIcon(":/icons/qrcode"));
	#endif
		
	hBoxLayout->addWidget(historyCopyButton);
	hBoxLayout->addWidget(historyExportButton);
	#ifdef USE_QRCODE
	hBoxLayout->addWidget(historyQRButton);
	#endif
	hBoxLayout->addStretch();
		
    transactionView = new TransactionView(this);
	connect(historyCopyButton, SIGNAL(released()), transactionView, SLOT(copyAddress()));
	connect(historyExportButton, SIGNAL(released()), transactionView, SLOT(exportClicked()));
	connect(historyQRButton, SIGNAL(released()), transactionView, SLOT(showQRCode()));
	
    vbox->addWidget(transactionView);
	vbox->addLayout(hBoxLayout);
    transactionsPage->setLayout(vbox);

    addressBookPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::SendingTab);

    receiveCoinsPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::ReceivingTab);

    sendCoinsPage = new SendCoinsDialog(this);

    signVerifyMessageDialog = new SignVerifyMessageDialog(this);

    stakeForCharityDialog = new StakeForCharityDialog(this);

    centralWidget = new QStackedWidget(this);
    centralWidget->addWidget(overviewPage);
        centralWidget->addWidget(statisticsPage);
        centralWidget->addWidget(chatWindow);
        centralWidget->addWidget(blockBrowser);
        if (fBridgeEnabled && bridgePage) {
            centralWidget->addWidget(bridgePage);
        }
    centralWidget->addWidget(transactionsPage);
    centralWidget->addWidget(addressBookPage);
    centralWidget->addWidget(receiveCoinsPage);
    centralWidget->addWidget(sendCoinsPage);
    
    centralWidget->addWidget(stakeForCharityDialog);
    
    setCentralWidget(centralWidget);
    
    // Create status bar
    statusBar();

    // Status bar notification icons
    QFrame *frameBlocks = new QFrame();
    frameBlocks->setContentsMargins(0,0,0,0);
    frameBlocks->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QHBoxLayout *frameBlocksLayout = new QHBoxLayout(frameBlocks);
    frameBlocksLayout->setContentsMargins(3,0,3,0);
    frameBlocksLayout->setSpacing(3);
    labelEncryptionIcon = new QLabel();
    labelMintingIcon = new QLabel();
    labelConnectionsIcon = new QLabel();
    labelBlocksIcon = new QLabel();
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelEncryptionIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelMintingIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelConnectionsIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelBlocksIcon);
    frameBlocksLayout->addStretch();

    // Set minting pixmap
    labelMintingIcon->setEnabled(false);
    // Add timer to update minting info
    QTimer *timerMintingIcon = new QTimer(labelMintingIcon);
    timerMintingIcon->start(MODEL_UPDATE_DELAY);
    connect(timerMintingIcon, SIGNAL(timeout()), this, SLOT(updateMintingIcon()));
    // Add timer to update minting weights
    QTimer *timerMintingWeights = new QTimer(labelMintingIcon);
    timerMintingWeights->start(1 * 5000); // 5 second update time
    connect(timerMintingWeights, SIGNAL(timeout()), this, SLOT(updateMintingWeights()));
    // Set initial values for user and network weights
    nWeight = 0;
	nHoursToMaturity = 0;
	nNetworkWeight = 0;
	nAmount = 0;

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(false);
    progressBar = new QProgressBar();
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setVisible(false);

    // Override style sheet for progress bar for styles that have a segmented progress bar,
    // as they make the text unreadable (workaround for issue #1071)
    // See https://qt-project.org/doc/qt-4.8/gallery.html
    QString curStyle = qApp->style()->metaObject()->className();
    if(curStyle == "QWindowsStyle" || curStyle == "QWindowsXPStyle")
    {
        progressBar->setStyleSheet("QProgressBar { background-color: #e8e8e8; border: 1px solid grey; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #FF8000, stop: 1 orange); border-radius: 7px; margin: 0px; }");
    }

    statusBar()->addWidget(progressBarLabel);
    statusBar()->addWidget(progressBar);
    statusBar()->addPermanentWidget(frameBlocks);

    syncIconMovie = new QMovie(":/movies/update_spinner", "mng", this);
    miningIconMovie = new QMovie(":/movies/mining", "mng", this);

    // Clicking on a transaction on the overview page simply sends you to transaction history page
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), this, SLOT(gotoHistoryPage()));
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), transactionView, SLOT(focusTransaction(QModelIndex)));

    // Double-clicking on a transaction on the transaction history page shows details
    connect(transactionView, SIGNAL(doubleClicked(QModelIndex)), transactionView, SLOT(showDetails()));

    rpcConsole = new RPCConsole(this);
    connect(openRPCConsoleAction, SIGNAL(triggered()), rpcConsole, SLOT(show()));

    // Clicking on "Verify Message" in the address book sends you to the verify message tab
    connect(addressBookPage, SIGNAL(verifyMessage(QString)), this, SLOT(gotoVerifyMessageTab(QString)));
    // Clicking on "Sign Message" in the receive coins page sends you to the sign message tab
    connect(receiveCoinsPage, SIGNAL(signMessage(QString)), this, SLOT(gotoSignMessageTab(QString)));

	// Clicking on "Block Browser" in the transaction page sends you to the blockbrowser
	connect(transactionView, SIGNAL(blockBrowserSignal(QString)), this, SLOT(gotoBlockBrowserWithTx(QString)));

    gotoOverviewPage();
}

BitcoinGUI::~BitcoinGUI()
{
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
#ifdef Q_OS_MAC
    delete appMenuBar;
#endif
}

void BitcoinGUI::historySelectionChanged()
{
	// Set button states based on selected tab and selection
	QTableView *table = transactionView->transactionView;
	if (!table->selectionModel()) {
		return;
	}

	if (table->selectionModel()->hasSelection())
	{	
		historyCopyButton->setEnabled(true);
		historyQRButton->setEnabled(true);
	}
	else 
	{
		historyCopyButton->setEnabled(false);
		historyQRButton->setEnabled(false);
	}
}

void BitcoinGUI::createActions()
{
    QActionGroup *tabGroup = new QActionGroup(this);

    overviewAction = new QAction(QIcon(":/icons/overview"), tr("&Overview"), this);
    overviewAction->setToolTip(tr("Show general overview of wallet"));
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);

    statisticsAction = new QAction(QIcon(":/icons/statistics"), tr("&Statistics"), this);
    statisticsAction->setToolTip(tr("View statistics"));
    statisticsAction->setCheckable(true);
    tabGroup->addAction(statisticsAction);

    chatAction = new QAction(QIcon(":/icons/social"), tr("&Social"), this);
    chatAction->setToolTip(tr("View chat, links to social media"));
    chatAction->setCheckable(true);
    tabGroup->addAction(chatAction);

    if (fBridgeEnabled) {
        bridgeAction = new QAction(QIcon(":/icons/bridge"), tr("&Bridge"), this);
        bridgeAction->setToolTip(tr("Bridge MOTA to Solana"));
        bridgeAction->setCheckable(true);
        tabGroup->addAction(bridgeAction);
    } else {
        bridgeAction = 0;
    }

    sendCoinsAction = new QAction(QIcon(":/icons/send"), tr("&Send coins"), this);
    sendCoinsAction->setToolTip(tr("Send coins to a MotaCoin address"));
    sendCoinsAction->setCheckable(true);
    sendCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    tabGroup->addAction(sendCoinsAction);

    receiveCoinsAction = new QAction(QIcon(":/icons/receiving_addresses"), tr("&Receive coins"), this);
    receiveCoinsAction->setToolTip(tr("Show the list of addresses for receiving payments"));
    receiveCoinsAction->setCheckable(true);
    receiveCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    tabGroup->addAction(receiveCoinsAction);

    historyAction = new QAction(QIcon(":/icons/history"), tr("&Transactions"), this);
    historyAction->setToolTip(tr("Browse transaction history"));
    historyAction->setCheckable(true);
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
    tabGroup->addAction(historyAction);

    addressBookAction = new QAction(QIcon(":/icons/address-book"), tr("&Address Book"), this);
    addressBookAction->setToolTip(tr("Edit the list of stored addresses and labels"));
    addressBookAction->setCheckable(true);
    addressBookAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_5));
    tabGroup->addAction(addressBookAction);

    blockAction = new QAction(QIcon(":/icons/block"), tr("&Block Explorer"), this);
    blockAction->setToolTip(tr("Explore the BlockChain"));
    blockAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_6));
    blockAction->setCheckable(true);
    tabGroup->addAction(blockAction);

    connect(blockAction, SIGNAL(triggered()), this, SLOT(gotoBlockBrowser()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(statisticsAction, SIGNAL(triggered()), this, SLOT(gotoStatisticsPage()));
    connect(chatAction, SIGNAL(triggered()), this, SLOT(gotoChatPage()));
    if (fBridgeEnabled && bridgeAction) {
        connect(bridgeAction, SIGNAL(triggered()), this, SLOT(gotoBridgePage()));
    }
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));
    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(gotoAddressBookPage()));

    quitAction = new QAction(QIcon(":/icons/quit"), tr("E&xit"), this);
    quitAction->setToolTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    aboutAction = new QAction(QIcon(":/icons/bitcoin"), tr("&About MotaCoin"), this);
    aboutAction->setToolTip(tr("Show information about MotaCoin"));
    aboutAction->setMenuRole(QAction::AboutRole);

        charityAction = new QAction(QIcon(":/icons/bitcoin"), tr("&Stake For Charity"), this);
    charityAction->setToolTip(tr("Stake For Charity Settings"));
    charityAction->setCheckable(true);
    charityAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_7));
    charityAction->setMenuRole(QAction::AboutRole);
    tabGroup->addAction(charityAction);

    aboutQtAction = new QAction(QIcon(":/trolltech/qmessagebox/images/qtlogo-64.png"), tr("About &Qt"), this);
    aboutQtAction->setToolTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(QIcon(":/icons/options"), tr("&Options..."), this);
    optionsAction->setToolTip(tr("Modify configuration options for MotaCoin"));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    toggleHideAction = new QAction(QIcon(":/icons/bitcoin"), tr("&Show / Hide"), this);
    encryptWalletAction = new QAction(QIcon(":/icons/lock_closed"), tr("&Encrypt Wallet..."), this);
    encryptWalletAction->setToolTip(tr("Encrypt or decrypt wallet"));
    encryptWalletAction->setCheckable(true);
	unlockWalletAction = new QAction(QIcon(":/icons/lock_open"), tr("&Unlock Wallet For PoS..."), this);
	unlockWalletAction->setStatusTip(tr("Unlock the wallet for PoS"));
	unlockWalletAction->setCheckable(true);
    backupWalletAction = new QAction(QIcon(":/icons/filesave"), tr("&Backup Wallet..."), this);
    backupWalletAction->setToolTip(tr("Backup wallet to another location"));
    changePassphraseAction = new QAction(QIcon(":/icons/key"), tr("&Change Passphrase..."), this);
    changePassphraseAction->setToolTip(tr("Change the passphrase used for wallet encryption"));
    lockWalletToggleAction = new QAction(this);
    signMessageAction = new QAction(QIcon(":/icons/edit"), tr("Sign &message..."), this);
    verifyMessageAction = new QAction(QIcon(":/icons/transaction_0"), tr("&Verify message..."), this);
        checkWalletAction = new QAction(QIcon(":/icons/transaction_confirmed"), tr("&Check Wallet..."), this);
        checkWalletAction->setStatusTip(tr("Check wallet integrity and report findings"));
        repairWalletAction = new QAction(QIcon(":/icons/options"), tr("&Repair Wallet..."), this);
        repairWalletAction->setStatusTip(tr("Fix wallet integrity and remove orphans"));
    exportAction = new QAction(QIcon(":/icons/export"), tr("&Export..."), this);
    exportAction->setToolTip(tr("Export the data in the current tab to a file"));
    openRPCConsoleAction = new QAction(QIcon(":/icons/debugwindow"), tr("&Debug window"), this);
    openRPCConsoleAction->setToolTip(tr("Open debugging and diagnostic console"));

    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
        connect(charityAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
        connect(charityAction, SIGNAL(triggered()), this, SLOT(charityClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    connect(encryptWalletAction, SIGNAL(triggered(bool)), this, SLOT(encryptWallet(bool)));
        connect(checkWalletAction, SIGNAL(triggered()), this, SLOT(checkWallet()));
        connect(repairWalletAction, SIGNAL(triggered()), this, SLOT(repairWallet()));
    connect(backupWalletAction, SIGNAL(triggered()), this, SLOT(backupWallet()));
    connect(changePassphraseAction, SIGNAL(triggered()), this, SLOT(changePassphrase()));
    connect(lockWalletToggleAction, SIGNAL(triggered()), this, SLOT(lockWalletToggle()));
    connect(signMessageAction, SIGNAL(triggered()), this, SLOT(gotoSignMessageTab()));
    connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(gotoVerifyMessageTab()));
	connect(unlockWalletAction, SIGNAL(triggered()), this, SLOT(unlockWalletForMint()));

        /* zeewolf: Hot swappable wallet themes */
    // Create action group for exclusive theme selection with checkmarks
    QActionGroup* themeActionGroup = new QActionGroup(this);
    themeActionGroup->setExclusive(true);
    
    // Add "Default" action (which means System Default, no theme)
    systemDefaultThemeAction = new QAction(QIcon(), tr("Default"), this);
    systemDefaultThemeAction->setCheckable(true);
    systemDefaultThemeAction->setActionGroup(themeActionGroup);
    systemDefaultThemeAction->setToolTip(tr("Use system default Qt styling"));
    systemDefaultThemeAction->setStatusTip(tr("Default theme (system default)"));
    QSignalMapper* systemDefaultMapper = new QSignalMapper(this);
    systemDefaultMapper->setMapping(systemDefaultThemeAction, QString(""));
    connect(systemDefaultThemeAction, SIGNAL(triggered()), systemDefaultMapper, SLOT(map()));
    connect(systemDefaultMapper, SIGNAL(mapped(QString)), this, SLOT(changeTheme(QString)));
    
    // Add custom themes (themes directory)
    if (themesList.count()>0)
    {
        QSignalMapper* signalMapper = new QSignalMapper (this) ;

        for( int i=0; i < themesList.count(); i++ )
        {
            QString theme=themesList[i];
            customActions[i] = new QAction(QIcon(), theme, this);
            customActions[i]->setCheckable(true);
            customActions[i]->setActionGroup(themeActionGroup);
            customActions[i]->setToolTip(QString("Switch to " + theme + " theme"));
            customActions[i]->setStatusTip(QString("Switch to " + theme + " theme"));
            signalMapper->setMapping(customActions[i], theme);
            connect(customActions[i], SIGNAL(triggered()), signalMapper, SLOT (map()));
        }
        connect(signalMapper, SIGNAL(mapped(QString)), this, SLOT(changeTheme(QString)));
    }
    /* zeewolf: Hot swappable wallet themes */
}

void BitcoinGUI::createMenuBar()
{
#ifdef Q_OS_MAC
    // Create a decoupled menu bar on Mac which stays even if the window is closed
    appMenuBar = new QMenuBar();
#else
    // Get the main window's menu bar on other platforms
    appMenuBar = menuBar();
#endif

    // Configure the menus
    QMenu *file = appMenuBar->addMenu(tr("&File"));
    file->addAction(backupWalletAction);
    file->addAction(exportAction);
    file->addAction(signMessageAction);
    file->addAction(verifyMessageAction);
    
    // Add theme selection actions
    file->addSeparator();
    // Add "Default" (which maps to System Default) as first option
    file->addAction(systemDefaultThemeAction);
    
    // Add other custom themes (skip "Default" directory theme)
    if (themesList.count() > 0) {
        for (int i = 0; i < themesList.count(); i++) {
            // Skip "Default" directory since we use System Default instead
            if (customActions[i] && themesList[i] != "Default") {
                file->addAction(customActions[i]);
            }
        }
    }
    
    file->addSeparator();
    file->addAction(quitAction);

    QMenu *settings = appMenuBar->addMenu(tr("&Tools"));
    settings->addAction(encryptWalletAction);
	settings->addAction(lockWalletToggleAction);
	settings->addAction(unlockWalletAction);
    settings->addAction(changePassphraseAction);
        settings->addAction(checkWalletAction);
        settings->addAction(repairWalletAction);
        settings->addAction(charityAction);
    settings->addSeparator();
    settings->addAction(optionsAction);

    /* zeewolf: Hot swappable wallet themes */

    QMenu *help = appMenuBar->addMenu(tr("&Help"));
    help->addAction(openRPCConsoleAction);
    help->addSeparator();
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
}

void BitcoinGUI::createToolBars()
{
    toolbar = addToolBar(tr("Tabs toolbar"));
    toolbar->setObjectName("toolbar");
    addToolBar(Qt::LeftToolBarArea,toolbar);
    toolbar->setOrientation(Qt::Vertical);
    toolbar->setMovable( false );
//    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolbar->addAction(overviewAction);
    toolbar->addAction(sendCoinsAction);
    toolbar->addAction(receiveCoinsAction);
    toolbar->addAction(historyAction);
//	toolbar->addAction(exportAction);
    toolbar->addAction(addressBookAction);
    toolbar->addAction(blockAction);
    toolbar->addAction(statisticsAction);
    toolbar->addAction(chatAction);
    if (fBridgeEnabled && bridgeAction) {
        toolbar->addAction(bridgeAction);
    }
#ifdef Q_OS_MAC
    toolbar->setMovable(false);
#endif

  //  QToolBar *toolbar2 = addToolBar(tr("Actions toolbar"));
  //  toolbar2->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
   // toolbar2->addAction(lockWalletToggleAction);
 //   toolbar2->addAction(exportAction);
#ifdef Q_OS_MAC
 //   toolbar2->setMovable(false);
#endif
}

void BitcoinGUI::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    if(clientModel)
    {
        // Set window title with version information (compact format to save space)
        QString versionString = clientModel->formatFullVersion();
        QString baseTitle = tr("MotaCoin");
        if(clientModel->isTestNet())
        {
            setWindowTitle(QString("%1 %2 [testnet]").arg(baseTitle).arg(versionString));
#ifndef Q_OS_MAC
            qApp->setWindowIcon(QIcon(":icons/bitcoin_testnet"));
            setWindowIcon(QIcon(":icons/bitcoin_testnet"));
#else
            MacDockIconHandler::instance()->setIcon(QIcon(":icons/bitcoin_testnet"));
#endif
            if(trayIcon)
            {
                trayIcon->setToolTip(tr("MotaCoin client") + QString(" ") + tr("[testnet]"));
                trayIcon->setIcon(QIcon(":/icons/toolbar_testnet"));
                toggleHideAction->setIcon(QIcon(":/icons/toolbar_testnet"));
            }

            aboutAction->setIcon(QIcon(":/icons/toolbar_testnet"));
        }
        else
        {
            setWindowTitle(QString("%1 %2").arg(baseTitle).arg(versionString));
        }

        // Keep up to date with client
        setNumConnections(clientModel->getNumConnections());
        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

        setNumBlocks(clientModel->getNumBlocks(), clientModel->getNumBlocksOfPeers());
        connect(clientModel, SIGNAL(numBlocksChanged(int,int)), this, SLOT(setNumBlocks(int,int)));

        // Report errors from network/worker thread
        connect(clientModel, SIGNAL(error(QString,QString,bool)), this, SLOT(error(QString,QString,bool)));

        rpcConsole->setClientModel(clientModel);
        addressBookPage->setOptionsModel(clientModel->getOptionsModel());
        receiveCoinsPage->setOptionsModel(clientModel->getOptionsModel());
    }
}

void BitcoinGUI::setWalletModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;
    if(walletModel)
    {
        // Report errors from wallet thread
        connect(walletModel, SIGNAL(error(QString,QString,bool)), this, SLOT(error(QString,QString,bool)));

        // Put transaction list in tabs
        transactionView->setModel(walletModel);
        
        // Connect selection changed signal after model is set
        if (transactionView->transactionView && transactionView->transactionView->selectionModel()) {
            connect(transactionView->transactionView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                    this, SLOT(historySelectionChanged()));
            historySelectionChanged();
        }

        overviewPage->setModel(walletModel);
        addressBookPage->setModel(walletModel->getAddressTableModel());
        receiveCoinsPage->setModel(walletModel->getAddressTableModel());
        sendCoinsPage->setModel(walletModel);
        signVerifyMessageDialog->setModel(walletModel);
                statisticsPage->setModel(clientModel);
                chatWindow->setModel(clientModel);
                blockBrowser->setModel(clientModel);
                if (fBridgeEnabled && bridgePage) {
                    bridgePage->setWalletModel(walletModel);
                    bridgePage->setClientModel(clientModel);
                }
                if (stakeForCharityDialog)
                    stakeForCharityDialog->setModel(walletModel);

        setEncryptionStatus(walletModel->getEncryptionStatus());
        connect(walletModel, SIGNAL(encryptionStatusChanged(int)), this, SLOT(setEncryptionStatus(int)));

        // Balloon pop-up for new transaction
        connect(walletModel->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(incomingTransaction(QModelIndex,int,int)));

        // Ask for passphrase if needed
        connect(walletModel, SIGNAL(requireUnlock()), this, SLOT(unlockWallet()));
    }
}

void BitcoinGUI::createTrayIcon()
{
    QMenu *trayIconMenu;
#ifndef Q_OS_MAC
    trayIcon = new QSystemTrayIcon(this);
    trayIconMenu = new QMenu(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setToolTip(tr("MotaCoin client"));
    trayIcon->setIcon(QIcon(":/icons/toolbar"));
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon->show();
#else
    // Note: On Mac, the dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    dockIconHandler->setMainWindow((QMainWindow *)this);
    trayIconMenu = dockIconHandler->dockMenu();
#endif

    // Configuration of the tray icon (or dock icon) icon menu
    trayIconMenu->addAction(toggleHideAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(sendCoinsAction);
    trayIconMenu->addAction(receiveCoinsAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(signMessageAction);
    trayIconMenu->addAction(verifyMessageAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addAction(openRPCConsoleAction);
#ifndef Q_OS_MAC // This is built-in on Mac
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
#endif

    notificator = new Notificator(qApp->applicationName(), trayIcon);
}

#ifndef Q_OS_MAC
void BitcoinGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers show/hide of the main window
        toggleHideAction->trigger();
    }
}
#endif

void BitcoinGUI::lockIconClicked()
{
    if(!walletModel)
        return;

    if(walletModel->getEncryptionStatus() == WalletModel::Locked)
        unlockWalletForMint();
}

void BitcoinGUI::optionsClicked()
{
    if(!clientModel || !clientModel->getOptionsModel())
        return;
    OptionsDialog dlg;
    dlg.setModel(clientModel->getOptionsModel());
    dlg.exec();
}

void BitcoinGUI::aboutClicked()
{
    AboutDialog dlg;
    dlg.setModel(clientModel);
    dlg.exec();
}

void BitcoinGUI::setNumConnections(int count)
{
    QString icon;
    switch(count)
    {
    case 0: icon = ":/icons/connect_0"; break;
    case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
    case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
    case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
    default: icon = ":/icons/connect_4"; break;
    }
    labelConnectionsIcon->setPixmap(QIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    labelConnectionsIcon->setToolTip(tr("%n active connection(s) to MotaCoin network", "", count));
}

void BitcoinGUI::setNumBlocks(int count, int nTotalBlocks)
{
    // don't show progress bar if we have no connection to the network, but show block height
    if (!clientModel)
    {
        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);
        return;
    }
    
    if (clientModel->getNumConnections() == 0)
    {
        // Show block height even when no connections
        progressBarLabel->setText(tr("Block: %1 | Connections: 0").arg(count));
        progressBarLabel->setVisible(true);
        progressBar->setVisible(false);
        return;
    }

    QString strStatusBarWarnings = clientModel->getStatusBarWarnings();
    QString tooltip;

    if(count < nTotalBlocks)
    {
        int nRemainingBlocks = nTotalBlocks - count;
        float nPercentageDone = count / (nTotalBlocks * 0.01f);

        if (strStatusBarWarnings.isEmpty())
        {
            progressBarLabel->setText(tr("Synchronizing with network..."));
            progressBarLabel->setVisible(true);
            progressBar->setFormat(tr("~%n block(s) remaining", "", nRemainingBlocks));
            progressBar->setMaximum(nTotalBlocks);
            progressBar->setValue(count);
            progressBar->setVisible(true);
        }

        tooltip = tr("Downloaded %1 of %2 blocks of transaction history (%3% done).").arg(count).arg(nTotalBlocks).arg(nPercentageDone, 0, 'f', 2);
    }
    else
    {
        if (strStatusBarWarnings.isEmpty())
        {
            // Show block height and connections when not synchronizing
            int numConnections = clientModel->getNumConnections();
            progressBarLabel->setText(tr("Block: %1 | Connections: %2").arg(count).arg(numConnections));
            progressBarLabel->setVisible(true);
        }
        else
        {
            progressBarLabel->setVisible(false);
        }

        progressBar->setVisible(false);
        tooltip = tr("Downloaded %1 blocks of transaction history.").arg(count);
    }

    // Override progressBarLabel text and hide progress bar, when we have warnings to display
    if (!strStatusBarWarnings.isEmpty())
    {
        progressBarLabel->setText(strStatusBarWarnings);
        progressBarLabel->setVisible(true);
        progressBar->setVisible(false);
    }

    tooltip = tr("Current difficulty is %1.").arg(clientModel->GetDifficulty()) + QString("<br>") + tooltip;

    QDateTime lastBlockDate = clientModel->getLastBlockDate();
    int secs = lastBlockDate.secsTo(QDateTime::currentDateTime());
    QString text;

    // Represent time from last generated block in human readable text
    if(secs <= 0)
    {
        // Fully up to date. Leave text empty.
    }
    else if(secs < 60)
    {
        text = tr("%n second(s) ago","",secs);
    }
    else if(secs < 60*60)
    {
        text = tr("%n minute(s) ago","",secs/60);
    }
    else if(secs < 24*60*60)
    {
        text = tr("%n hour(s) ago","",secs/(60*60));
    }
    else
    {
        text = tr("%n day(s) ago","",secs/(60*60*24));
    }

    // Set icon state: spinning if catching up, tick otherwise
    if(count >= nTotalBlocks)
    {
        tooltip = tr("Up to date") + QString(".<br>") + tooltip;
        labelBlocksIcon->setPixmap(QIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));

        overviewPage->showOutOfSyncWarning(false);
    }
    else
    {
        tooltip = tr("Catching up...") + QString("<br>") + tooltip;
        labelBlocksIcon->setMovie(syncIconMovie);
        syncIconMovie->start();

        overviewPage->showOutOfSyncWarning(true);
    }

    if(!text.isEmpty())
    {
        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1.").arg(text);
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void BitcoinGUI::error(const QString &title, const QString &message, bool modal)
{
    // Report errors from network/worker thread
    if(modal)
    {
        QMessageBox::critical(this, title, message, QMessageBox::Ok, QMessageBox::Ok);
    } else {
        notificator->notify(Notificator::Critical, title, message);
    }
}

void BitcoinGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MAC // Ignored on Mac
    if(e->type() == QEvent::WindowStateChange)
    {
        if(clientModel && clientModel->getOptionsModel()->getMinimizeToTray())
        {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);
            if(!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized())
            {
                QTimer::singleShot(0, this, SLOT(hide()));
                e->ignore();
            }
        }
    }
#endif
}

void BitcoinGUI::closeEvent(QCloseEvent *event)
{
    if(clientModel)
    {
#ifndef Q_OS_MAC // Ignored on Mac
        if(!clientModel->getOptionsModel()->getMinimizeToTray() &&
           !clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            qApp->quit();
        }
#endif
    }
    QMainWindow::closeEvent(event);
}

void BitcoinGUI::askFee(qint64 nFeeRequired, bool *payFee)
{
    QString strMessage =
        tr("This transaction is over the size limit.  You can still send it for a fee of %1, "
          "which goes to the nodes that process your transaction and helps to support the network.  "
          "Do you want to pay the fee?").arg(
                BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, nFeeRequired));
    QMessageBox::StandardButton retval = QMessageBox::question(
          this, tr("Confirm transaction fee"), strMessage,
          QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
    *payFee = (retval == QMessageBox::Yes);
}

void BitcoinGUI::incomingTransaction(const QModelIndex & parent, int start, int end)
{
    if(!walletModel || !clientModel)
        return;
    TransactionTableModel *ttm = walletModel->getTransactionTableModel();
    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent)
                    .data(Qt::EditRole).toULongLong();
    if(!clientModel->inInitialBlockDownload())
    {
        fMultiSendNotify = pwalletMain->fMultiSendNotify;

        // On new transaction, make an info balloon
        // Unless the initial block download is in progress, to prevent balloon-spam
        QString date = ttm->index(start, TransactionTableModel::Date, parent)
                        .data().toString();
        QString type = ttm->index(start, TransactionTableModel::Type, parent)
                        .data().toString();
        QString address = ttm->index(start, TransactionTableModel::ToAddress, parent)
                        .data().toString();
        QIcon icon = qvariant_cast<QIcon>(ttm->index(start,
                            TransactionTableModel::ToAddress, parent)
                        .data(Qt::DecorationRole));

        notificator->notify(Notificator::Information,
                            (amount)<0 ? (fMultiSendNotify == true ? tr("Sent MultiSend transaction") : tr("Sent transaction") ):
                                         tr("Incoming transaction"),
                              tr("Date: %1\n"
                                 "Amount: %2\n"
                                 "Type: %3\n"
                                 "Address: %4\n")
                              .arg(date)
                              .arg(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), amount, true))
                              .arg(type)
                              .arg(address), icon);
			
		pwalletMain->fMultiSendNotify = false;
	
    }
}

void BitcoinGUI::gotoOverviewPage()
{
    overviewAction->setChecked(true);
    centralWidget->setCurrentWidget(overviewPage);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::gotoBlockBrowser()
{
    blockAction->setChecked(true);
    centralWidget->setCurrentWidget(blockBrowser);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::gotoBlockBrowserWithTx(QString txid)
{
    blockAction->setChecked(true);
    centralWidget->setCurrentWidget(blockBrowser);

	int pos = txid.lastIndexOf(QChar('-'));
	
	blockBrowser->setTx(txid.left(pos));

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::gotoStatisticsPage()
{
    statisticsAction->setChecked(true);
    centralWidget->setCurrentWidget(statisticsPage);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::gotoChatPage()
{
    chatAction->setChecked(true);
    centralWidget->setCurrentWidget(chatWindow);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::gotoBridgePage()
{
    if (!fBridgeEnabled || !bridgePage || !bridgeAction) {
        return;
    }
    bridgeAction->setChecked(true);
    centralWidget->setCurrentWidget(bridgePage);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::gotoHistoryPage()
{
    historyAction->setChecked(true);
    centralWidget->setCurrentWidget(transactionsPage);

	// exportAction->setEnabled(false);
    // disconnect(exportAction, SIGNAL(triggered()), 0, 0);

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), transactionView, SLOT(exportClicked()));
}

void BitcoinGUI::gotoAddressBookPage()
{
    addressBookAction->setChecked(true);
    centralWidget->setCurrentWidget(addressBookPage);

	// exportAction->setEnabled(false);
    // disconnect(exportAction, SIGNAL(triggered()), 0, 0);

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), addressBookPage, SLOT(exportClicked()));
}

void BitcoinGUI::gotoReceiveCoinsPage()
{
    receiveCoinsAction->setChecked(true);
    centralWidget->setCurrentWidget(receiveCoinsPage);

	// exportAction->setEnabled(false);
    // disconnect(exportAction, SIGNAL(triggered()), 0, 0);

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), receiveCoinsPage, SLOT(exportClicked()));
}

void BitcoinGUI::gotoSendCoinsPage()
{
    sendCoinsAction->setChecked(true);
    centralWidget->setCurrentWidget(sendCoinsPage);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::gotoSignMessageTab(QString addr)
{
    // call show() in showTab_SM()
    signVerifyMessageDialog->showTab_SM(true);

    if(!addr.isEmpty())
        signVerifyMessageDialog->setAddress_SM(addr);
}

void BitcoinGUI::gotoVerifyMessageTab(QString addr)
{
    // call show() in showTab_VM()
    signVerifyMessageDialog->showTab_VM(true);

    if(!addr.isEmpty())
        signVerifyMessageDialog->setAddress_VM(addr);
}

void BitcoinGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void BitcoinGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        int nValidUrisFound = 0;
        QList<QUrl> uris = event->mimeData()->urls();
        Q_FOREACH(const QUrl &uri, uris)
        {
            if (sendCoinsPage->handleURI(uri.toString()))
                nValidUrisFound++;
        }

        // if valid URIs were found
        if (nValidUrisFound)
            gotoSendCoinsPage();
        else
            notificator->notify(Notificator::Warning, tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid MotaCoin address or malformed URI parameters."));
    }

    event->acceptProposedAction();
}

void BitcoinGUI::handleURI(QString strURI)
{
    // URI has to be valid
    if (sendCoinsPage->handleURI(strURI))
    {
        showNormalIfMinimized();
        gotoSendCoinsPage();
    }
    else
        notificator->notify(Notificator::Warning, tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid MotaCoin address or malformed URI parameters."));
}

void BitcoinGUI::setEncryptionStatus(int status)
{
    switch(status)
    {
    case WalletModel::Unencrypted:
        labelEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        encryptWalletAction->setEnabled(true);
		labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        changePassphraseAction->setEnabled(false);
        lockWalletToggleAction->setVisible(false);
		unlockWalletAction->setChecked(false);
        break;
    case WalletModel::Unlocked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        encryptWalletAction->setChecked(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
		unlockWalletAction->setChecked(false);
		unlockWalletAction->setEnabled(false);
        changePassphraseAction->setEnabled(true);
        lockWalletToggleAction->setVisible(true);
        lockWalletToggleAction->setIcon(QIcon(":/icons/lock_closed"));
        lockWalletToggleAction->setText(tr("&Lock Wallet"));
        lockWalletToggleAction->setToolTip(tr("Lock wallet"));
        break;
    case WalletModel::Locked:
        labelEncryptionIcon->hide();
        encryptWalletAction->setChecked(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
		unlockWalletAction->setChecked(false);
		unlockWalletAction->setEnabled(true);
        changePassphraseAction->setEnabled(true);
        lockWalletToggleAction->setVisible(true);
        lockWalletToggleAction->setIcon(QIcon(":/icons/lock_open"));
        lockWalletToggleAction->setText(tr("&Unlock Wallet..."));
        lockWalletToggleAction->setToolTip(tr("Unlock wallet"));
        break;
    }
}

void BitcoinGUI::encryptWallet(bool status)
{
    if(!walletModel)
        return;
    AskPassphraseDialog dlg(status ? AskPassphraseDialog::Encrypt:
                                     AskPassphraseDialog::Decrypt, this);
    dlg.setModel(walletModel);
    dlg.exec();

    setEncryptionStatus(walletModel->getEncryptionStatus());
}

void BitcoinGUI::checkWallet()
{

    int nMismatchSpent;
    int64_t nBalanceInQuestion;
    int nOrphansFound;

    if(!walletModel)
        return;

    // Check the wallet as requested by user
    walletModel->checkWallet(nMismatchSpent, nBalanceInQuestion, nOrphansFound);

    if (nMismatchSpent == 0 && nOrphansFound == 0)
        notificator->notify(Notificator::Warning,
                tr("Check Wallet Information"),
                tr("Wallet passed integrity test!\n"
                   "Nothing found to fix."));
  else
                notificator->notify(Notificator::Warning, //tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid MotaCoin address or malformed URI parameters."));
                        tr("Check Wallet Information"), tr("Wallet failed integrity test!\n\n"
                  "Mismatched coin(s) found: %1.\n"
                  "Amount in question: %2.\n"
                  "Orphans found: %3.\n\n"
                  "Please backup wallet and run repair wallet.\n")
                                                .arg(nMismatchSpent)
                        .arg(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), nBalanceInQuestion,true))
                        .arg(nOrphansFound));
}

void BitcoinGUI::repairWallet()
{
    int nMismatchSpent;
    int64_t nBalanceInQuestion;
    int nOrphansFound;

    if(!walletModel)
        return;

    // Repair the wallet as requested by user
    walletModel->repairWallet(nMismatchSpent, nBalanceInQuestion, nOrphansFound);

    if (nMismatchSpent == 0 && nOrphansFound == 0)
       notificator->notify(Notificator::Warning,
           tr("Repair Wallet Information"),
               tr("Wallet passed integrity test!\n"
                  "Nothing found to fix."));
    else
                notificator->notify(Notificator::Warning,
                tr("Repair Wallet Information"),
               tr("Wallet failed integrity test and has been repaired!\n"
                  "Mismatched coin(s) found: %1\n"
                  "Amount affected by repair: %2\n"
                  "Orphans removed: %3\n")
                        .arg(nMismatchSpent)
                        .arg(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), nBalanceInQuestion,true))
                        .arg(nOrphansFound));
}

void BitcoinGUI::backupWallet()
{
    QString saveDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString filename = QFileDialog::getSaveFileName(this, tr("Backup Wallet"), saveDir, tr("Wallet Data (*.dat)"));
    if(!filename.isEmpty()) {
        if(!walletModel->backupWallet(filename)) {
            QMessageBox::warning(this, tr("Backup Failed"), tr("There was an error trying to save the wallet data to the new location."));
        }
    }
}

void BitcoinGUI::changePassphrase()
{
    AskPassphraseDialog dlg(AskPassphraseDialog::ChangePass, this);
    dlg.setModel(walletModel);
    dlg.exec();
}

void BitcoinGUI::lockWalletToggle()
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model
    if(walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
		AskPassphraseDialog dlg(AskPassphraseDialog::Unlock, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
    else
        walletModel->setWalletLocked(true);
}

void BitcoinGUI::unlockWallet()
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model
    if(walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog dlg(AskPassphraseDialog::Unlock, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
}

void BitcoinGUI::unlockWalletForMint()
{
    if(!walletModel)
        return;

    // Unlock wallet when requested by user
    if(walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog dlg(AskPassphraseDialog::UnlockForMint, this);
        dlg.setModel(walletModel);
        dlg.exec();
		
		// Only show message if unlock is sucessfull.
		if(walletModel->getEncryptionStatus() == WalletModel::Unlocked)
		notificator->notify(Notificator::Warning,
			tr("Unlock Wallet Information"),
                tr("Wallet has been unlocked. \n"
					"Proof of Stake has started.\n"));
    }
}

void BitcoinGUI::lockWallet()
{
    if(!walletModel)
       return;

    // Lock wallet when requested by user
    if(walletModel->getEncryptionStatus() == WalletModel::Unlocked)
         walletModel->setWalletLocked(true,"",true);
	notificator->notify(Notificator::Warning,
			tr("Lock Wallet Information"),
                tr("Wallet has been unlocked. \n"
					"Proof of Stake has stopped.\n"));
}

void BitcoinGUI::showNormalIfMinimized(bool fToggleHidden)
{
    // activateWindow() (sometimes) helps with keyboard focus on Windows
    if (isHidden())
    {
        show();
        activateWindow();
    }
    else if (isMinimized())
    {
        showNormal();
        activateWindow();
    }
    else if (GUIUtil::isObscured(this))
    {
        raise();
        activateWindow();
    }
    else if(fToggleHidden)
        hide();
}

void BitcoinGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

void BitcoinGUI::updateMintingIcon()
{
    if (pwalletMain && pwalletMain->IsLocked())
    {
        labelMintingIcon->setToolTip(tr("Not minting because wallet is locked.<br>Network weight is %1.<br>Charity: %2").arg(nNetworkWeight).arg(pwalletMain->fStakeForCharity ? tr("Active"):tr("Not Active")));
        labelMintingIcon->setEnabled(false);
        labelMintingIcon->setPixmap(QIcon(":/icons/staking_off").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
    }
    else if (vNodes.empty())
    {
        labelMintingIcon->setToolTip(tr("Not minting because wallet is offline.<br>Network weight is %1.<br>Charity: %2").arg(nNetworkWeight).arg(pwalletMain && pwalletMain->fStakeForCharity ? tr("Active"):tr("Not Active")));
        labelMintingIcon->setEnabled(false);
        labelMintingIcon->setPixmap(QIcon(":/icons/staking_off").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
    }
    else if (IsInitialBlockDownload())
    {
        labelMintingIcon->setToolTip(tr("Not minting because wallet is syncing.<br>Network weight is %1.<br>Charity: %2").arg(nNetworkWeight).arg(pwalletMain && pwalletMain->fStakeForCharity ? tr("Active"):tr("Not Active")));
        labelMintingIcon->setEnabled(false);
        labelMintingIcon->setPixmap(QIcon(":/icons/staking_off").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
    }
    else if (!nWeight)
    {
        labelMintingIcon->setToolTip(tr("Not minting because you don't have mature coins.<br>Next block matures in %2 hours<br>Network weight is %1<br>Charity: %3").arg(nNetworkWeight).arg(nHoursToMaturity).arg(pwalletMain && pwalletMain->fStakeForCharity ? tr("Active"):tr("Not Active")));
        labelMintingIcon->setEnabled(false);
        labelMintingIcon->setPixmap(QIcon(":/icons/staking_off").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
    }
    else if (nLastCoinStakeSearchInterval)
    {	
		uint64_t nAccuracyAdjustment = 1; // this is a manual adjustment param if needed to make more accurate
        uint64_t nEstimateTime = nTargetSpacing * nNetworkWeight / nWeight / nAccuracyAdjustment;
	
		uint64_t nRangeLow = nEstimateTime;
		uint64_t nRangeHigh = nEstimateTime * 1.5;

        QString text;
        if (nEstimateTime < 60)
        {
            text = tr("%1 - %2 seconds").arg(nRangeLow).arg(nRangeHigh);
        }
        else if (nEstimateTime < 60*60)
        {
            text = tr("%1 - %2 minutes").arg(nRangeLow / 60).arg(nRangeHigh / 60);
        }
        else if (nEstimateTime < 24*60*60)
        {
            text = tr("%1 - %2 hours").arg(nRangeLow / (60*60)).arg(nRangeHigh / (60*60));
        }
        else
        {
            text = tr("%1 - %2 days").arg(nRangeLow / (60*60*24)).arg(nRangeHigh / (60*60*24));
        }

        labelMintingIcon->setPixmap(QIcon(":/icons/staking_on").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelMintingIcon->setEnabled(true);
        labelMintingIcon->setToolTip(tr("Minting.<br>Your weight is %1.<br>Network weight is %2.<br><b>Estimated</b> next stake in %3.<br>Charity: %4").arg(nWeight).arg(nNetworkWeight).arg(text).arg(pwalletMain && pwalletMain->fStakeForCharity ? tr("Active"):tr("Not Active")));
    }
    else
    {
        labelMintingIcon->setToolTip(tr("Not minting."));
        labelMintingIcon->setEnabled(false);
        labelMintingIcon->setPixmap(QIcon(":/icons/staking_off").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
    }
}

void BitcoinGUI::updateMintingWeights()
{
    // Only update if we have the network's current number of blocks, or weight(s) are zero (fixes lagging GUI)
    if ((clientModel && clientModel->getNumBlocks() >= clientModel->getNumBlocksOfPeers()) || !nWeight || !nNetworkWeight)
    {
        nWeight = 0;
		
		nAmount = 0;
		
        if (pwalletMain)
			pwalletMain->GetStakeWeight2(*pwalletMain, nMinMax, nMinMax, nWeight, nHoursToMaturity, nAmount);
		
		if (nHoursToMaturity > 48)
			nHoursToMaturity = 0;
        nNetworkWeight = GetPoSKernelPS();
    }
	
	//MultiSend check
	if(walletModel)
	{
		fMultiSend = pwalletMain->fMultiSend;
	}
		
}

void BitcoinGUI::charityClicked()
{
    charityAction->setChecked(true);
    
    centralWidget->setCurrentWidget(stakeForCharityDialog);
    
    // Trigger widget creation when user actually opens the charity dialog
    // This ensures widgets are only created when needed, after wallet loading
    if (stakeForCharityDialog && walletModel)
    {
        stakeForCharityDialog->refreshTable();
    }

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}
/* zeewolf: Hot swappable wallet themes */
void BitcoinGUI::changeTheme(QString theme)
{
    // If theme is empty, just clear stylesheet (system default)
    // Otherwise load the selected theme
    loadTheme(theme);
}

void BitcoinGUI::loadTheme(QString theme)
{
    // template variables : key => value
    QMap<QString, QString> variables;

    // path to selected theme dir - for simpler use, just use $theme-dir in qss : url($theme-dir/image.png)
    QString themeDir = themesDir + "/" + theme;

    // if theme selected
    if (theme != "") {
        QFile qss(themeDir + "/styles.qss");
        // open qss
        if (qss.open(QFile::ReadOnly))
        {
            // read stylesheet
            QString styleSheet = QString(qss.readAll());
            QTextStream in(&qss);
            // rewind
            in.seek(0);
            bool readingVariables = false;

            // seek for variables
            while(!in.atEnd()) {
                QString line = in.readLine();
                // variables starts here
                if (line == "/** [VARS]") {
                    readingVariables = true;
                }
                // variables end here
                if (line == "[/VARS] */") {
                    break;
                }
                // if we're reading variables - store them in a map
                if (readingVariables == true) {
                    // skip empty lines
                    if (line.length()>3 && line.contains('=')) {
                        QStringList fields = line.split("=");
                        QString var = fields.at(0).trimmed();
                        QString value = fields.at(1).trimmed();
                        variables[var] = value;
                    }
                }
            }

            // replace path to themes dir
            styleSheet.replace("$theme-dir", themeDir);
            styleSheet.replace("$themes-dir", themesDir);

            QMapIterator<QString, QString> variable(variables);
            variable.toBack();
            // iterate backwards to prevent overwriting variables
            while (variable.hasPrevious()) {
                variable.previous();
                // replace variables
                styleSheet.replace(variable.key(), variable.value());
            }

            qss.close();

            // Apply the result qss file to Qt

            /*if (styleSheet.contains("$", Qt::CaseInsensitive)) {
                QRegExp rx("(\\$[-\\w]+)");
                rx.indexIn(styleSheet);
                QString captured = rx.cap(1);
                QMessageBox::warning(this, "Theme syntax error", "You have used variable that is not declared " + captured + ". Theme will not be applied.");
            } else {*/
                qApp->setStyleSheet(styleSheet);
            /*}*/
        }
    } else {
        // If not theme name given - clear styles (System Default)
        qApp->setStyleSheet(QString(""));
        // Reset toolbar button style to default (TextUnderIcon) for System Default theme
        if (toolbar) {
            toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        }
    }

    // set selected theme and store it in registry
    selectedTheme = theme;
    QSettings settings;
    settings.setValue("Template", selectedTheme);
    
    // Update checkmarks on theme actions (only if menu is already created)
    if (systemDefaultThemeAction) {
        updateThemeCheckmarks();
    }
}

void BitcoinGUI::listThemes(QStringList& themes)
{
    QDir currentDir(qApp->applicationDirPath());
    QDir themesDirCandidate;
    bool found = false;
    
    // try app dir (for packaged/installed version)
    if (currentDir.cd("themes")) {
        themesDirCandidate = currentDir;
        found = true;
    } 
    // try src/qt/res/themes (for development builds)
    else {
        currentDir = QDir(qApp->applicationDirPath());
        if (currentDir.cd("src/qt/res/themes")) {
            themesDirCandidate = currentDir;
            found = true;
        }
        // try relative path from app dir
        else {
            currentDir = QDir(qApp->applicationDirPath());
            if (currentDir.cd("../src/qt/res/themes")) {
                themesDirCandidate = currentDir;
                found = true;
            }
            // try relative to build dir
            else {
                currentDir = QDir(qApp->applicationDirPath());
                if (currentDir.cd("../../src/qt/res/themes")) {
                    themesDirCandidate = currentDir;
                    found = true;
                }
                // try absolute path from executable location
                else {
                    QString appPath = qApp->applicationDirPath();
                    QString themesPath = appPath + "/src/qt/res/themes";
                    QDir absDir(themesPath);
                    if (absDir.exists()) {
                        themesDirCandidate = absDir;
                        found = true;
                    }
                }
            }
        }
    }
    
    if (!found) {
        // themes not found :(
        return;
    }
    
    themesDir = themesDirCandidate.absolutePath();
    themesDirCandidate.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    themesDirCandidate.setSorting(QDir::Name);
    QStringList entries = themesDirCandidate.entryList();
    
    for( QStringList::ConstIterator entry=entries.begin(); entry!=entries.end(); ++entry )
    {
        QString themeName=*entry;
        // Check if theme directory has a styles.qss file
        QDir themeDir(themesDirCandidate.absoluteFilePath(themeName));
        if (themeDir.exists("styles.qss")) {
            themes.append(themeName);
        }
    }

    // get selected theme from registry (if any)
    QSettings settings;
    selectedTheme = settings.value("Template").toString();
    // If no theme selected or theme doesn't exist, use "Default" which means system default
    if (selectedTheme=="" || !themes.contains(selectedTheme)) {
        selectedTheme = "";  // Empty string = system default (what "Default" means)
    }
    // "Default" means system default (empty string), so map it
    if (selectedTheme == "Default") {
        selectedTheme = "";
    }
    // load it! (Note: updateThemeCheckmarks can't be called here because actions don't exist yet)
    loadTheme(selectedTheme);
}

void BitcoinGUI::updateThemeCheckmarks()
{
    // Uncheck all theme actions first
    if (systemDefaultThemeAction) {
        systemDefaultThemeAction->setChecked(false);
    }
    for (int i = 0; i < themesList.count(); i++) {
        if (customActions[i]) {
            customActions[i]->setChecked(false);
        }
    }
    
    // Check the currently selected theme
    if (selectedTheme == "") {
        // System default selected
        if (systemDefaultThemeAction) {
            systemDefaultThemeAction->setChecked(true);
        }
    } else {
        // Find and check the matching theme action
        for (int i = 0; i < themesList.count(); i++) {
            if (customActions[i] && themesList[i] == selectedTheme) {
                customActions[i]->setChecked(true);
                break;
            }
        }
    }
}

void BitcoinGUI::keyPressEvent(QKeyEvent * e)
{
    switch (e->type())
     {
       case QEvent::KeyPress:
         // $ key
         if (e->key() == 36) {
             // dev feature: key reloads selected theme
             loadTheme(selectedTheme);
         }
         break;
       default:
         break;
     }

}

/* /zeewolf: Hot swappable wallet themes */
