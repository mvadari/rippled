//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2023 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <ripple/app/tx/impl/Relay.h>

#include <ripple/app/tx/impl/ApplyContext.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/ApplyView.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/protocol/st.h>

namespace ripple {

NotTEC
Relay::preflight(PreflightContext const& ctx)
{
    if (auto const ret = preflight1(ctx); !isTesSuccess(ret))
        return ret;

    if (ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const blob = ctx.tx[sfSignature];

    SerialIter sitTrans(blob);

    std::shared_ptr<STTx const> stpTrans;

    try
    {
        stpTrans = std::make_shared<STTx const>(std::ref(sitTrans));
    }
    catch (std::exception& e)
    {
        JLOG(ctx.j.trace()) << e.what();
        return temINVALID;
    }

    if (auto const ret =
            ripple::preflight(
                ctx.app, ctx.rules, *stpTrans.get(), ctx.flags, ctx.j)
                .ter;
        !isTesSuccess(ret))
        return ret;

    return preflight2(ctx);
}

TER
applyTx(ApplyContext& ctx_, Slice const& blob, beast::Journal j_)
{
    SerialIter sitTrans(blob);

    std::shared_ptr<STTx const> stpTrans;
    stpTrans = std::make_shared<STTx const>(std::ref(sitTrans));
    JLOG(j_.trace()) << stpTrans->getFullText();

    auto const preflightResult = ripple::preflight(
        ctx_.app, ctx_.view().rules(), *stpTrans.get(), ctx_.flags(), j_);
    if (!isTesSuccess(preflightResult.ter))
        return preflightResult.ter;

    auto const preclaimResult =
        ripple::preclaim(preflightResult, ctx_.app, ctx_.openView());
    if (!isTesSuccess(preclaimResult.ter))
        return preclaimResult.ter;

    if (preclaimResult.view.seq() != ctx_.view().seq())
    {
        // Logic error from the caller. Don't have enough
        // info to recover.
        return tefEXCEPTION;
    }
    try
    {
        if (!preclaimResult.likelyToClaimFee)
            return preclaimResult.ter;
        OpenView sandbox = OpenView(ctx_.openView());
        ApplyContext applyCtx(
            ctx_.app,
            sandbox,
            preclaimResult.tx,
            preclaimResult.ter,
            calculateBaseFee(sandbox, preclaimResult.tx),
            preclaimResult.flags,
            j_);
        auto const result = invoke_apply(applyCtx);
        applyCtx.viewImpl().apply(ctx_.rawView());

        auto const sleAccount = ctx_.view().peek(
            keylet::account(stpTrans->getAccountID(sfAccount)));
        sleAccount->setFieldAmount(
            sfBalance,
            sleAccount->getFieldAmount(sfBalance) +
                stpTrans->getFieldAmount(sfFee));
        return result.first;
    }
    catch (std::exception const& e)
    {
        JLOG(j_.fatal()) << "Relay apply: " << e.what();
        return tefEXCEPTION;
    }
}

TER
Relay::doApply()
{
    if (auto const ret = applyTx(ctx_, ctx_.tx[sfSignature], j_);
        !isTesSuccess(ret))
        return ret;

    // if (auto const ret = applyTx(ctx_, ctx_.tx[sfData], j_);
    // !isTesSuccess(ret))
    //     return ret;

    return tesSUCCESS;
}

}  // namespace ripple
