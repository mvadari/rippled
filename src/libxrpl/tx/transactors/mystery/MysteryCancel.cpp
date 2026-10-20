#include <xrpl/tx/transactors/mystery/MysteryCancel.h>

#include <xrpl/basics/Log.h>
#include <xrpl/ledger/ApplyView.h>
#include <xrpl/ledger/helpers/AccountRootHelpers.h>
#include <xrpl/protocol/AccountID.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/STLedgerEntry.h>
#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol/SeqProxy.h>
#include <xrpl/protocol/TER.h>
#include <xrpl/protocol/XRPAmount.h>
#include <xrpl/tx/Transactor.h>

namespace xrpl {

NotTEC
MysteryCancel::preflight(PreflightContext const& ctx)
{
    return tesSUCCESS;
}

TER
MysteryCancel::preclaim(PreclaimContext const& ctx)
{
    auto const seqProxy = SeqProxy::rawSequence(ctx.tx[sfOfferSequence]);
    if (!ctx.view.exists(keylet::mystery(ctx.tx[sfAccount], seqProxy)))
        return tecNO_ENTRY;

    return tesSUCCESS;
}

TER
MysteryCancel::deleteMystery(
    ApplyView& view,
    SLE::Ref mystery,
    AccountID const& recipient,
    beast::Journal j)
{
    AccountID const owner = (*mystery)[sfAccount];

    if (!view.dirRemove(keylet::ownerDir(owner), (*mystery)[sfOwnerNode], mystery->key(), true))
    {
        // LCOV_EXCL_START
        JLOG(j.fatal()) << "Unable to delete Mystery from owner.";
        return tefBAD_LEDGER;
        // LCOV_EXCL_STOP
    }

    auto const sleOwner = view.peek(keylet::account(owner));
    auto const sleRecipient = view.peek(keylet::account(recipient));
    if (!sleOwner || !sleRecipient)
        return tefINTERNAL;  // LCOV_EXCL_LINE

    decreaseOwnerCountForObject(view, sleOwner, mystery, 1, j);
    view.update(sleOwner);

    (*sleRecipient)[sfBalance] = (*sleRecipient)[sfBalance] + (*mystery)[sfAmount];
    view.update(sleRecipient);

    view.erase(mystery);
    return tesSUCCESS;
}

TER
MysteryCancel::doApply()
{
    auto const seqProxy = SeqProxy::rawSequence(ctx_.tx[sfOfferSequence]);
    auto const mystery = ctx_.view().peek(keylet::mystery(accountID_, seqProxy));
    if (!mystery)
        return tecINTERNAL;  // LCOV_EXCL_LINE

    return deleteMystery(ctx_.view(), mystery, accountID_, j_);
}

void
MysteryCancel::visitInvariantEntry(bool, SLE::ConstRef, SLE::ConstRef)
{
    // No transaction-specific invariants yet (future work).
}

bool
MysteryCancel::finalizeInvariants(
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
