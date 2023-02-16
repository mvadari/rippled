//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

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

#include <ripple/app/tx/applySteps.h>
#include <ripple/app/tx/impl/ApplyContext.h>
#include <ripple/app/tx/impl/CancelCheck.h>
#include <ripple/app/tx/impl/CancelOffer.h>
#include <ripple/app/tx/impl/CashCheck.h>
#include <ripple/app/tx/impl/Change.h>
#include <ripple/app/tx/impl/CreateCheck.h>
#include <ripple/app/tx/impl/CreateOffer.h>
#include <ripple/app/tx/impl/CreateTicket.h>
#include <ripple/app/tx/impl/DeleteAccount.h>
#include <ripple/app/tx/impl/DepositPreauth.h>
#include <ripple/app/tx/impl/Escrow.h>
#include <ripple/app/tx/impl/NFTokenAcceptOffer.h>
#include <ripple/app/tx/impl/NFTokenBurn.h>
#include <ripple/app/tx/impl/NFTokenCancelOffer.h>
#include <ripple/app/tx/impl/NFTokenCreateOffer.h>
#include <ripple/app/tx/impl/NFTokenMint.h>
#include <ripple/app/tx/impl/PayChan.h>
#include <ripple/app/tx/impl/Payment.h>
#include <ripple/app/tx/impl/SetAccount.h>
#include <ripple/app/tx/impl/SetRegularKey.h>
#include <ripple/app/tx/impl/SetSignerList.h>
#include <ripple/app/tx/impl/SetTrust.h>
#include <iostream>
#include <dlfcn.h>
#include <map>

static const std::string libPath = "/Users/mvadari/Documents/plugin_transactor/cpp/build/libplugin_transactor.dylib";

