#pragma once

#include <xrpl/beast/utility/Journal.h>
#include <xrpl/ledger/ApplyView.h>
#include <xrpl/ledger/ReadView.h>
#include <xrpl/ledger/entries/SLEBase.h>
#include <xrpl/protocol/AccountID.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/LedgerFormats.h>
#include <xrpl/protocol/SeqProxy.h>

namespace xrpl {

template <typename ViewT>
class MysteryEntry : public SLEBase<ViewT, ltMYSTERY>
{
public:
    using Base = SLEBase<ViewT, ltMYSTERY>;

    // Inherit base constructors: adopt an existing SLE, or resolve one from a
    // Keylet against the view.
    using Base::Base;

    explicit MysteryEntry(
        AccountID const& owner,
        SeqProxy const& seq,
        Base::ViewRefType view,
        beast::Journal j = beast::Journal{beast::Journal::getNullSink()})
        : Base(keylet::mystery(owner, seq), view, j)
    {
    }
};

using MysteryEntryR = MysteryEntry<ReadView>;
using MysteryEntryW = MysteryEntry<ApplyView>;

}  // namespace xrpl
