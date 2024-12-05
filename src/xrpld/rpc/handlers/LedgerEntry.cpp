//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2014 Ripple Labs Inc.

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

#include <xrpld/app/main/Application.h>
#include <xrpld/app/misc/CredentialHelpers.h>
#include <xrpld/ledger/ReadView.h>
#include <xrpld/rpc/Context.h>
#include <xrpld/rpc/GRPCHandlers.h>
#include <xrpld/rpc/detail/RPCHelpers.h>
#include <xrpl/basics/StringUtilities.h>
#include <xrpl/basics/strHex.h>
#include <xrpl/beast/core/LexicalCast.h>
#include <xrpl/json/json_errors.h>
#include <xrpl/protocol/ErrorCodes.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/RPCErr.h>
#include <xrpl/protocol/STXChainBridge.h>
#include <xrpl/protocol/jss.h>
#include <functional>

namespace ripple {

std::nullopt_t
missingFieldError(Json::Value& json, std::string& err, Json::StaticString field)
{
    auto const& error = RPC::missing_field_message(std::string(field.c_str()));
    json[jss::error] = err;
    json[jss::error_code] = rpcINVALID_PARAMS;
    json[jss::error_message] = error;
    return std::nullopt;
}

std::nullopt_t
invalidFieldError(
    Json::Value& json,
    std::string& err,
    Json::StaticString field,
    std::string const& type)
{
    auto const& error =
        RPC::expected_field_message(std::string(field.c_str()), type);
    json[jss::error] = err;
    json[jss::error_code] = rpcINVALID_PARAMS;
    json[jss::error_message] = error;
    return std::nullopt;
}

std::optional<AccountID>
parseAccountID(Json::Value const& param)
{
    if (!param.isString())
        return std::nullopt;

    auto const account = parseBase58<AccountID>(param.asString());
    if (!account || account->isZero())
    {
        return std::nullopt;
    }

    return account;
}

std::optional<Blob>
parseHexBlob(Json::Value const& param, std::size_t maxLength)
{
    if (!param.isString())
        return std::nullopt;

    auto const blob = strUnHex(param.asString());
    if (!blob || blob->empty() || blob->size() > maxLength)
        return std::nullopt;

    return blob;
}

bool
hasRequired(
    const Json::Value& params,
    std::initializer_list<Json::StaticString> fields)
{
    for (const auto& field : fields)
    {
        if (!params.isMember(field))
        {
            // TODO: use `missingFieldError`
            return false;
        }
    }
    return true;
}

std::optional<std::uint32_t>
parseUInt32(Json::Value const& param)
{
    if (param.isUInt() || (param.isInt() && param.asInt() >= 0))
        return std::make_optional(param.asUInt());

    if (param.isString())
    {
        std::uint32_t v;
        if (beast::lexicalCastChecked(v, param.asString()))
            return std::make_optional(v);
    }

    return std::nullopt;
}

std::optional<uint256>
parseUInt256(Json::Value const& param)
{
    uint256 uNodeIndex;
    if (!param.isString() || !uNodeIndex.parseHex(param.asString()))
    {
        return std::nullopt;
    }

    return uNodeIndex;
}

std::optional<uint256>
parseIndex(Json::Value const& params, Json::Value& jvResult)
{
    if (auto const uNodeIndex = parseUInt256(params))
    {
        return uNodeIndex;
    }

    // TODO: use `invalidFieldError`
    jvResult[jss::error] = "malformedRequest";
    return std::nullopt;
}

std::optional<uint256>
parseAccountRoot(Json::Value const& params, Json::Value& jvResult)
{
    if (auto const account = parseAccountID(params))
    {
        return keylet::account(*account).key;
    }

    jvResult[jss::error] = "malformedAddress";
    return std::nullopt;
}

std::optional<uint256>
parseCheck(Json::Value const& params, Json::Value& jvResult)
{
    return parseIndex(params, jvResult);
}

static STArray
parseAuthorizeCredentials(Json::Value const& jv)
{
    if (!jv.isArray())
        return {};
    STArray arr(sfAuthorizeCredentials, jv.size());
    for (auto const& jo : jv)
    {
        if (!jo.isObject() ||
            !hasRequired(jo, {jss::issuer, jss::credential_type}) ||
            !jo[jss::credential_type].isString())
            return {};

        auto const issuer = parseAccountID(jo[jss::issuer]);
        if (!issuer || !*issuer)
            return {};

        auto const credentialType =
            parseHexBlob(jo[jss::credential_type], maxCredentialTypeLength);
        if (!credentialType)
            return {};

        auto credential = STObject::makeInnerObject(sfCredential);
        credential.setAccountID(sfIssuer, *issuer);
        credential.setFieldVL(sfCredentialType, *credentialType);
        arr.push_back(std::move(credential));
    }

    return arr;
}

std::optional<uint256>
parseDepositPreauth(Json::Value const& dp, Json::Value& jvResult)
{
    if (!dp.isObject())
    {
        return parseIndex(dp, jvResult);
    }

    if (!dp.isMember(jss::owner) ||
        (dp.isMember(jss::authorized) ==
         dp.isMember(jss::authorized_credentials)))
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    auto const owner = parseAccountID(dp[jss::owner]);
    if (!owner)
    {
        jvResult[jss::error] = "malformedOwner";
        return std::nullopt;
    }

    if (dp.isMember(jss::authorized))
    {
        if (auto const authorized = parseAccountID(dp[jss::authorized]))
        {
            return keylet::depositPreauth(*owner, *authorized).key;
        }

        jvResult[jss::error] = "malformedAuthorized";
        return std::nullopt;
    }

    auto const& ac(dp[jss::authorized_credentials]);
    STArray const arr = parseAuthorizeCredentials(ac);
    if (arr.empty() || (arr.size() > maxCredentialsArraySize))
    {
        jvResult[jss::error] = "malformedAuthorizedCredentials";
        return std::nullopt;
    }

    auto const& sorted = credentials::makeSorted(arr);
    if (sorted.empty())
    {
        jvResult[jss::error] = "malformedAuthorizedCredentials";
        return std::nullopt;
    }

    return keylet::depositPreauth(*owner, sorted).key;
}

std::optional<uint256>
parseDirectoryNode(Json::Value const& params, Json::Value& jvResult)
{
    if (!params.isObject())
    {
        return parseIndex(params, jvResult);
    }

    if (params.isMember(jss::sub_index) && !params[jss::sub_index].isIntegral())
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    if (params.isMember(jss::owner) == params.isMember(jss::dir_root))
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    std::uint64_t uSubIndex = params.get(jss::sub_index, 0).asUInt();

    if (params.isMember(jss::dir_root))
    {
        if (auto const uDirRoot = parseUInt256(params[jss::dir_root]))
        {
            return keylet::page(*uDirRoot, uSubIndex).key;
        }

        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    if (params.isMember(jss::owner))
    {
        auto const ownerID = parseAccountID(params[jss::owner]);
        if (!ownerID)
        {
            jvResult[jss::error] = "malformedOwner";
            return std::nullopt;
        }

        return keylet::page(keylet::ownerDir(*ownerID), uSubIndex).key;
    }

    jvResult[jss::error] = "malformedRequest";
    return std::nullopt;
}

std::optional<uint256>
parseEscrow(Json::Value const& params, Json::Value& jvResult)
{
    if (!params.isObject())
    {
        return parseIndex(params, jvResult);
    }

    if (!hasRequired(params, {jss::owner, jss::seq}))
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    auto const id = parseAccountID(params[jss::owner]);
    if (!id)
    {
        jvResult[jss::error] = "malformedOwner";
        return std::nullopt;
    }

    auto const seq = parseUInt32(params[jss::seq]);
    if (!seq)
    {
        jvResult[jss::error] = "malformedSeq";
        return std::nullopt;
    }

    return keylet::escrow(*id, params[jss::seq].asUInt()).key;
}

std::optional<uint256>
parseAmendments(Json::Value const& params, Json::Value& jvResult)
{
    return parseIndex(params, jvResult);
}

std::optional<uint256>
parseFeeSettings(Json::Value const& params, Json::Value& jvResult)
{
    return parseIndex(params, jvResult);
}

std::optional<uint256>
parseSignerList(Json::Value const& params, Json::Value& jvResult)
{
    return parseIndex(params, jvResult);
}

std::optional<uint256>
parseNegativeUNL(Json::Value const& params, Json::Value& jvResult)
{
    return parseIndex(params, jvResult);
}

std::optional<uint256>
parseLedgerHashes(Json::Value const& params, Json::Value& jvResult)
{
    return parseIndex(params, jvResult);
}

std::optional<uint256>
parseOffer(Json::Value const& params, Json::Value& jvResult)
{
    if (!params.isObject())
    {
        return parseIndex(params, jvResult);
    }

    if (!hasRequired(params, {jss::account, jss::seq}))
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    auto const id = parseAccountID(params[jss::account]);
    if (!id)
    {
        jvResult[jss::error] = "malformedAccount";
        return std::nullopt;
    }

    auto const seq = parseUInt32(params[jss::seq]);
    if (!seq)
    {
        jvResult[jss::error] = "malformedSeq";
        return std::nullopt;
    }

    return keylet::offer(*id, *seq).key;
}

std::optional<uint256>
parsePayChannel(Json::Value const& params, Json::Value& jvResult)
{
    return parseIndex(params, jvResult);
}

std::optional<uint256>
parseRippleState(Json::Value const& jvRippleState, Json::Value& jvResult)
{
    Currency uCurrency;

    if (!jvRippleState.isObject())
    {
        return parseIndex(jvRippleState, jvResult);
    }

    if (!hasRequired(jvRippleState, {jss::currency, jss::accounts}))
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    if (!jvRippleState[jss::accounts].isArray() ||
        jvRippleState[jss::accounts].size() != 2)
    {
        jvResult[jss::error] = "malformedAccounts";
        return std::nullopt;
    }

    auto const id1 = parseAccountID(jvRippleState[jss::accounts][0u]);
    auto const id2 = parseAccountID(jvRippleState[jss::accounts][1u]);
    if (!id1 || !id2)
    {
        jvResult[jss::error] = "malformedAccount";
        return std::nullopt;
    }
    if (id1 == id2)
    {
        jvResult[jss::error] = "badRequest";
        return std::nullopt;
    }

    if (!jvRippleState[jss::currency].isString() ||
        !to_currency(uCurrency, jvRippleState[jss::currency].asString()))
    {
        jvResult[jss::error] = "malformedCurrency";
        return std::nullopt;
    }

    return keylet::line(*id1, *id2, uCurrency).key;
}

std::optional<uint256>
parseTicket(Json::Value const& params, Json::Value& jvResult)
{
    if (!params.isObject())
    {
        return parseIndex(params, jvResult);
    }

    if (!hasRequired(params, {jss::account, jss::ticket_seq}))
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    auto const id = parseAccountID(params[jss::account]);
    if (!id)
    {
        jvResult[jss::error] = "malformedAccount";
        return std::nullopt;
    }

    auto const seq = parseUInt32(params[jss::ticket_seq]);
    if (!seq)
    {
        jvResult[jss::error] = "malformedTicketSeq";
        return std::nullopt;
    }

    return getTicketIndex(*id, *seq);
}

std::optional<uint256>
parseNFTokenPage(Json::Value const& params, Json::Value& jvResult)
{
    return parseIndex(params, jvResult);
}

std::optional<uint256>
parseNFTokenOffer(Json::Value const& params, Json::Value& jvResult)
{
    return parseIndex(params, jvResult);
}

std::optional<uint256>
parseAMM(Json::Value const& params, Json::Value& jvResult)
{
    if (!params.isObject())
    {
        return parseIndex(params, jvResult);
    }

    if (!hasRequired(params, {jss::asset, jss::asset2}))
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    try
    {
        auto const issue = issueFromJson(params[jss::asset]);
        auto const issue2 = issueFromJson(params[jss::asset2]);
        return keylet::amm(issue, issue2).key;
    }
    catch (std::runtime_error const&)
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }
}

std::optional<uint256>
parseBridge(Json::Value const& params, Json::Value& jvResult)
{
    // return the keylet for the specified bridge or nullopt if the
    // request is malformed
    auto const maybeKeylet = [&]() -> std::optional<Keylet> {
        try
        {
            if (!params.isMember(jss::bridge_account))
                return std::nullopt;

            auto const& jsBridgeAccount = params[jss::bridge_account];
            auto const account = parseAccountID(jsBridgeAccount);
            if (!account)
            {
                return std::nullopt;
            }

            // This may throw and is the reason for the `try` block. The
            // try block has a larger scope so the `bridge` variable
            // doesn't need to be an optional.
            STXChainBridge const bridge(params[jss::bridge]);
            STXChainBridge::ChainType const chainType =
                STXChainBridge::srcChain(account == bridge.lockingChainDoor());

            if (account != bridge.door(chainType))
                return std::nullopt;

            return keylet::bridge(bridge, chainType);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }();

    if (maybeKeylet)
    {
        return maybeKeylet->key;
    }

    jvResult[jss::error] = "malformedRequest";
    return std::nullopt;
}

std::optional<uint256>
parseXChainOwnedClaimID(Json::Value const& claim_id, Json::Value& jvResult)
{
    if (!claim_id.isObject())
    {
        return parseIndex(claim_id, jvResult);
    }

    if (!hasRequired(
            claim_id,
            {jss::IssuingChainDoor,
             jss::LockingChainDoor,
             jss::IssuingChainIssue,
             jss::LockingChainIssue,
             jss::xchain_owned_claim_id}))
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    // if not specified with a node id, a claim_id is specified by
    // four strings defining the bridge (locking_chain_door,
    // locking_chain_issue, issuing_chain_door, issuing_chain_issue)
    // and the claim id sequence number.
    auto const lockingChainDoor =
        parseAccountID(claim_id[jss::LockingChainDoor]);
    auto const issuingChainDoor =
        parseAccountID(claim_id[jss::IssuingChainDoor]);

    if (!(lockingChainDoor && issuingChainDoor))
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    Issue lockingChainIssue, issuingChainIssue;

    try
    {
        lockingChainIssue = issueFromJson(claim_id[jss::LockingChainIssue]);
        issuingChainIssue = issueFromJson(claim_id[jss::IssuingChainIssue]);
    }
    catch (std::runtime_error const& ex)
    {
        jvResult[jss::error] = "malformedIssue";
        return std::nullopt;
    }

    auto const seq = parseUInt32(claim_id[jss::xchain_owned_claim_id]);
    if (!seq)
    {
        jvResult[jss::error] = "malformedXChainOwnedClaimID";
        return std::nullopt;
    }

    STXChainBridge bridge_spec(
        *lockingChainDoor,
        lockingChainIssue,
        *issuingChainDoor,
        issuingChainIssue);
    Keylet keylet = keylet::xChainClaimID(bridge_spec, *seq);
    return keylet.key;
}

std::optional<uint256>
parseXChainOwnedCreateAccountClaimID(
    Json::Value const& claim_id,
    Json::Value& jvResult)
{
    if (!claim_id.isObject())
    {
        return parseIndex(claim_id, jvResult);
    }

    if (!hasRequired(
            claim_id,
            {jss::IssuingChainDoor,
             jss::LockingChainDoor,
             jss::IssuingChainIssue,
             jss::LockingChainIssue,
             jss::xchain_owned_create_account_claim_id}))
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    // if not specified with a node id, a create account claim_id is
    // specified by four strings defining the bridge
    // (locking_chain_door, locking_chain_issue, issuing_chain_door,
    // issuing_chain_issue) and the create account claim id sequence
    // number.
    auto const lockingChainDoor =
        parseAccountID(claim_id[jss::LockingChainDoor]);
    auto const issuingChainDoor =
        parseAccountID(claim_id[jss::IssuingChainDoor]);

    if (!(lockingChainDoor && issuingChainDoor))
    {
        return std::nullopt;
    }

    Issue lockingChainIssue, issuingChainIssue;

    try
    {
        lockingChainIssue = issueFromJson(claim_id[jss::LockingChainIssue]);
        issuingChainIssue = issueFromJson(claim_id[jss::IssuingChainIssue]);
    }
    catch (std::runtime_error const& ex)
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    auto const seq =
        parseUInt32(claim_id[jss::xchain_owned_create_account_claim_id]);
    if (!seq)
    {
        jvResult[jss::error] = "malformedXChainOwnedCreateAccountClaimID";
        return std::nullopt;
    }

    STXChainBridge bridge_spec(
        *lockingChainDoor,
        lockingChainIssue,
        *issuingChainDoor,
        issuingChainIssue);
    Keylet keylet = keylet::xChainCreateAccountClaimID(bridge_spec, *seq);
    return keylet.key;
}

std::optional<uint256>
parseDID(Json::Value const& params, Json::Value& jvResult)
{
    auto const account = parseAccountID(params);
    if (!account)
    {
        jvResult[jss::error] = "malformedAddress";
        return std::nullopt;
    }

    return keylet::did(*account).key;
}

std::optional<uint256>
parseOracle(Json::Value const& params, Json::Value& jvResult)
{
    if (!params.isObject())
    {
        return parseIndex(params, jvResult);
    }

    if (!hasRequired(params, {jss::oracle_document_id, jss::account}))
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    auto const& oracle = params;
    auto const documentID = parseUInt32(oracle[jss::oracle_document_id]);

    auto const account = parseAccountID(oracle[jss::account]);
    if (!account)
    {
        jvResult[jss::error] = "malformedAddress";
        return std::nullopt;
    }

    if (!documentID)
    {
        jvResult[jss::error] = "malformedDocumentID";
        return std::nullopt;
    }

    return keylet::oracle(*account, *documentID).key;
}

std::optional<uint256>
parseCredential(Json::Value const& cred, Json::Value& jvResult)
{
    if (!cred.isObject())
    {
        return parseIndex(cred, jvResult);
    }

    if (!hasRequired(cred, {jss::subject, jss::issuer, jss::credential_type}))
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    auto const subject = parseAccountID(cred[jss::subject]);
    auto const issuer = parseAccountID(cred[jss::issuer]);
    auto const credType =
        parseHexBlob(cred[jss::credential_type], maxCredentialTypeLength);

    if (!subject || !issuer || !credType)
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    return keylet::credential(
               *subject, *issuer, Slice(credType->data(), credType->size()))
        .key;
}

std::optional<uint256>
parseMPTokenIssuance(
    Json::Value const& unparsedMPTIssuanceID,
    Json::Value& jvResult)
{
    uint192 mptIssuanceID;
    if (!unparsedMPTIssuanceID.isString() ||
        !mptIssuanceID.parseHex(unparsedMPTIssuanceID.asString()))
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    return keylet::mptIssuance(mptIssuanceID).key;
}

std::optional<uint256>
parseMPToken(Json::Value const& mptJson, Json::Value& jvResult)
{
    if (!mptJson.isObject())
    {
        return parseIndex(mptJson, jvResult);
    }

    if (!hasRequired(mptJson, {jss::mpt_issuance_id, jss::account}))
    {
        jvResult[jss::error] = "malformedRequest";
        return std::nullopt;
    }

    uint192 mptIssuanceID;
    if (!mptJson[jss::mpt_issuance_id].isString() ||
        !mptIssuanceID.parseHex(mptJson[jss::mpt_issuance_id].asString()))
    {
        jvResult[jss::error] = "malformedMPTIssuanceID";
        return std::nullopt;
    }

    auto const account = parseAccountID(mptJson[jss::account]);
    if (!account)
    {
        jvResult[jss::error] = "malformedAddress";
        return std::nullopt;
    }

    return keylet::mptoken(mptIssuanceID, *account).key;
}

using FunctionType =
    std::optional<uint256> (*)(Json::Value const&, Json::Value&);

struct LedgerEntry
{
    Json::StaticString fieldName;
    FunctionType parseFunction;
    LedgerEntryType expectedType;
};

// {
//   ledger_hash : <ledger>
//   ledger_index : <ledger_index>
//   ...
// }
Json::Value
doLedgerEntry(RPC::JsonContext& context)
{
    static auto ledgerEntryParsers = std::to_array<LedgerEntry>({
#pragma push_macro("LEDGER_ENTRY")
#undef LEDGER_ENTRY

#define LEDGER_ENTRY(tag, value, name, rpcName, fields) \
    {jss::rpcName, parse##name, tag},

#include <xrpl/protocol/detail/ledger_entries.macro>

#undef LEDGER_ENTRY
#pragma pop_macro("LEDGER_ENTRY")
        {jss::index, parseIndex, ltANY},
        {jss::account_root, parseAccountRoot, ltACCOUNT_ROOT},
        {jss::ripple_state, parseRippleState, ltRIPPLE_STATE},
    });

    auto hasMoreThanOneMember = [&]() {
        int count = 0;

        for (const auto& ledgerEntry : ledgerEntryParsers)
        {
            if (context.params.isMember(ledgerEntry.fieldName))
            {
                count++;
                if (count > 1)  // Early exit if more than one is found
                    return true;
            }
        }
        return false;  // Return false if <= 1 is found
    }();

    if (hasMoreThanOneMember)
    {
        Json::Value jvResult = Json::objectValue;
        jvResult[jss::error] = "invalidParams";
        jvResult[jss::error_message] = "Too many fields provided.";
        return jvResult;
    }

    std::shared_ptr<ReadView const> lpLedger;
    auto jvResult = RPC::lookupLedger(lpLedger, context);

    if (!lpLedger)
        return jvResult;

    uint256 uNodeIndex;
    LedgerEntryType expectedType = ltANY;

    try
    {
        bool found = false;
        for (const auto& ledgerEntry : ledgerEntryParsers)
        {
            if (context.params.isMember(ledgerEntry.fieldName))
            {
                expectedType = ledgerEntry.expectedType;
                // `Bridge` is the only type that involves two fields at the
                // `ledger_entry` param level.
                // So that parser needs to have the whole `params` field.
                // All other parsers only need the one field name's info.
                Json::Value const& params = ledgerEntry.fieldName == jss::bridge
                    ? context.params
                    : context.params[ledgerEntry.fieldName];
                uNodeIndex = ledgerEntry.parseFunction(params, jvResult)
                                 .value_or(beast::zero);
                if (jvResult.isMember(jss::error))
                {
                    return jvResult;
                }
                found = true;
                break;
            }
        }
        if (!found)
        {
            if (context.apiVersion < 2u)
                jvResult[jss::error] = "unknownOption";
            else
                jvResult[jss::error] = "invalidParams";
            return jvResult;
        }
    }
    catch (Json::error& e)
    {
        if (context.apiVersion > 1u)
        {
            // For apiVersion 2 onwards, any parsing failures that throw this
            // exception return an invalidParam error.
            jvResult[jss::error] = "invalidParams";
            return jvResult;
        }
        else
            throw;
    }

    if (uNodeIndex.isZero())
    {
        jvResult[jss::error] = "entryNotFound";
        return jvResult;
    }

    auto const sleNode = lpLedger->read(keylet::unchecked(uNodeIndex));

    bool bNodeBinary = false;
    if (context.params.isMember(jss::binary))
        bNodeBinary = context.params[jss::binary].asBool();

    if (!sleNode)
    {
        // Not found.
        jvResult[jss::error] = "entryNotFound";
        return jvResult;
    }

    if ((expectedType != ltANY) && (expectedType != sleNode->getType()))
    {
        jvResult[jss::error] = "unexpectedLedgerType";
        return jvResult;
    }

    if (bNodeBinary)
    {
        Serializer s;

        sleNode->add(s);

        jvResult[jss::node_binary] = strHex(s.peekData());
        jvResult[jss::index] = to_string(uNodeIndex);
    }
    else
    {
        jvResult[jss::node] = sleNode->getJson(JsonOptions::none);
        jvResult[jss::index] = to_string(uNodeIndex);
    }

    return jvResult;
}

std::pair<org::xrpl::rpc::v1::GetLedgerEntryResponse, grpc::Status>
doLedgerEntryGrpc(
    RPC::GRPCContext<org::xrpl::rpc::v1::GetLedgerEntryRequest>& context)
{
    org::xrpl::rpc::v1::GetLedgerEntryRequest& request = context.params;
    org::xrpl::rpc::v1::GetLedgerEntryResponse response;
    grpc::Status status = grpc::Status::OK;

    std::shared_ptr<ReadView const> ledger;
    if (auto status = RPC::ledgerFromRequest(ledger, context))
    {
        grpc::Status errorStatus;
        if (status.toErrorCode() == rpcINVALID_PARAMS)
        {
            errorStatus = grpc::Status(
                grpc::StatusCode::INVALID_ARGUMENT, status.message());
        }
        else
        {
            errorStatus =
                grpc::Status(grpc::StatusCode::NOT_FOUND, status.message());
        }
        return {response, errorStatus};
    }

    auto const key = uint256::fromVoidChecked(request.key());
    if (!key)
    {
        grpc::Status errorStatus{
            grpc::StatusCode::INVALID_ARGUMENT, "index malformed"};
        return {response, errorStatus};
    }

    auto const sleNode = ledger->read(keylet::unchecked(*key));
    if (!sleNode)
    {
        grpc::Status errorStatus{
            grpc::StatusCode::NOT_FOUND, "object not found"};
        return {response, errorStatus};
    }
    else
    {
        Serializer s;
        sleNode->add(s);

        auto& stateObject = *response.mutable_ledger_object();
        stateObject.set_data(s.peekData().data(), s.getLength());
        stateObject.set_key(request.key());
        *(response.mutable_ledger()) = request.ledger();
        return {response, status};
    }
}
}  // namespace ripple
