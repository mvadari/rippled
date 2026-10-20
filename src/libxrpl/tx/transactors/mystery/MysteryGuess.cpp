#include <xrpl/tx/transactors/mystery/MysteryGuess.h>

#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/Protocol.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/STLedgerEntry.h>
#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol/SeqProxy.h>
#include <xrpl/protocol/TER.h>
#include <xrpl/protocol/XRPAmount.h>
#include <xrpl/protocol/digest.h>
#include <xrpl/tx/Transactor.h>
#include <xrpl/tx/transactors/mystery/MysteryCancel.h>

namespace xrpl {

NotTEC
MysteryGuess::preflight(PreflightContext const& ctx)
{
    auto const guess = ctx.tx[sfGuess];
    if (guess.empty() || guess.size() > kMaxMysteryGuessLength)
        return temMALFORMED;

    return tesSUCCESS;
}

TER
MysteryGuess::preclaim(PreclaimContext const& ctx)
{
    // TODO: Find the Mystery from the Owner and OfferSequence fields, then
    // check that:
    // 1. The Mystery exists. If it does not, return tecNO_ENTRY.
    // 2. The guesser does not own the Mystery. If they do, return
    //    tecNO_PERMISSION.
    // 3. The SHA-512Half of the Guess is equal to the Digest of the Mystery.
    //    If it is not, return tecNO_PERMISSION.
    return tesSUCCESS;
}

TER
MysteryGuess::doApply()
{
    // TODO: Pay the prize to the guesser and delete the Mystery.
    // Hint: MysteryCancel does almost the same thing.
    return tesSUCCESS;
}

void
MysteryGuess::visitInvariantEntry(bool, SLE::ConstRef, SLE::ConstRef)
{
    // No transaction-specific invariants yet (future work).
}

bool
MysteryGuess::finalizeInvariants(
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
