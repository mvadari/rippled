#include <xrpl/tx/transactors/mystery/MysteryCreate.h>

#include <xrpl/ledger/ApplyView.h>
#include <xrpl/ledger/helpers/AccountRootHelpers.h>
#include <xrpl/ledger/helpers/DirectoryHelpers.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/Protocol.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/STAmount.h>
#include <xrpl/protocol/STLedgerEntry.h>
#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol/TER.h>
#include <xrpl/protocol/XRPAmount.h>
#include <xrpl/tx/Transactor.h>

#include <memory>

namespace xrpl {

/*
    Mystery
    =======

    A Mystery locks XRP in the ledger. The owner stores the SHA-512Half of a
    secret phrase in Digest. The first account to submit the phrase in a
    MysteryGuess gets the XRP.
*/

TxConsequences
MysteryCreate::makeTxConsequences(PreflightContext const& ctx)
{
    auto const amount = ctx.tx[sfAmount];
    return TxConsequences{ctx.tx, isXRP(amount) ? amount.xrp() : beast::kZero};
}

NotTEC
MysteryCreate::preflight(PreflightContext const& ctx)
{
    // TODO: Check that:
    // 1. Amount is XRP and greater than zero. If it is not, return
    //    temBAD_AMOUNT.
    // 2. Digest is not zero. If it is, return temMALFORMED.
    // 3. If Data is present, it is not empty and is not longer than
    //    kMaxMysteryDataLength. If it is empty or too long, return
    //    temMALFORMED.
    return tesSUCCESS;
}

TER
MysteryCreate::preclaim(PreclaimContext const& ctx)
{
    // TODO (optional): Add any checks that need the ledger state.
    return tesSUCCESS;
}

TER
MysteryCreate::doApply()
{
    auto const sleOwner = ctx_.view().peek(keylet::account(accountID_));
    if (!sleOwner)
        return tefINTERNAL;  // LCOV_EXCL_LINE

    // The owner must cover the reserve for the new object, and must still
    // cover it after the prize is locked.
    auto const balance = STAmount((*sleOwner)[sfBalance]).xrp();
    auto const reserve = accountReserve(ctx_.view(), sleOwner, j_, {.ownerCountDelta = 1});
    if (balance < reserve)
        return tecINSUFFICIENT_RESERVE;

    auto const amount = ctx_.tx[sfAmount].xrp();
    if (balance - amount < reserve)
        return tecUNFUNDED;

    // Use the value from the sequence or ticket. See SeqProxy.h.
    auto const seqProxy = ctx_.tx.getSeqProxy();
    auto const mystery = std::make_shared<SLE>(keylet::mystery(accountID_, seqProxy));
    (*mystery)[sfAccount] = accountID_;
    (*mystery)[sfSequence] = seqProxy.value();
    (*mystery)[sfAmount] = STAmount{amount};
    (*mystery)[sfDigest] = ctx_.tx[sfDigest];
    (*mystery)[~sfData] = ctx_.tx[~sfData];
    ctx_.view().insert(mystery);

    auto const page = ctx_.view().dirInsert(
        keylet::ownerDir(accountID_), mystery->key(), describeOwnerDir(accountID_));
    if (!page)
        return tecDIR_FULL;  // LCOV_EXCL_LINE
    (*mystery)[sfOwnerNode] = *page;

    (*sleOwner)[sfBalance] = (*sleOwner)[sfBalance] - amount;
    increaseOwnerCount(ctx_.view(), sleOwner, {}, 1, j_);
    ctx_.view().update(sleOwner);

    return tesSUCCESS;
}

void
MysteryCreate::visitInvariantEntry(bool, SLE::ConstRef, SLE::ConstRef)
{
    // No transaction-specific invariants yet (future work).
}

bool
MysteryCreate::finalizeInvariants(
    STTx const&,
    TER,
    XRPAmount,
    ReadView const&,
    beast::Journal const&)
{
    // No transaction-specific invariants yet (future work).
    return true;
}

}  // namespace xrpl
