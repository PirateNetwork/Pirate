// Copyright (c) 2018 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TRANSACTION_BUILDER_H
#define TRANSACTION_BUILDER_H

#include "consensus/params.h"
#include "keystore.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "script/standard.h"
#include "uint256.h"
#include "serialize.h"
#include "streams.h"
#include "zcash/Address.hpp"
#include "zcash/IncrementalMerkleTree.hpp"
#include "zcash/Note.hpp"
#include "zcash/NoteEncryption.hpp"

#include <boost/optional.hpp>

struct SpendDescriptionInfo {
    libzcash::SaplingExpandedSpendingKey expsk;
    libzcash::SaplingNote note;
    uint256 alpha;
    uint256 anchor;
    SaplingWitness witness;

    SpendDescriptionInfo(
        libzcash::SaplingExpandedSpendingKey expsk,
        libzcash::SaplingNote note,
        uint256 anchor,
        SaplingWitness witness);
};

class SpendDescriptionInfoRaw
{
public:

    libzcash::SaplingPaymentAddress addr;
    CAmount value;
    SaplingOutPoint op;

    SpendDescriptionInfoRaw() {}

    SpendDescriptionInfoRaw(
      libzcash::SaplingPaymentAddress addrIn,
      CAmount valueIn,
      SaplingOutPoint opIn) {
      addr = addrIn;
      value = valueIn;
      op = opIn;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(addr);
        READWRITE(value);
        READWRITE(op);
    }

};



struct OutputDescriptionInfo {
    uint256 ovk;
    libzcash::SaplingNote note;
    std::array<unsigned char, ZC_MEMO_SIZE> memo;

    OutputDescriptionInfo(
        uint256 ovk,
        libzcash::SaplingNote note,
        std::array<unsigned char, ZC_MEMO_SIZE> memo) : ovk(ovk), note(note), memo(memo) {}

    boost::optional<OutputDescription> Build(void* ctx);
};


class OutputDescriptionInfoRaw
{
public:

    libzcash::SaplingPaymentAddress addr;
    CAmount value;
    std::array<unsigned char, ZC_MEMO_SIZE> memo;

    OutputDescriptionInfoRaw() {}
    OutputDescriptionInfoRaw(
        libzcash::SaplingPaymentAddress addrIn,
        CAmount valueIn,
        std::array<unsigned char, ZC_MEMO_SIZE> memoIn) { addr = addrIn; value = valueIn; memo = memoIn; }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(addr);
        READWRITE(value);
        READWRITE(memo);
    }

};




struct TransparentInputInfo {
    CScript scriptPubKey;
    CAmount value;

    TransparentInputInfo(
        CScript scriptPubKey,
        CAmount value) : scriptPubKey(scriptPubKey), value(value) {}
};

class TransactionBuilderResult {
private:
    boost::optional<CTransaction> maybeTx;
    boost::optional<std::string> maybeError;
public:
    TransactionBuilderResult() = delete;
    TransactionBuilderResult(const CTransaction& tx);
    TransactionBuilderResult(const std::string& error);
    bool IsTx();
    bool IsError();
    CTransaction GetTxOrThrow();
    std::string GetError();
};

class TransactionBuilder
{
private:
    Consensus::Params consensusParams;
    int nHeight;
    const CKeyStore* keystore;
    CMutableTransaction mtx;
    CAmount fee = 10000;

    std::vector<SpendDescriptionInfo> spends;
    std::vector<OutputDescriptionInfo> outputs;
    std::vector<TransparentInputInfo> tIns;

    boost::optional<std::pair<uint256, libzcash::SaplingPaymentAddress>> zChangeAddr;
    boost::optional<CTxDestination> tChangeAddr;
    boost::optional<CScript> opReturn;

    bool AddOpRetLast(CScript &s);

public:
    std::vector<SpendDescriptionInfoRaw> rawSpends;
    std::vector<OutputDescriptionInfoRaw> rawOutputs;

    TransactionBuilder() {}
    TransactionBuilder(const Consensus::Params& consensusParams, int nHeight, CKeyStore* keyStore = nullptr);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(rawSpends);
        READWRITE(rawOutputs);
        READWRITE(mtx);
        READWRITE(fee);
    }


    void SetFee(CAmount fee);

    void SetExpiryHeight(int nHeight);

    CTransaction getTransaction() {return mtx;}

    // Returns false if the anchor does not match the anchor used by
    // previously-added Sapling spends.
    bool AddSaplingSpend(
        libzcash::SaplingExpandedSpendingKey expsk,
        libzcash::SaplingNote note,
        uint256 anchor,
        SaplingWitness witness);

    bool AddSaplingSpendRaw(
      libzcash::SaplingPaymentAddress from,
      CAmount value,
      SaplingOutPoint op);

    void AddSaplingOutput(
        uint256 ovk,
        libzcash::SaplingPaymentAddress to,
        CAmount value,
        std::array<unsigned char, ZC_MEMO_SIZE> memo = {{0}});

    void AddSaplingOutputRaw(
        libzcash::SaplingPaymentAddress to,
        CAmount value,
        std::array<unsigned char, ZC_MEMO_SIZE> memo = {{0}});

    void ConvertRawSaplingOutput(uint256 ovk);

    // Assumes that the value correctly corresponds to the provided UTXO.
    void AddTransparentInput(COutPoint utxo, CScript scriptPubKey, CAmount value, uint32_t nSequence = 0xffffffff);

    bool AddTransparentOutput(CTxDestination& to, CAmount value);

    void AddOpRet(CScript &s);

    bool AddOpRetLast();

    void SendChangeTo(libzcash::SaplingPaymentAddress changeAddr, uint256 ovk);

    bool SendChangeTo(CTxDestination& changeAddr);

    void SetLockTime(uint32_t time) { this->mtx.nLockTime = time; }

    TransactionBuilderResult Build();
};

#endif /* TRANSACTION_BUILDER_H */
