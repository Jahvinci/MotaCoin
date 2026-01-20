//
// Unit tests for block signature verification with charity outputs
//
// Note: BOOST_TEST_MODULE is defined in test_bitcoin.cpp
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include "main.h"
#include "wallet.h"
#include "key.h"
#include "script.h"
#include "base58.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(block_signature_tests)

// Helper function to create a coinstake transaction without charity
static CTransaction CreateCoinstakeWithoutCharity(CKey& key)
{
    CTransaction tx;
    tx.nTime = GetTime();
    
    // Coinstake requires at least one input (not null)
    tx.vin.resize(1);
    tx.vin[0].prevout.hash = uint256("1234567890123456789012345678901234567890123456789012345678901234");
    tx.vin[0].prevout.n = 0;
    
    // Mark as coinstake
    CScript scriptEmpty;
    scriptEmpty.clear();
    tx.vout.push_back(CTxOut(0, scriptEmpty)); // vout[0] = marker
    
    // Add staker output (TX_PUBKEY)
    CScript scriptPubKey;
    scriptPubKey << key.GetPubKey() << OP_CHECKSIG;
    tx.vout.push_back(CTxOut(100 * COIN, scriptPubKey)); // vout[1] = staker output
    
    return tx;
}

// Helper function to create a coinstake transaction with charity outputs
static CTransaction CreateCoinstakeWithCharity(CKey& stakerKey, CKey& charityKey)
{
    CTransaction tx;
    tx.nTime = GetTime();
    
    // Coinstake requires at least one input (not null)
    tx.vin.resize(1);
    tx.vin[0].prevout.hash = uint256("1234567890123456789012345678901234567890123456789012345678901234");
    tx.vin[0].prevout.n = 0;
    
    // Mark as coinstake
    CScript scriptEmpty;
    scriptEmpty.clear();
    tx.vout.push_back(CTxOut(0, scriptEmpty)); // vout[0] = marker
    
    // Add charity output (TX_PUBKEYHASH - address-based)
    CScript scriptCharity;
    scriptCharity.SetDestination(charityKey.GetPubKey().GetID());
    tx.vout.push_back(CTxOut(10 * COIN, scriptCharity)); // vout[1] = charity output
    
    // Add staker output (TX_PUBKEY)
    CScript scriptPubKey;
    scriptPubKey << stakerKey.GetPubKey() << OP_CHECKSIG;
    tx.vout.push_back(CTxOut(90 * COIN, scriptPubKey)); // vout[2] = staker output
    
    return tx;
}

// Helper function to create a proof-of-stake block
static CBlock CreatePoSBlock(const CTransaction& coinstake, CKey& key)
{
    CBlock block;
    block.nVersion = 7;
    block.nTime = GetTime();
    block.nBits = 0x1d00ffff;
    block.nNonce = 0;
    block.hashPrevBlock = 0; // Test block, no previous block needed
    
    // Coinbase transaction for PoS: must have 2 empty outputs
    CTransaction coinbase;
    coinbase.nTime = block.nTime;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(2); // PoS requires 2 empty outputs
    coinbase.vout[0].scriptPubKey.clear();
    coinbase.vout[0].nValue = 0;
    coinbase.vout[1].scriptPubKey.clear();
    coinbase.vout[1].nValue = 0;
    block.vtx.push_back(coinbase);
    
    // Coinstake transaction
    block.vtx.push_back(coinstake);
    
    // Build merkle tree
    block.hashMerkleRoot = block.BuildMerkleTree();
    
    // Sign block - GetHash() calculates from header (doesn't include signature)
    // Important: GetHash() must be called AFTER all block fields are set
    uint256 hashBlock = block.GetHash();
    
    // Verify hash is consistent
    uint256 hashBlock2 = block.GetHash();
    if (hashBlock != hashBlock2)
    {
        BOOST_ERROR("Block hash changed between calls!");
    }
    
    if (!key.Sign(hashBlock, block.vchBlockSig))
    {
        BOOST_ERROR("Failed to sign block");
    }
    
    // Verify signature immediately after creation
    // Find the staker output (first TX_PUBKEY output) to verify signature
    CKey verifyKey;
    vector<valtype> vSolutions;
    txnouttype whichType;
    bool foundStaker = false;
    for (unsigned int i = 1; i < coinstake.vout.size() && !foundStaker; i++)
    {
        txnouttype testType;
        vector<valtype> testSolutions;
        if (Solver(coinstake.vout[i].scriptPubKey, testType, testSolutions) && testType == TX_PUBKEY)
        {
            whichType = testType;
            vSolutions = testSolutions;
            foundStaker = true;
        }
    }
    // Fallback to vout[1] for traditional blocks (backward compatibility)
    if (!foundStaker && coinstake.vout.size() >= 2)
    {
        if (Solver(coinstake.vout[1].scriptPubKey, whichType, vSolutions) && whichType == TX_PUBKEY)
        {
            foundStaker = true;
        }
    }
    if (foundStaker && whichType == TX_PUBKEY && !vSolutions.empty())
    {
        verifyKey.SetPubKey(vSolutions[0]);
        bool immediateVerify = verifyKey.Verify(hashBlock, block.vchBlockSig);
        if (!immediateVerify)
        {
            BOOST_ERROR("Immediate signature verification failed - signing may be incorrect");
        }
    }
    else
    {
        BOOST_ERROR("Failed to find staker output (TX_PUBKEY) for immediate signature verification");
    }
    
    return block;
}

