macx{
 cache()
}

message($$_PRO_FILE_PWD_)

TEMPLATE = app
TARGET = MotaCoin-qt
VERSION = 1.5.0
INCLUDEPATH += src src/json src/qt
contains(QMAKE_CXX,arm-raspbian-linux-gnueabihf-g++) {
  QT_SYSROOT = /Users/tim/raspi/sysroot
#  INCLUDEPATH += $$QT_SYSROOT/usr/include
#  INCLUDEPATH += $$QT_SYSROOT/usr/include/arm-linux-gnueabihf
#  INCLUDEPATH += $$QT_SYSROOT/usr/local/include
  CONFIG += static
} else {
  INCLUDEPATH += /usr/local/include
}

DEFINES += QT_GUI BOOST_THREAD_USE_LIB BOOST_SPIRIT_THREADSAFE
CONFIG += no_include_pwd
CONFIG += thread
QT += core gui widgets network dbus
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
lessThan(QT_MAJOR_VERSION, 5): CONFIG += static

macx|linux-g++:release {
    CONFIG += static
}

QMAKE_CXXFLAGS += -fpermissive
win32:QMAKE_CXXFLAGS += -Wa,-mbig-obj

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
    DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
}

# use pacman -S for msys2 (win32)
win32:BOOST_LIB_SUFFIX=-mt

# use brew install for macx
# brew tap rbenv/tap
# brew install rbenv/tap/openssl@1.0

macx {
  BDB_INCLUDE_PATH=$$_PRO_FILE_PWD_/../db4/include
  BDB_LIB_PATH=$$_PRO_FILE_PWD_/../db4/lib
  OPENSSL_INCLUDE_PATH=$$_PRO_FILE_PWD_/../openssl@1.0/include
  OPENSSL_LIB_PATH=$$_PRO_FILE_PWD_/../openssl@1.0/lib
  QRENCODE_INCLUDE_PATH=$$_PRO_FILE_PWD_/../qrencode@4.1/include
  QRENCODE_LIB_PATH=$$_PRO_FILE_PWD_/../qrencode@4.1/lib
  MINIUPNPC_INCLUDE_PATH=/usr/local/opt/miniupnpc/include
  MINIUPNPC_LIB_PATH=/usr/local/opt/miniupnpc/lib
}

# TODO write ifs to allow override
linux-g++ {
  BDB_INCLUDE_PATH=$$_PRO_FILE_PWD_/../db4/include
  BDB_LIB_PATH=$$_PRO_FILE_PWD_/../db4/lib
  OPENSSL_INCLUDE_PATH=$$_PRO_FILE_PWD_/../openssl@1.0/include
  OPENSSL_LIB_PATH=$$_PRO_FILE_PWD_/../openssl@1.0/lib
  QRENCODE_INCLUDE_PATH=$$_PRO_FILE_PWD_/../qrencode@4.1/include
  QRENCODE_LIB_PATH=$$_PRO_FILE_PWD_/../qrencode@4.1/lib
  BOOST_INCLUDE_PATH=/usr/include/x86_64-linux-gnu
  BOOST_LIB_PATH=/usr/lib/x86_64-linux-gnu
  MINIUPNPC_INCLUDE_PATH=/usr/include/x86_64-linux-gnu
  MINIUPNPC_LIB_PATH=/usr/lib/x86_64-linux-gnu
}

contains(QMAKE_CXX,arm-raspbian-linux-gnueabihf-g++) {
  QTPLUGIN += qxcb qeglfs
  QMAKE_CXXFLAGS += -fpermissive --sysroot=$$QT_SYSROOT
  BDB_INCLUDE_PATH=$$QT_SYSROOT/usr/local/include
  BDB_LIB_PATH=$$QT_SYSROOT/usr/local/lib
  OPENSSL_INCLUDE_PATH=$$QT_SYSROOT/usr/local/openssl/include
  OPENSSL_LIB_PATH=$$QT_SYSROOT/usr/local/openssl/lib
  QRENCODE_INCLUDE_PATH=$$QT_SYSROOT/usr/local/include
  QRENCODE_LIB_PATH=$$QT_SYSROOT/usr/local/lib
#  BOOST_INCLUDE_PATH=$$QT_SYSROOT/usr/include/arm-linux-gnueabihf
#  BOOST_LIB_PATH=$$QT_SYSROOT/usr/lib/arm-linux-gnueabihf
}


