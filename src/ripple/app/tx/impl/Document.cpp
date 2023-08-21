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

#include <ripple/app/tx/impl/Document.h>

#include <ripple/basics/Log.h>
#include <ripple/ledger/ApplyView.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/protocol/st.h>

namespace ripple {

/*
    Document
    ======

    TODO: add docs here
*/

//------------------------------------------------------------------------------

NotTEC
DocumentSet::preflight(PreflightContext const& ctx)
{
    if (!ctx.rules.enabled(featureDocument))
        return temDISABLED;

    if (ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    if (auto const ret = preflight1(ctx); !isTesSuccess(ret))
        return ret;

    if (!ctx.tx.isFieldPresent(sfURI) && !ctx.tx.isFieldPresent(sfData))
        return temEMPTY_DOCUMENT;

    if (ctx.tx.isFieldPresent(sfURI) && ctx.tx[sfURI].empty() &&
        ctx.tx.isFieldPresent(sfData) && ctx.tx[sfData].empty())
        return temEMPTY_DOCUMENT;

    if (auto uri = ctx.tx[~sfURI])
    {
        if (uri->length() > maxDocumentURILength)
            return temMALFORMED;
    }

    if (auto data = ctx.tx[~sfData])
    {
        if (data->length() > maxDocumentDataLength)
            return temMALFORMED;
    }

    return preflight2(ctx);
}

TER
addSLE(
    ApplyContext& ctx,
    std::shared_ptr<SLE> const& sle,
    AccountID const& owner)
{
    auto const sleAccount = ctx.view().peek(keylet::account(owner));
    if (!sleAccount)
        return tefINTERNAL;

    // Check reserve availability for new object creation
    {
        auto const balance = STAmount((*sleAccount)[sfBalance]).xrp();
        auto const reserve =
            ctx.view().fees().accountReserve((*sleAccount)[sfOwnerCount] + 1);

        if (balance < reserve)
            return tecINSUFFICIENT_RESERVE;
    }

    // Add ledger object to ledger
    ctx.view().insert(sle);

    // Add ledger object to owner's page
    {
        auto page = ctx.view().dirInsert(
            keylet::ownerDir(owner), sle->key(), describeOwnerDir(owner));
        if (!page)
            return tecDIR_FULL;
        (*sle)[sfOwnerNode] = *page;
    }
    adjustOwnerCount(ctx.view(), sleAccount, 1, ctx.journal);
    ctx.view().update(sleAccount);

    return tesSUCCESS;
}

TER
DocumentSet::doApply()
{
    // Edit ledger object if it already exists
    Keylet const docKeylet =
        keylet::document(account_, ctx_.tx[sfDocumentNumber]);
    if (auto const sleDoc = ctx_.view().peek(docKeylet))
    {
        if (auto const uri = ctx_.tx[~sfURI])
        {
            if (uri->empty())
            {
                sleDoc->makeFieldAbsent(sfURI);
            }
            else
            {
                (*sleDoc)[sfURI] = *uri;
            }
        }
        if (auto const data = ctx_.tx[~sfData])
        {
            if (data->empty())
            {
                sleDoc->makeFieldAbsent(sfData);
            }
            else
            {
                (*sleDoc)[sfData] = *data;
            }
        }
        if (!sleDoc->isFieldPresent(sfURI) && !sleDoc->isFieldPresent(sfData))
        {
            return tecEMPTY_DOCUMENT;
        }
        ctx_.view().update(sleDoc);
        return tesSUCCESS;
    }

    // Create new ledger object otherwise
    auto const sleDoc = std::make_shared<SLE>(docKeylet);
    (*sleDoc)[sfDocumentNumber] = ctx_.tx[sfDocumentNumber];
    (*sleDoc)[sfOwner] = account_;
    if (auto const uri = ctx_.tx[~sfURI]; uri.has_value() && !uri->empty())
        (*sleDoc)[sfURI] = uri.value();
    if (auto const data = ctx_.tx[~sfData]; data.has_value() && !data->empty())
        (*sleDoc)[sfData] = data.value();
    return addSLE(ctx_, sleDoc, account_);
}

NotTEC
DocumentDelete::preflight(PreflightContext const& ctx)
{
    if (!ctx.rules.enabled(featureDocument))
        return temDISABLED;

    if (ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    if (auto const ret = preflight1(ctx); !isTesSuccess(ret))
        return ret;

    return preflight2(ctx);
}

TER
DocumentDelete::deleteSLE(
    ApplyContext& ctx,
    Keylet sleKeylet,
    AccountID const owner)
{
    auto const sle = ctx.view().peek(sleKeylet);
    if (!sle)
        return tecNO_ENTRY;

    return DocumentDelete::deleteSLE(ctx.view(), sle, owner, ctx.journal);
}

TER
DocumentDelete::deleteSLE(
    ApplyView& view,
    std::shared_ptr<SLE> sle,
    AccountID const owner,
    beast::Journal j)
{
    // Remove object from owner directory
    if (!view.dirRemove(
            keylet::ownerDir(owner), (*sle)[sfOwnerNode], sle->key(), true))
    {
        JLOG(j.fatal()) << "Unable to delete Document from owner.";
        return tefBAD_LEDGER;
    }

    auto const sleOwner = view.peek(keylet::account(owner));
    adjustOwnerCount(view, sleOwner, -1, j);
    view.update(sleOwner);

    // Remove object from ledger
    view.erase(sle);
    return tesSUCCESS;
}

TER
DocumentDelete::doApply()
{
    AccountID const account = ctx_.tx[sfAccount];
    auto const docKeylet = keylet::document(account, ctx_.tx[sfDocumentNumber]);

    return deleteSLE(ctx_, docKeylet, account_);
}

}  // namespace ripple