BOOST_AUTO_TEST_CASE(CheckBlockSignature_WithoutCharity)
{
    // Test backward compatibility: block without charity should work
    // This tests the fallback to vout[1] behavior
    
    CKey stakerKey;
    stakerKey.MakeNewKey(false); // uncompressed key for TX_PUBKEY
    
    CTransaction coinstake = CreateCoinstakeWithoutCharity(stakerKey);
    CBlock block = CreatePoSBlock(coinstake, stakerKey);
    
    // Debug: Check block structure
    BOOST_TEST_MESSAGE("Block vtx[1].vout.size() = " << block.vtx[1].vout.size());
    BOOST_TEST_MESSAGE("Block vchBlockSig.size() = " << block.vchBlockSig.size());
    BOOST_TEST_MESSAGE("Block hash = " << block.GetHash().ToString());
    
    // Verify structure: vout[0] = marker, vout[1] = staker output
    BOOST_CHECK(block.vtx[1].vout.size() >= 2);
    BOOST_CHECK(block.vtx[1].vout[0].scriptPubKey.empty()); // marker
    BOOST_CHECK(!block.vtx[1].vout[1].scriptPubKey.empty()); // staker output
    
    // Check if signature is empty
    if (block.vchBlockSig.empty())
    {
        BOOST_ERROR("Block signature is empty - signing may have failed");
    }
    
    // Debug: Check scriptPubKey
    vector<valtype> vSolutions;
    txnouttype whichType;
    if (Solver(block.vtx[1].vout[1].scriptPubKey, whichType, vSolutions))
    {
        BOOST_TEST_MESSAGE("Staker output script type: " << whichType);
        BOOST_TEST_MESSAGE("Staker output solutions size: " << vSolutions.size());
        if (whichType == TX_PUBKEY && vSolutions.size() > 0)
        {
            CKey testKey;
            if (testKey.SetPubKey(vSolutions[0]))
            {
                BOOST_TEST_MESSAGE("Public key extracted successfully");
                BOOST_TEST_MESSAGE("PubKey matches stakerKey: " << (testKey.GetPubKey() == stakerKey.GetPubKey()));
            }
        }
    }
    
    // Verify block signature
    bool sigResult = block.CheckBlockSignature();
    BOOST_TEST_MESSAGE("CheckBlockSignature() returned: " << sigResult);
    BOOST_CHECK(sigResult);
}