#Added win32 conditional - Yash
#    QMAKE_TARGET_BUNDLE_PREFIX = co.opalcoin
#    BOOST_LIB_SUFFIX=-mt
#    BOOST_INCLUDE_PATH=/usr/local/Cellar/boost/1.57.0/include
#    BOOST_LIB_PATH=/usr/local/Cellar/boost/1.57.0/lib
#
#    BDB_INCLUDE_PATH=/usr/local/opt/berkeley-db4/include
#    BDB_LIB_PATH=/usr/local/Cellar/berkeley-db4/4.8.30/lib
#
#    OPENSSL_INCLUDE_PATH=/usr/local/opt/openssl/include
#    OPENSSL_LIB_PATH=/usr/local/opt/openssl/lib
#
#    MINIUPNPC_INCLUDE_PATH=/usr/local/opt/miniupnpc/include
#    MINIUPNPC_LIB_PATH=/usr/local/Cellar/miniupnpc/1.9.20141027/lib
#
#    QRENCODE_INCLUDE_PATH=/usr/local/opt/qrencode/include
#    QRENCODE_LIB_PATH=/usr/local/opt/qrencode/lib
#
#    DEFINES += IS_ARCH_64
#    QMAKE_CXXFLAGS += -arch x86_64 -stdlib=libc++
#    QMAKE_CFLAGS += -arch x86_64
#    QMAKE_LFLAGS += -arch x86_64 -stdlib=libc++

# for boost 1.37, add -mt to the boost libraries
# use: qmake BOOST_LIB_SUFFIX=-mt
# for boost thread win32 with _win32 sufix
# use: BOOST_THREAD_LIB_SUFFIX=_win32-...
# or when linking against a specific BerkelyDB versioD: BDB_LIB_SUFFIX=-4.8

# Dependency library locations can be customized with:
#    BOOST_INCLUDE_PATH, BOOST_LIB_PATH, BDB_INCLUDE_PATH,
#    BDB_LIB_PATH, OPENSSL_INCLUDE_PATH and OPENSSL_LIB_PATH respectively

OBJECTS_DIR = build
MOC_DIR = build
UI_DIR = build


# use: qmake "RELEASE=1"
contains(RELEASE, 1) {
    # Mac: compile for maximum compatibility (10.5, 32-bit)
    # macx:QMAKE_CXXFLAGS += -mmacosx-version-min=10.5 -arch x86_64 -isysroot /Developer/SDKs/MacOSX10.5.sdk

    !windows:!macx {
        # Linux: static link
        LIBS += -Wl,-Bstatic
    }
}
# come back and fix this later
macx:QMAKE_MAC_SDK = macosx10.15
macx:QMAKE_CXXFLAGS += -Wno-error=implicit-function-declaration -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.15.sdk -mmacosx-version-min=10.10
#macx:QMAKE_MAC_SDK.macosx.Path = ~/Applications/Xcode_10.2.1.app/Contents/Developer/Platforms/MacOSX.platform
#macx:QMAKE_MAC_SDK.macosx.PlatformPath = ~/Applications/Xcode_10.2.1.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.14.sdk
#macx:QMAKE_MAC_SDK.macosx.SDKVersion = 10.14
macx:QMAKE_CXXFLAGS +=


!win32 {
# for extra security against potential buffer overflows: enable GCCs Stack Smashing Protection
QMAKE_CXXFLAGS *= -fstack-protector-all --param ssp-buffer-size=1
QMAKE_LFLAGS *= -fstack-protector-all --param ssp-buffer-size=1
# We need to exclude this for Windows cross compile with MinGW 4.2.x, as it will result in a non-working executable!
# This can be enabled for Windows, when we switch to MinGW >= 4.4.x.
}
# for extra security on Windows: enable ASLR and DEP via GCC linker flags
#win32:QMAKE_LFLAGS *= -Wl,--large-address-aware -static
win32:QMAKE_LFLAGS *= -static
win32:QMAKE_LFLAGS += -static-libgcc -static-libstdc++
lessThan(QT_MAJOR_VERSION, 5): win32: QMAKE_LFLAGS *= -static


