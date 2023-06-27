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

#ifndef RIPPLE_PLUGIN_EXPORT_H_INCLUDED
#define RIPPLE_PLUGIN_EXPORT_H_INCLUDED

#include <ripple/app/tx/TxConsequences.h>
#include <ripple/app/tx/impl/Transactor.h>
#include <ripple/plugin/plugin.h>
#include <ripple/protocol/impl/STVar.h>
#include <vector>

namespace ripple {

typedef Container<STypeExport> (*getSTypesPtr)();

typedef Container<SFieldExport> (*getSFieldsPtr)();

typedef Container<LedgerObjectExport> (*getLedgerObjectsPtr)();

typedef Container<TERExport> (*getTERcodesPtr)();

// Transactors

typedef TxConsequences (*makeTxConsequencesPtr)(
    PreflightContext const&);  // TODO: fix
typedef NotTEC (*preflightPtr)(PreflightContext const&);
typedef TER (*preclaimPtr)(PreclaimContext const&);
typedef XRPAmount (*calculateBaseFeePtr)(ReadView const& view, STTx const& tx);
typedef TER (*doApplyPtr)(
    ApplyContext& ctx,
    XRPAmount mPriorBalance,
    XRPAmount mSourceBalance);

// less common ones
typedef NotTEC (
    *checkSeqProxyPtr)(ReadView const& view, STTx const& tx, beast::Journal j);
typedef NotTEC (*checkPriorTxAndLastLedgerPtr)(PreclaimContext const& ctx);
typedef TER (*checkFeePtr)(PreclaimContext const& ctx, XRPAmount baseFee);
typedef NotTEC (*checkSignPtr)(PreclaimContext const& ctx);

struct TransactorExport
{
    char const* txName;
    std::uint16_t txType;
    Container<SOElementExport> txFormat;
    Transactor::ConsequencesFactoryType consequencesFactoryType;
    makeTxConsequencesPtr makeTxConsequences;
    preflightPtr preflight;
    preclaimPtr preclaim;
    calculateBaseFeePtr calculateBaseFee;
    doApplyPtr doApply;
    checkSeqProxyPtr checkSeqProxy;
    checkPriorTxAndLastLedgerPtr checkPriorTxAndLastLedger;
    checkFeePtr checkFee;
    checkSignPtr checkSign;
};
typedef Container<TransactorExport> (*getTransactorsPtr)();

}  // namespace ripple

#endif