BOOST_AUTO_TEST_CASE(CheckBlockSignature_WithCharity)
{
    // Test new behavior: block with charity should find staker output dynamically
    // This is the main test for the fix
    
    CKey stakerKey;
    stakerKey.MakeNewKey(false); // uncompressed key for TX_PUBKEY
    
    CKey charityKey;
    charityKey.MakeNewKey(true); // compressed key for charity address
    
    CTransaction coinstake = CreateCoinstakeWithCharity(stakerKey, charityKey);
    CBlock block = CreatePoSBlock(coinstake, stakerKey);
    
    // Verify block signature
    BOOST_CHECK(block.CheckBlockSignature());
    
    // Verify structure: vout[0] = marker, vout[1] = charity, vout[2] = staker
    BOOST_CHECK(block.vtx[1].vout.size() >= 3);
    BOOST_CHECK(block.vtx[1].vout[0].scriptPubKey.empty()); // marker
    
    // Verify vout[1] is charity (TX_PUBKEYHASH)
    vector<valtype> vSolutions;
    txnouttype whichType;
    BOOST_CHECK(Solver(block.vtx[1].vout[1].scriptPubKey, whichType, vSolutions));
    BOOST_CHECK(whichType == TX_PUBKEYHASH); // Charity output
    
    // Verify vout[2] is staker (TX_PUBKEY)
    BOOST_CHECK(Solver(block.vtx[1].vout[2].scriptPubKey, whichType, vSolutions));
    BOOST_CHECK(whichType == TX_PUBKEY); // Staker output
}

BOOST_AUTO_TEST_CASE(CheckBlockSignature_WithMultipleCharity)
{
    // Test with multiple charity outputs
    
    CKey stakerKey;
    stakerKey.MakeNewKey(false);
    
    CKey charityKey1, charityKey2;
    charityKey1.MakeNewKey(true);
    charityKey2.MakeNewKey(true);
    
    CTransaction tx;
    tx.nTime = GetTime();
    
    // Coinstake requires at least one input (not null)
    tx.vin.resize(1);
    tx.vin[0].prevout.hash = uint256("1234567890123456789012345678901234567890123456789012345678901234");
    tx.vin[0].prevout.n = 0;
    
    // Marker
    CScript scriptEmpty;
    scriptEmpty.clear();
    tx.vout.push_back(CTxOut(0, scriptEmpty));
    
    // First charity output
    CScript scriptCharity1;
    scriptCharity1.SetDestination(charityKey1.GetPubKey().GetID());
    tx.vout.push_back(CTxOut(5 * COIN, scriptCharity1));
    
    // Second charity output
    CScript scriptCharity2;
    scriptCharity2.SetDestination(charityKey2.GetPubKey().GetID());
    tx.vout.push_back(CTxOut(5 * COIN, scriptCharity2));
    
    // Staker output
    CScript scriptPubKey;
    scriptPubKey << stakerKey.GetPubKey() << OP_CHECKSIG;
    tx.vout.push_back(CTxOut(90 * COIN, scriptPubKey));
    
    CBlock block = CreatePoSBlock(tx, stakerKey);
    
    // Verify block signature works with multiple charity outputs
    BOOST_CHECK(block.CheckBlockSignature());
    
    // Verify staker output is at correct position (vout[3])
    BOOST_CHECK(block.vtx[1].vout.size() >= 4);
    vector<valtype> vSolutions;
    txnouttype whichType;
    BOOST_CHECK(Solver(block.vtx[1].vout[3].scriptPubKey, whichType, vSolutions));
    BOOST_CHECK(whichType == TX_PUBKEY); // Staker output should be at index 3
}

BOOST_AUTO_TEST_CASE(CheckBlockSignature_InvalidSignature)
{
    // Test that invalid signatures are rejected
    
    CKey stakerKey;
    stakerKey.MakeNewKey(false);
    
    CTransaction coinstake = CreateCoinstakeWithoutCharity(stakerKey);
    CBlock block = CreatePoSBlock(coinstake, stakerKey);
    
    // Corrupt the signature
    if (!block.vchBlockSig.empty())
    {
        block.vchBlockSig[0] ^= 0x01; // Flip a bit
        
        // Should fail signature check
        BOOST_CHECK(!block.CheckBlockSignature());
    }
}

BOOST_AUTO_TEST_SUITE_END()