USE_QRCODE=1
# use: qmake "USE_QRCODE=1"
# libqrencode (http://fukuchi.org/works/qrencode/index.en.html) must be installed for support - configure with --enable_static
contains(USE_QRCODE, 1) {
    message(Building with QRCode support)
    DEFINES += USE_QRCODE
    macx|linux-g++:release  {
      LIBS += $$QRENCODE_LIB_PATH/libqrencode.a
    }
    else {
      LIBS += -lqrencode
    }
}

# use: qmake "USE_UPNP=1" ( enabled by default; default)
#  or: qmake "USE_UPNP=0" (disabled by default)
#  or: qmake "USE_UPNP=-" (not supported)
# miniupnpc (http://miniupnp.free.fr/files/) must be installed for support
contains(USE_UPNP, -) {
    message(Building without UPNP support)
} else {
    message(Building with UPNP support)
    count(USE_UPNP, 0) {
        USE_UPNP=1
    }
    DEFINES += USE_UPNP=$$USE_UPNP STATICLIB MINIUPNP_STATICLIB
    INCLUDEPATH += $$MINIUPNPC_INCLUDE_PATH
    LIBS += $$join(MINIUPNPC_LIB_PATH,,-L,)
    macx|linux-g++:release  {
      LIBS += $$MINIUPNPC_LIB_PATH/libminiupnpc.a
    }
    else {
      LIBS += -lminiupnpc
    }
    win32:LIBS += -liphlpapi
}

# use: qmake "USE_DBUS=1"
contains(USE_DBUS, 1) {
    message(Building with DBUS (Freedesktop notifications) support)
    DEFINES += USE_DBUS
    QT += dbus
}

# use: qmake "USE_IPV6=1" ( enabled by default; default)
#  or: qmake "USE_IPV6=0" (disabled by default)
#  or: qmake "USE_IPV6=-" (not supported)
contains(USE_IPV6, -) {
    message(Building without IPv6 support)
} else {
    count(USE_IPV6, 0) {
        USE_IPV6=1
    }
    DEFINES += USE_IPV6=$$USE_IPV6
}

contains(BITCOIN_NEED_QT_PLUGINS, 1) {
    DEFINES += BITCOIN_NEED_QT_PLUGINS
    QTPLUGIN += qcncodecs qjpcodecs qtwcodecs qkrcodecs qtaccessiblewidgets
}

INCLUDEPATH += src/leveldb/include src/leveldb/helpers
LIBS += $$PWD/src/leveldb/libleveldb.a $$PWD/src/leveldb/libmemenv.a
SOURCES += src/txdb-leveldb.cpp \
    src/bloom.cpp \
    src/hash.cpp \
    src/aes_helper.c \
    src/blake.c \
    src/bmw.c \
    src/cubehash.c \
    src/echo.c \
    src/groestl.c \
    src/jh.c \
    src/keccak.c \
    src/luffa.c \
    src/shavite.c \
    src/simd.c \
    src/skein.c \
        src/fugue.c \
        src/hamsi.c

