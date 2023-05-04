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

#include <ripple/protocol/TxFormats.h>
#include <ripple/protocol/jss.h>
#include <list>
#include <map>
#include <dlfcn.h>

namespace ripple {

struct FakeSOElement {
    int fieldCode;
    ripple::SOEStyle style;
};

typedef std::vector<FakeSOElement> (*getTxFormatPtr)();
typedef char const* (*getTxNamePtr)();
typedef std::uint16_t (*getTxTypePtr)();
typedef std::string (*getTTNamePtr)();

struct TxFormatsWrapper {
    char const* name;
    std::uint16_t type;
    std::vector<SOElement> uniqueFields;
    std::initializer_list<SOElement> commonFields;
};

std::map<std::string, std::uint16_t> txTypes{
    {"ttPAYMENT", 0},
    {"ttESCROW_CREATE", 1},
    {"ttESCROW_FINISH", 2},
    {"ttACCOUNT_SET", 3},
    {"ttESCROW_CANCEL", 4},
    {"ttREGULAR_KEY_SET", 5},
    {"ttOFFER_CREATE", 7},
    {"ttOFFER_CANCEL", 8},
    {"ttTICKET_CREATE", 10},
    {"ttSIGNER_LIST_SET", 12},
    {"ttPAYCHAN_CREATE", 13},
    {"ttPAYCHAN_FUND", 14},
    {"ttPAYCHAN_CLAIM", 15},
    {"ttCHECK_CREATE", 16},
    {"ttCHECK_CASH", 17},
    {"ttCHECK_CANCEL", 18},
    {"ttDEPOSIT_PREAUTH", 19},
    // {"ttTRUST_SET", 20},
    {"ttACCOUNT_DELETE", 21},
    {"ttHOOK_SET", 22},
    {"ttNFTOKEN_MINT", 25},
    {"ttNFTOKEN_BURN", 26},
    {"ttNFTOKEN_CREATE_OFFER", 27},
    {"ttNFTOKEN_CANCEL_OFFER", 28},
    {"ttNFTOKEN_ACCEPT_OFFER", 29},
    {"ttDUMMY_TX", 30},
    {"ttAMENDMENT", 100},
    {"ttFEE", 101},
    {"ttUNL_MODIFY", 102},
};

std::uint16_t
getTxTypeFromName(std::string name)
{
    if (auto it = txTypes.find(name);
        it != txTypes.end())
    {
        return it->second;
    }
    assert(false);
    return -1;
}

void
addToTxTypes(std::string dynamicLib)
{
    void* handle = dlopen(dynamicLib.c_str(), RTLD_LAZY);
    auto const type = ((getTxTypePtr)dlsym(handle, "getTxName"))();
    auto const ttName = ((getTTNamePtr)dlsym(handle, "getTTName"))();
    txTypes.insert({ttName, type});
}

const std::initializer_list<SOElement> commonFields {
    {sfTransactionType, soeREQUIRED},
    {sfFlags, soeOPTIONAL},
    {sfSourceTag, soeOPTIONAL},
    {sfAccount, soeREQUIRED},
    {sfSequence, soeREQUIRED},
    {sfPreviousTxnID, soeOPTIONAL},  // emulate027
    {sfLastLedgerSequence, soeOPTIONAL},
    {sfAccountTxnID, soeOPTIONAL},
    {sfFee, soeREQUIRED},
    {sfOperationLimit, soeOPTIONAL},
    {sfMemos, soeOPTIONAL},
    {sfSigningPubKey, soeREQUIRED},
    {sfTxnSignature, soeOPTIONAL},
    {sfSigners, soeOPTIONAL},  // submit_multisigned
    {sfNetworkID, soeOPTIONAL},
};

std::initializer_list<TxFormatsWrapper> txFormatsList{
    {
        jss::AccountSet,
        getTxTypeFromName("ttACCOUNT_SET"),
        {
            {sfEmailHash, soeOPTIONAL},
            {sfWalletLocator, soeOPTIONAL},
            {sfWalletSize, soeOPTIONAL},
            {sfMessageKey, soeOPTIONAL},
            {sfDomain, soeOPTIONAL},
            {sfTransferRate, soeOPTIONAL},
            {sfSetFlag, soeOPTIONAL},
            {sfClearFlag, soeOPTIONAL},
            {sfTickSize, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
            {sfNFTokenMinter, soeOPTIONAL},
        },
        commonFields},
    // {
    //     jss::TrustSet,
    //     getTxTypeFromName("ttTRUST_SET"),
    //     {
    //         {sfLimitAmount, soeOPTIONAL},
    //         {sfQualityIn, soeOPTIONAL},
    //         {sfQualityOut, soeOPTIONAL},
    //         {sfTicketSequence, soeOPTIONAL},
    //     },
    //     commonFields},
    {jss::OfferCreate,
        getTxTypeFromName("ttOFFER_CREATE"),
        {
            {sfTakerPays, soeREQUIRED},
            {sfTakerGets, soeREQUIRED},
            {sfExpiration, soeOPTIONAL},
            {sfOfferSequence, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::OfferCancel,
        getTxTypeFromName("ttOFFER_CANCEL"),
        {
            {sfOfferSequence, soeREQUIRED},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::SetRegularKey,
        getTxTypeFromName("ttREGULAR_KEY_SET"),
        {
            {sfRegularKey, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::Payment,
        getTxTypeFromName("ttPAYMENT"),
        {
            {sfDestination, soeREQUIRED},
            {sfAmount, soeREQUIRED},
            {sfSendMax, soeOPTIONAL},
            {sfPaths, soeDEFAULT},
            {sfInvoiceID, soeOPTIONAL},
            {sfDestinationTag, soeOPTIONAL},
            {sfDeliverMin, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::EscrowCreate,
        getTxTypeFromName("ttESCROW_CREATE"),
        {
            {sfDestination, soeREQUIRED},
            {sfAmount, soeREQUIRED},
            {sfCondition, soeOPTIONAL},
            {sfCancelAfter, soeOPTIONAL},
            {sfFinishAfter, soeOPTIONAL},
            {sfDestinationTag, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::EscrowFinish,
        getTxTypeFromName("ttESCROW_FINISH"),
        {
            {sfOwner, soeREQUIRED},
            {sfOfferSequence, soeREQUIRED},
            {sfFulfillment, soeOPTIONAL},
            {sfCondition, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::EscrowCancel,
        getTxTypeFromName("ttESCROW_CANCEL"),
        {
            {sfOwner, soeREQUIRED},
            {sfOfferSequence, soeREQUIRED},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::EnableAmendment,
        getTxTypeFromName("ttAMENDMENT"),
        {
            {sfLedgerSequence, soeREQUIRED},
            {sfAmendment, soeREQUIRED},
        },
        commonFields},
    {
        jss::SetFee,
        getTxTypeFromName("ttFEE"),
        {
            {sfLedgerSequence, soeOPTIONAL},
            // Old version uses raw numbers
            {sfBaseFee, soeOPTIONAL},
            {sfReferenceFeeUnits, soeOPTIONAL},
            {sfReserveBase, soeOPTIONAL},
            {sfReserveIncrement, soeOPTIONAL},
            // New version uses Amounts
            {sfBaseFeeDrops, soeOPTIONAL},
            {sfReserveBaseDrops, soeOPTIONAL},
            {sfReserveIncrementDrops, soeOPTIONAL},
        },
        commonFields},
    {
        jss::UNLModify,
        getTxTypeFromName("ttUNL_MODIFY"),
        {
            {sfUNLModifyDisabling, soeREQUIRED},
            {sfLedgerSequence, soeREQUIRED},
            {sfUNLModifyValidator, soeREQUIRED},
        },
        commonFields},
    {
        jss::TicketCreate,
        getTxTypeFromName("ttTICKET_CREATE"),
        {
            {sfTicketCount, soeREQUIRED},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},

    // The SignerEntries are optional because a SignerList is deleted by
    // setting the SignerQuorum to zero and omitting SignerEntries.
    {
        jss::SignerListSet,
        getTxTypeFromName("ttSIGNER_LIST_SET"),
        {
            {sfSignerQuorum, soeREQUIRED},
            {sfSignerEntries, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::PaymentChannelCreate,
        getTxTypeFromName("ttPAYCHAN_CREATE"),
        {
            {sfDestination, soeREQUIRED},
            {sfAmount, soeREQUIRED},
            {sfSettleDelay, soeREQUIRED},
            {sfPublicKey, soeREQUIRED},
            {sfCancelAfter, soeOPTIONAL},
            {sfDestinationTag, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::PaymentChannelFund,
        getTxTypeFromName("ttPAYCHAN_FUND"),
        {
            {sfChannel, soeREQUIRED},
            {sfAmount, soeREQUIRED},
            {sfExpiration, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::PaymentChannelClaim,
        getTxTypeFromName("ttPAYCHAN_CLAIM"),
        {
            {sfChannel, soeREQUIRED},
            {sfAmount, soeOPTIONAL},
            {sfBalance, soeOPTIONAL},
            {sfSignature, soeOPTIONAL},
            {sfPublicKey, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::CheckCreate,
        getTxTypeFromName("ttCHECK_CREATE"),
        {
            {sfDestination, soeREQUIRED},
            {sfSendMax, soeREQUIRED},
            {sfExpiration, soeOPTIONAL},
            {sfDestinationTag, soeOPTIONAL},
            {sfInvoiceID, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::CheckCash,
        getTxTypeFromName("ttCHECK_CASH"),
        {
            {sfCheckID, soeREQUIRED},
            {sfAmount, soeOPTIONAL},
            {sfDeliverMin, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::CheckCancel,
        getTxTypeFromName("ttCHECK_CANCEL"),
        {
            {sfCheckID, soeREQUIRED},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::AccountDelete,
        getTxTypeFromName("ttACCOUNT_DELETE"),
        {
            {sfDestination, soeREQUIRED},
            {sfDestinationTag, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::DepositPreauth,
        getTxTypeFromName("ttDEPOSIT_PREAUTH"),
        {
            {sfAuthorize, soeOPTIONAL},
            {sfUnauthorize, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::NFTokenMint,
        getTxTypeFromName("ttNFTOKEN_MINT"),
        {
            {sfNFTokenTaxon, soeREQUIRED},
            {sfTransferFee, soeOPTIONAL},
            {sfIssuer, soeOPTIONAL},
            {sfURI, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::NFTokenBurn,
        getTxTypeFromName("ttNFTOKEN_BURN"),
        {
            {sfNFTokenID, soeREQUIRED},
            {sfOwner, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::NFTokenCreateOffer,
        getTxTypeFromName("ttNFTOKEN_CREATE_OFFER"),
        {
            {sfNFTokenID, soeREQUIRED},
            {sfAmount, soeREQUIRED},
            {sfDestination, soeOPTIONAL},
            {sfOwner, soeOPTIONAL},
            {sfExpiration, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::NFTokenCancelOffer,
        getTxTypeFromName("ttNFTOKEN_CANCEL_OFFER"),
        {
            {sfNFTokenOffers, soeREQUIRED},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::NFTokenAcceptOffer,
        getTxTypeFromName("ttNFTOKEN_ACCEPT_OFFER"),
        {
            {sfNFTokenBuyOffer, soeOPTIONAL},
            {sfNFTokenSellOffer, soeOPTIONAL},
            {sfNFTokenBrokerFee, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
    {
        jss::DummyTx,
        getTxTypeFromName("ttDUMMY_TX"),
        {
            {sfRegularKey, soeOPTIONAL},
            {sfTicketSequence, soeOPTIONAL},
        },
        commonFields},
};

std::vector<TxFormatsWrapper> txFormatsList2{};

std::vector<SOElement>
convertToUniqueFields(std::vector<FakeSOElement> txFormat)
{
    std::vector<SOElement> uniqueFields;
    for (auto &param : txFormat)
    {
        uniqueFields.push_back({SField::getField(param.fieldCode), param.style});
    }
    return uniqueFields;
}

void
addToTxFormats(std::uint16_t type, std::string dynamicLib)
{
    void* handle = dlopen(dynamicLib.c_str(), RTLD_LAZY);
    auto const name = ((getTxNamePtr)dlsym(handle, "getTxName"))();
    auto const txFormat = ((getTxFormatPtr)dlsym(handle, "getTxFormat"))();
    txFormatsList2.push_back({
        name,
        type,
        convertToUniqueFields(txFormat),
        commonFields});
}

TxFormats::TxFormats()
{
    // Fields shared by all txFormats:
    for (auto &e: txFormatsList)
    {
        std::vector<SOElement> uniqueFields(e.uniqueFields);
        add(e.name, e.type, uniqueFields, e.commonFields);
    }
    for (auto &e: txFormatsList2)
    {
        add(e.name, e.type, e.uniqueFields, e.commonFields);
    }
}

TxFormats const&
TxFormats::getInstance()
{
    static TxFormats const instance;
    return instance;
}

}  // namespace ripple
