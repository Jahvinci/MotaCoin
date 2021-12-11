// Copyright (c) 2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include "clientversion.h"
#include <string>

//
// database format versioning
//
static const int DATABASE_VERSION = 70508;

//
// network protocol versioning
//

static const int PROTOCOL_VERSION = 211206;	// POW rewards changed

static const int MIN_PROTO_VERSION = 211206;
// earlier versions not supported as of Jun 2021, and are disconnected
// static const int MIN_PROTO_VERSION = 210725;
// earlier versions not supported as of Feb 2012, and are disconnected
//static const int MIN_PROTO_VERSION = 209;

// nTime field added to CAddress, starting with this version;
// if possible, avoid requesting addresses nodes older than this
static const int CADDR_TIME_VERSION = 31402;

// only request blocks from nodes outside this range of versions
static const int NOBLKS_VERSION_START = 60002;
static const int NOBLKS_VERSION_END = 60006;

// BIP 0031, pong message, is enabled for all versions AFTER this one
static const int BIP0031_VERSION = 60000;

// "mempool" command, enhanced "getdata" behavior starts with this version:
static const int MEMPOOL_GD_VERSION = 60002;

#endif