win32 {
  # make an educated guess about what the ranlib command is called
  isEmpty(QMAKE_RANLIB) {
      QMAKE_RANLIB = $$replace(QMAKE_STRIP, strip, ranlib)
  }
  LIBS += -lshlwapi
  genleveldb.commands = cd $$PWD/src/leveldb && CC=$$QMAKE_CC CXX=$$QMAKE_CXX TARGET_OS=OS_WINDOWS_CROSSCOMPILE $(MAKE) OPT=\"$$QMAKE_CXXFLAGS $$QMAKE_CXXFLAGS_RELEASE\" libleveldb.a libmemenv.a && $$QMAKE_RANLIB $$PWD/src/leveldb/libleveldb.a && $$QMAKE_RANLIB $$PWD/src/leveldb/libmemenv.a
} else {
  contains(QMAKE_CXX,arm-raspbian-linux-gnueabihf-g++) {
    warning('yay raspbian')
    warning(QMAKE_CXX=$$QMAKE_CXX)
    warning(QMAKE_CXXFLAGS=$$QMAKE_CXXFLAGS)
    QMAKE_RANLIB=arm-raspbian-linux-gnueabihf-ranlib
    warning(QMAKE_RANLIB=$$QMAKE_RANLIB)
    genleveldb.commands = cd $$PWD/src/leveldb && CC=$$QMAKE_CC CXX=$$QMAKE_CXX TARGET_OS=OS_RASPBIAN_CROSSCOMPILE $(MAKE) OPT=\"$$QMAKE_CXXFLAGS $$QMAKE_CXXFLAGS_RELEASE\" libleveldb.a libmemenv.a  && $$QMAKE_RANLIB libleveldb.a && $$QMAKE_RANLIB libmemenv.a
  }
  else {
    # we use QMAKE_CXXFLAGS_RELEASE even without RELEASE=1 because we use RELEASE to indicate linking preferences not -O preferences
    genleveldb.commands = cd $$PWD/src/leveldb && CC=$$QMAKE_CC CXX=$$QMAKE_CXX $(MAKE) OPT=\"$$QMAKE_CXXFLAGS $$QMAKE_CXXFLAGS_RELEASE\" libleveldb.a libmemenv.a
  }
}

genleveldb.target = $$PWD/src/leveldb/libleveldb.a
genleveldb.depends = FORCE
PRE_TARGETDEPS += $$PWD/src/leveldb/libleveldb.a
QMAKE_EXTRA_TARGETS += genleveldb
# Gross ugly hack that depends on qmake internals, unfortunately there is no other way to do it.
QMAKE_CLEAN += $$PWD/src/leveldb/libleveldb.a; cd $$PWD/src/leveldb ; $(MAKE) clean

# regenerate src/build.h
!windows|contains(USE_BUILD_INFO, 1) {
    genbuild.depends = FORCE
    genbuild.commands = cd $$PWD; /bin/sh share/genbuild.sh $$OUT_PWD/build/build.h
    genbuild.target = $$OUT_PWD/build/build.h
    PRE_TARGETDEPS += $$OUT_PWD/build/build.h
    QMAKE_EXTRA_TARGETS += genbuild
    DEFINES += HAVE_BUILD_INFO
}

contains(USE_O3, 1) {
    message(Building O3 optimization flag)
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS += -O3
    QMAKE_CFLAGS += -O3
}

*-g++-32 {
    message("32 platform, adding -msse2 flag")

    QMAKE_CXXFLAGS += -msse2
    QMAKE_CFLAGS += -msse2
}

QMAKE_CXXFLAGS_WARN_ON = -fdiagnostics-show-option -Wall  -Wextra -Wno-ignored-qualifiers -Wformat -Wformat-security -Wno-unused-parameter -Wstack-protector