namespace ripple {

typedef NotTEC (*preflightPtr)(PreflightContext const&);
typedef TER (*preclaimPtr)(PreclaimContext const&);
typedef XRPAmount (*calculateBaseFeePtr)(ReadView const& view, STTx const& tx);
typedef std::pair<TER, bool> (*applyPtr)(ApplyContext& ctx);

struct TransactorWrapper {
    preflightPtr preflight;
    preclaimPtr preclaim;
    calculateBaseFeePtr calculateBaseFee;
    applyPtr apply;
};

template <class T>
std::pair<TER, bool>
apply_helper(ApplyContext& ctx)
{
    T p(ctx);
    return p();
}

template <class T>
TransactorWrapper
transactor_helper()
{
    return {
        T::preflight,
        T::preclaim,
        T::calculateBaseFee,
        apply_helper<T>,
    };
};

TransactorWrapper
transactor_helper(std::string pathToLib)
{
    void* handle = dlopen(libPath.c_str(), RTLD_LAZY);
    return {
        (preflightPtr)dlsym(handle, "preflight"),
        (preclaimPtr)dlsym(handle, "preclaim"),
        (calculateBaseFeePtr)dlsym(handle, "calculateBaseFee"),
        (applyPtr)dlsym(handle, "apply"),
    };
};

std::map <TxType, TransactorWrapper> transactorMap {
    {ttACCOUNT_DELETE, transactor_helper<DeleteAccount>()},
    {ttACCOUNT_SET, transactor_helper<SetAccount>()},
    {ttCHECK_CANCEL, transactor_helper<CancelCheck>()},
    {ttCHECK_CASH, transactor_helper<CashCheck>()},
    {ttCHECK_CREATE, transactor_helper<CreateCheck>()},
    {ttDEPOSIT_PREAUTH, transactor_helper<DepositPreauth>()},
    {ttOFFER_CANCEL, transactor_helper<CancelOffer>()},
    {ttOFFER_CREATE, transactor_helper<CreateOffer>()},
    {ttESCROW_CREATE, transactor_helper<EscrowCreate>()},
    {ttESCROW_FINISH, transactor_helper<EscrowFinish>()},
    {ttESCROW_CANCEL, transactor_helper<EscrowCancel>()},
    {ttPAYCHAN_CLAIM, transactor_helper<PayChanClaim>()},
    {ttPAYCHAN_CREATE, transactor_helper<PayChanCreate>()},
    {ttPAYCHAN_FUND, transactor_helper<PayChanFund>()},
    {ttPAYMENT, transactor_helper<Payment>()},
    {ttREGULAR_KEY_SET, transactor_helper<SetRegularKey>()},
    {ttSIGNER_LIST_SET, transactor_helper<SetSignerList>()},
    {ttTICKET_CREATE, transactor_helper<CreateTicket>()},
    {ttTRUST_SET, transactor_helper<SetTrust>()},
    {ttAMENDMENT, transactor_helper<Change>()},
    {ttFEE, transactor_helper<Change>()},
    {ttUNL_MODIFY, transactor_helper<Change>()},
    {ttNFTOKEN_MINT, transactor_helper<NFTokenMint>()},
    {ttNFTOKEN_BURN, transactor_helper<NFTokenBurn>()},
    {ttNFTOKEN_CREATE_OFFER, transactor_helper<NFTokenCreateOffer>()},
    {ttNFTOKEN_CANCEL_OFFER, transactor_helper<NFTokenCancelOffer>()},
    {ttNFTOKEN_ACCEPT_OFFER, transactor_helper<NFTokenAcceptOffer>()},
    {ttDUMMY_TX, transactor_helper(libPath)},
};

TxConsequences
consequences_helper(PreflightContext const& ctx)
{
    // TODO: add support for Blocker and Custom TxConsequences values
    return TxConsequences(ctx.tx);
}

static std::pair<NotTEC, TxConsequences>
invoke_preflight(PreflightContext const& ctx)
{
    if (auto it = transactorMap.find(ctx.tx.getTxnType()); 
        it != transactorMap.end())
    {
        auto const tec = it->second.preflight(ctx);
        return {
            tec,
            isTesSuccess(tec) ? consequences_helper(ctx) : TxConsequences{tec}
        };
    }
    assert(false);
    return {temUNKNOWN, TxConsequences{temUNKNOWN}};
}

/* invoke_preclaim<T> uses name hiding to accomplish
    compile-time polymorphism of (presumably) static
    class functions for Transactor and derived classes.
*/
template <class T>
static TER
invoke_preclaim(PreclaimContext const& ctx)
{
    // If the transactor requires a valid account and the transaction doesn't
    // list one, preflight will have already a flagged a failure.
    auto const id = ctx.tx.getAccountID(sfAccount);

    if (id != beast::zero)
    {
        TER result = T::checkSeqProxy(ctx.view, ctx.tx, ctx.j);

        if (result != tesSUCCESS)
            return result;

        result = T::checkPriorTxAndLastLedger(ctx);

        if (result != tesSUCCESS)
            return result;

        result = T::checkFee(ctx, calculateBaseFee(ctx.view, ctx.tx));

        if (result != tesSUCCESS)
            return result;

        result = T::checkSign(ctx);

        if (result != tesSUCCESS)
            return result;
    }

    return T::preclaim(ctx);
}

static TER
invoke_preclaim(PreclaimContext const& ctx)
{
    switch (ctx.tx.getTxnType())
    {
        case ttACCOUNT_DELETE:
            return invoke_preclaim<DeleteAccount>(ctx);
        case ttACCOUNT_SET:
            return invoke_preclaim<SetAccount>(ctx);
        case ttCHECK_CANCEL:
            return invoke_preclaim<CancelCheck>(ctx);
        case ttCHECK_CASH:
            return invoke_preclaim<CashCheck>(ctx);
        case ttCHECK_CREATE:
            return invoke_preclaim<CreateCheck>(ctx);
        case ttDEPOSIT_PREAUTH:
            return invoke_preclaim<DepositPreauth>(ctx);
        case ttOFFER_CANCEL:
            return invoke_preclaim<CancelOffer>(ctx);
        case ttOFFER_CREATE:
            return invoke_preclaim<CreateOffer>(ctx);
        case ttESCROW_CREATE:
            return invoke_preclaim<EscrowCreate>(ctx);
        case ttESCROW_FINISH:
            return invoke_preclaim<EscrowFinish>(ctx);
        case ttESCROW_CANCEL:
            return invoke_preclaim<EscrowCancel>(ctx);
        case ttPAYCHAN_CLAIM:
            return invoke_preclaim<PayChanClaim>(ctx);
        case ttPAYCHAN_CREATE:
            return invoke_preclaim<PayChanCreate>(ctx);
        case ttPAYCHAN_FUND:
            return invoke_preclaim<PayChanFund>(ctx);
        case ttPAYMENT:
            return invoke_preclaim<Payment>(ctx);
        case ttREGULAR_KEY_SET:
            return invoke_preclaim<SetRegularKey>(ctx);
        case ttSIGNER_LIST_SET:
            return invoke_preclaim<SetSignerList>(ctx);
        case ttTICKET_CREATE:
            return invoke_preclaim<CreateTicket>(ctx);
        case ttTRUST_SET:
            return invoke_preclaim<SetTrust>(ctx);
        case ttAMENDMENT:
        case ttFEE:
        case ttUNL_MODIFY:
            return invoke_preclaim<Change>(ctx);
        case ttNFTOKEN_MINT:
            return invoke_preclaim<NFTokenMint>(ctx);
        case ttNFTOKEN_BURN:
            return invoke_preclaim<NFTokenBurn>(ctx);
        case ttNFTOKEN_CREATE_OFFER:
            return invoke_preclaim<NFTokenCreateOffer>(ctx);
        case ttNFTOKEN_CANCEL_OFFER:
            return invoke_preclaim<NFTokenCancelOffer>(ctx);
        case ttNFTOKEN_ACCEPT_OFFER:
            return invoke_preclaim<NFTokenAcceptOffer>(ctx);
        default:
            assert(false);
            return temUNKNOWN;
    }
}

static XRPAmount
invoke_calculateBaseFee(ReadView const& view, STTx const& tx)
{
    if (auto it = transactorMap.find(tx.getTxnType()); 
        it != transactorMap.end())
    {
        return it->second.calculateBaseFee(view, tx);
    }
    assert(false);
    return XRPAmount{0};
}

static std::pair<TER, bool>
invoke_apply(ApplyContext& ctx)
{
    if (auto it = transactorMap.find(ctx.tx.getTxnType()); 
        it != transactorMap.end())
    {
        return it->second.apply(ctx);
    }
    assert(false);
    return {temUNKNOWN, false};
}

PreflightResult
preflight(
    Application& app,
    Rules const& rules,
    STTx const& tx,
    ApplyFlags flags,
    beast::Journal j)
{
    PreflightContext const pfctx(app, tx, rules, flags, j);
    try
    {
        return {pfctx, invoke_preflight(pfctx)};
    }
    catch (std::exception const& e)
    {
        JLOG(j.fatal()) << "apply: " << e.what();
        return {pfctx, {tefEXCEPTION, TxConsequences{tx}}};
    }
}

PreclaimResult
preclaim(
    PreflightResult const& preflightResult,
    Application& app,
    OpenView const& view)
{
    std::optional<PreclaimContext const> ctx;
    if (preflightResult.rules != view.rules())
    {
        auto secondFlight = preflight(
            app,
            view.rules(),
            preflightResult.tx,
            preflightResult.flags,
            preflightResult.j);
        ctx.emplace(
            app,
            view,
            secondFlight.ter,
            secondFlight.tx,
            secondFlight.flags,
            secondFlight.j);
    }
    else
    {
        ctx.emplace(
            app,
            view,
            preflightResult.ter,
            preflightResult.tx,
            preflightResult.flags,
            preflightResult.j);
    }
    try
    {
        if (ctx->preflightResult != tesSUCCESS)
            return {*ctx, ctx->preflightResult};
        return {*ctx, invoke_preclaim(*ctx)};
    }
    catch (std::exception const& e)
    {
        JLOG(ctx->j.fatal()) << "apply: " << e.what();
        return {*ctx, tefEXCEPTION};
    }
}

XRPAmount
calculateBaseFee(ReadView const& view, STTx const& tx)
{
    return invoke_calculateBaseFee(view, tx);
}

XRPAmount
calculateDefaultBaseFee(ReadView const& view, STTx const& tx)
{
    return Transactor::calculateBaseFee(view, tx);
}

std::pair<TER, bool>
doApply(PreclaimResult const& preclaimResult, Application& app, OpenView& view)
{
    if (preclaimResult.view.seq() != view.seq())
    {
        // Logic error from the caller. Don't have enough
        // info to recover.
        return {tefEXCEPTION, false};
    }
    try
    {
        if (!preclaimResult.likelyToClaimFee)
            return {preclaimResult.ter, false};
        ApplyContext ctx(
            app,
            view,
            preclaimResult.tx,
            preclaimResult.ter,
            calculateBaseFee(view, preclaimResult.tx),
            preclaimResult.flags,
            preclaimResult.j);
        return invoke_apply(ctx);
    }
    catch (std::exception const& e)
    {
        JLOG(preclaimResult.j.fatal()) << "apply: " << e.what();
        return {tefEXCEPTION, false};
    }
}

}  // namespace ripple