# Input
DEPENDPATH += src src/json src/qt
HEADERS += src/qt/bitcoingui.h \
    src/qt/transactiontablemodel.h \
    src/qt/addresstablemodel.h \
    src/qt/optionsdialog.h \
    src/qt/coincontroldialog.h \
    src/qt/coincontroltreewidget.h \
    src/qt/sendcoinsdialog.h \
    src/qt/addressbookpage.h \
    src/qt/signverifymessagedialog.h \
    src/qt/aboutdialog.h \
	src/qt/charitydialog.h \
    src/qt/editaddressdialog.h \
    src/qt/bitcoinaddressvalidator.h \
    src/alert.h \
    src/addrman.h \
    src/base58.h \
    src/bignum.h \
    src/checkpoints.h \
    src/compat.h \
    src/coincontrol.h \
    src/sync.h \
    src/util.h \
    src/uint256.h \
    src/uint256_t.h \
    src/kernel.h \
    src/scrypt.h \
    src/pbkdf2.h \
    src/serialize.h \
    src/strlcpy.h \
    src/main.h \
    src/miner.h \
    src/net.h \
    src/key.h \
    src/db.h \
    src/txdb.h \
    src/walletdb.h \
    src/script.h \
    src/init.h \
    src/irc.h \
    src/mruset.h \
    src/json/json_spirit_writer_template.h \
    src/json/json_spirit_writer.h \
    src/json/json_spirit_value.h \
    src/json/json_spirit_utils.h \
    src/json/json_spirit_stream_reader.h \
    src/json/json_spirit_reader_template.h \
    src/json/json_spirit_reader.h \
    src/json/json_spirit_error_position.h \
    src/json/json_spirit.h \
    src/qt/clientmodel.h \
    src/qt/guiutil.h \
    src/qt/transactionrecord.h \
    src/qt/guiconstants.h \
    src/qt/optionsmodel.h \
    src/qt/monitoreddatamapper.h \
    src/qt/transactiondesc.h \
    src/qt/transactiondescdialog.h \
    src/qt/bitcoinamountfield.h \
    src/wallet.h \
    src/keystore.h \
    src/qt/transactionfilterproxy.h \
    src/qt/transactionview.h \
    src/qt/walletmodel.h \
    src/bitcoinrpc.h \
    src/qt/overviewpage.h \
    src/qt/csvmodelwriter.h \
    src/crypter.h \
    src/qt/sendcoinsentry.h \
    src/qt/qvalidatedlineedit.h \
    src/qt/bitcoinunits.h \
    src/qt/qvaluecombobox.h \
    src/qt/askpassphrasedialog.h \
    src/protocol.h \
    src/qt/notificator.h \
    src/qt/qtipcserver.h \
    src/allocators.h \
    src/ui_interface.h \
    src/qt/rpcconsole.h \
    src/qt/blockbrowser.h \
    src/qt/statisticspage.h \
    src/version.h \
    src/netbase.h \
    src/clientversion.h \
    src/qt/chatwindow.h \
    src/qt/serveur.h \
    src/bloom.h \
    src/checkqueue.h \
    src/hash.h \
    src/hashblock.h \
    src/limitedmap.h \
    src/sph_blake.h \
    src/sph_bmw.h \
    src/sph_cubehash.h \
    src/sph_echo.h \
    src/sph_groestl.h \
    src/sph_jh.h \
    src/sph_keccak.h \
    src/sph_luffa.h \
    src/sph_shavite.h \
    src/sph_simd.h \
    src/sph_skein.h \
    src/sph_fugue.h \
    src/sph_hamsi.h \
    src/sph_types.h \
    src/threadsafety.h \
    src/txdb-leveldb.h \
    src/types/camount.h

SOURCES += src/qt/bitcoin.cpp src/qt/bitcoingui.cpp \
    src/qt/transactiontablemodel.cpp \
    src/qt/addresstablemodel.cpp \
    src/qt/optionsdialog.cpp \
    src/qt/sendcoinsdialog.cpp \
    src/qt/coincontroldialog.cpp \
    src/qt/coincontroltreewidget.cpp \
    src/qt/addressbookpage.cpp \
    src/qt/signverifymessagedialog.cpp \
    src/qt/aboutdialog.cpp \
	src/qt/charitydialog.cpp \
    src/qt/editaddressdialog.cpp \
    src/qt/bitcoinaddressvalidator.cpp \
        src/qt/chatwindow.cpp \
        src/qt/statisticspage.cpp \
        src/qt/blockbrowser.cpp \
        src/qt/serveur.cpp \
    src/alert.cpp \
    src/version.cpp \
    src/sync.cpp \
    src/util.cpp \
    src/netbase.cpp \
    src/key.cpp \
    src/script.cpp \
    src/main.cpp \
    src/miner.cpp \
    src/init.cpp \
    src/net.cpp \
    src/irc.cpp \
    src/checkpoints.cpp \
    src/addrman.cpp \
    src/db.cpp \
    src/walletdb.cpp \
    src/qt/clientmodel.cpp \
    src/qt/guiutil.cpp \
    src/qt/transactionrecord.cpp \
    src/qt/optionsmodel.cpp \
    src/qt/monitoreddatamapper.cpp \
    src/qt/transactiondesc.cpp \
    src/qt/transactiondescdialog.cpp \
    src/qt/bitcoinstrings.cpp \
    src/qt/bitcoinamountfield.cpp \
    src/wallet.cpp \
    src/keystore.cpp \
    src/qt/transactionfilterproxy.cpp \
    src/qt/transactionview.cpp \
    src/qt/walletmodel.cpp \
    src/bitcoinrpc.cpp \
    src/rpcdump.cpp \
    src/rpcnet.cpp \
    src/rpcmining.cpp \
    src/rpcwallet.cpp \
    src/rpcblockchain.cpp \
    src/rpcrawtransaction.cpp \
    src/qt/overviewpage.cpp \
    src/qt/csvmodelwriter.cpp \
    src/crypter.cpp \
    src/qt/sendcoinsentry.cpp \
    src/qt/qvalidatedlineedit.cpp \
    src/qt/bitcoinunits.cpp \
    src/qt/qvaluecombobox.cpp \
    src/qt/askpassphrasedialog.cpp \
    src/protocol.cpp \
    src/qt/notificator.cpp \
    src/qt/qtipcserver.cpp \
    src/qt/rpcconsole.cpp \
    src/noui.cpp \
    src/kernel.cpp \
    src/scrypt-arm.S \
    src/scrypt-x86.S \
    src/scrypt-x86_64.S \
    src/scrypt.cpp \
    src/pbkdf2.cpp

RESOURCES += \
    src/qt/bitcoin.qrc

FORMS += \
    src/qt/forms/coincontroldialog.ui \
    src/qt/forms/sendcoinsdialog.ui \
    src/qt/forms/addressbookpage.ui \
    src/qt/forms/signverifymessagedialog.ui \
    src/qt/forms/aboutdialog.ui \
	  src/qt/forms/charitydialog.ui \
    src/qt/forms/editaddressdialog.ui \
    src/qt/forms/transactiondescdialog.ui \
    src/qt/forms/overviewpage.ui \
    src/qt/forms/sendcoinsentry.ui \
    src/qt/forms/askpassphrasedialog.ui \
    src/qt/forms/rpcconsole.ui \
        src/qt/forms/statisticspage.ui \
        src/qt/forms/blockbrowser.ui \
        src/qt/forms/chatwindow.ui \
    src/qt/forms/optionsdialog.ui

contains(USE_QRCODE, 1) {
  HEADERS += src/qt/qrcodedialog.h
  SOURCES += src/qt/qrcodedialog.cpp
  FORMS += src/qt/forms/qrcodedialog.ui
}

CODECFORTR = UTF-8

# for lrelease/lupdate
# also add new translations to src/qt/bitcoin.qrc under translations/
TRANSLATIONS = $$files(src/qt/locale/bitcoin_*.ts)

isEmpty(QMAKE_LRELEASE) {
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}
isEmpty(QM_DIR):QM_DIR = $$PWD/src/qt/locale
# automatically build translations, so they can be included in resource file
TSQM.name = lrelease ${QMAKE_FILE_IN}
TSQM.input = TRANSLATIONS
TSQM.output = $$QM_DIR/${QMAKE_FILE_BASE}.qm
TSQM.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
TSQM.CONFIG = no_link
QMAKE_EXTRA_COMPILERS += TSQM

# "Other files" to show in Qt Creator
OTHER_FILES += \
    doc/*.rst doc/*.txt doc/README README.md res/motacoin-qt.rc

# platform specific defaults, if not overridden on command line
isEmpty(BOOST_LIB_SUFFIX) {
    macx:BOOST_LIB_SUFFIX = -mt
    wiD:BOOST_LIB_SUFFIX = -mgw48-mt-s-1_550  #Replaced windows with win - Yash
}

isEmpty(BOOST_THREAD_LIB_SUFFIX) {
    BOOST_THREAD_LIB_SUFFIX = $$BOOST_LIB_SUFFIX
}

#isEmpty(BDB_LIB_PATH) {
#    macx:BDB_LIB_PATH =/Users/tim/Projects/bdb/db4/lib
#}

isEmpty(BDB_LIB_SUFFIX) {
    macx:BDB_LIB_SUFFIX = -4.8
}

#isEmpty(BDB_INCLUDE_PATH) {
#    macx:BDB_INCLUDE_PATH = /Users/tim/Projects/bdb/db4/include
#}

isEmpty(BOOST_LIB_PATH) {
    macx:BOOST_LIB_PATH = /usr/local/lib
}

isEmpty(BOOST_INCLUDE_PATH) {
    macx:BOOST_INCLUDE_PATH = /opt/local/include
}

windows:DEFINES += WIN32
windows:RC_FILE = src/qt/res/motacoin-qt.rc

windows:!contains(MINGW_THREAD_BUGFIX, 0) {
    # At least qmake's win32-g++-cross profile is missing the -lmingwthrd
    # thread-safety flag. GCC has -mthreads to enable this, but it doesn't
    # work with static linking. -lmingwthrd must come BEFORE -lmingw, so
    # it is prepended to QMAKE_LIBS_QT_ENTRY.
    # It can be turned off with MINGW_THREAD_BUGFIX=0, just in case it causes
    # any problems on some untested qmake profile now or in the future.
    DEFINES += _MT BOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN
    QMAKE_LIBS_QT_ENTRY = -lmingwthrd $$QMAKE_LIBS_QT_ENTRY
}

macx {
  HEADERS += src/qt/macdockiconhandler.h
  OBJECTIVE_SOURCES += src/qt/macdockiconhandler.mm
  LIBS += -framework Foundation -framework ApplicationServices -framework AppKit
  DEFINES += MAC_OSX MSG_NOSIGNAL=0
  ICON = src/qt/res/icons/motacoin.icns
  TARGET = "MotaCoin-Qt"
  QMAKE_CFLAGS_THREAD += -pthread
  QMAKE_LFLAGS_THREAD += -pthread
  QMAKE_CXXFLAGS_THREAD += -pthread
}

# Set libraries and includes at end, to use platform-defined defaults if not overridden
# Be careful about the order
INCLUDEPATH = $$BOOST_INCLUDE_PATH $$BDB_INCLUDE_PATH $$OPENSSL_INCLUDE_PATH $$QRENCODE_INCLUDE_PATH $$INCLUDEPATH
LIBS += $$join(BOOST_LIB_PATH,,-L,) $$join(BDB_LIB_PATH,,-L,) $$join(OPENSSL_LIB_PATH,,-L,) $$join(QRENCODE_LIB_PATH,,-L,)

message($$INCLUDEPATH)

# Static-linked binaries are not supported in osx
# https://developer.apple.com/library/content/qa/qa1118/_index.html
macx|linux-g++:release {
  LIBS += $$OPENSSL_LIB_PATH/libssl.a $$OPENSSL_LIB_PATH/libcrypto.a $$BDB_LIB_PATH/libdb_cxx.a
  LIBS += $$BOOST_LIB_PATH/libboost_system$${BOOST_LIB_SUFFIX}.a $$BOOST_LIB_PATH/libboost_filesystem$${BOOST_LIB_SUFFIX}.a $$BOOST_LIB_PATH/libboost_program_options$${BOOST_LIB_SUFFIX}.a $$BOOST_LIB_PATH/libboost_thread$${BOOST_LIB_SUFFIX}.a $$BOOST_LIB_PATH/libboost_chrono$${BOOST_LIB_SUFFIX}.a
}
else {
  LIBS += -lssl -lcrypto -ldb_cxx$$BDB_LIB_SUFFIX
  LIBS += -lboost_system$$BOOST_LIB_SUFFIX -lboost_filesystem$$BOOST_LIB_SUFFIX -lboost_program_options$$BOOST_LIB_SUFFIX -lboost_thread$$BOOST_THREAD_LIB_SUFFIX -lboost_chrono$$BOOST_THREAD_LIB_SUFFIX

  # -lgdi32 has to happen after -lcrypto (see  #681)
  windows:LIBS += -lws2_32 -lshlwapi -lmswsock -lole32 -loleaut32 -luuid -lgdi32
  windows:LIBS += -lboost_chrono$$BOOST_LIB_SUFFIX
}

contains(RELEASE, 1) {
    !windows:!macx {
        # Linux: turn dynamic linking back on for c/c++ runtime libraries
        LIBS += -Wl,-Bdynamic -ldl
    }
}

!windows:!macx {
    DEFINES += LINUX
    LIBS += -ldl
	LIBS += -lrt
}

system($$QMAKE_LRELEASE -silent $$_PRO_FILE_)
