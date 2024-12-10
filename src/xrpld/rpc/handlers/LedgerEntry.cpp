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

Unexpected<Json::Value>
missingFieldError(Json::StaticString field)
{
    Json::Value json = Json::objectValue;
    auto const& error = RPC::missing_field_message(std::string(field.c_str()));
    json[jss::error] = "malformedRequest";
    json[jss::error_code] = rpcINVALID_PARAMS;
    json[jss::error_message] = error;
    return Unexpected(json);
}

Unexpected<Json::Value>
invalidFieldError(
    std::string err,
    Json::StaticString field,
    std::string const& type)
{
    Json::Value json = Json::objectValue;
    auto const& error =
        RPC::expected_field_message(std::string(field.c_str()), type);
    json[jss::error] = err;
    json[jss::error_code] = rpcINVALID_PARAMS;
    json[jss::error_message] = error;
    return Unexpected(json);
}

Unexpected<Json::Value>
malformedError(std::string err, std::string message)
{
    Json::Value json = Json::objectValue;
    json[jss::error] = err;
    json[jss::error_code] = rpcINVALID_PARAMS;
    json[jss::error_message] = message;
    return Unexpected(json);
}

template <class T>
std::optional<T>
parse(Json::Value const& param);

template <>
std::optional<AccountID>
parse(Json::Value const& param)
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

template <class T>
Expected<T, Json::Value>
required(
    Json::Value const& params,
    Json::StaticString const& fieldName,
    std::string err,
    std::string expectedType)
{
    if (!params.isMember(fieldName))
    {
        return missingFieldError(fieldName);
    }
    if (auto obj = parse<T>(params[fieldName]))
    {
        return *obj;
    }
    return invalidFieldError(err, fieldName, expectedType);
}

Expected<AccountID, Json::Value>
requiredAccountID(
    Json::Value const& params,
    Json::StaticString const& fieldName,
    std::string err)
{
    return required<AccountID>(params, fieldName, err, "AccountID");
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

Expected<Blob, Json::Value>
requiredHexBlob(
    Json::Value const& params,
    Json::StaticString const& fieldName,
    std::size_t maxLength,
    std::string err)
{
    if (!params.isMember(fieldName))
    {
        return missingFieldError(fieldName);
    }
    if (auto blob = parseHexBlob(params[fieldName], maxLength))
    {
        return *blob;
    }
    return invalidFieldError(err, fieldName, "hex string");
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

template <>
std::optional<std::uint32_t>
parse(Json::Value const& param)
{
    if (param.isUInt() || (param.isInt() && param.asInt() >= 0))
        return param.asUInt();

    if (param.isString())
    {
        std::uint32_t v;
        if (beast::lexicalCastChecked(v, param.asString()))
            return v;
    }

    return std::nullopt;
}

Expected<std::uint32_t, Json::Value>
requiredUInt32(
    Json::Value const& params,
    Json::StaticString const& fieldName,
    std::string err)
{
    return required<std::uint32_t>(params, fieldName, err, "number");
}

template <>
std::optional<uint256>
parse(Json::Value const& param)
{
    uint256 uNodeIndex;
    if (!param.isString() || !uNodeIndex.parseHex(param.asString()))
    {
        return std::nullopt;
    }

    return uNodeIndex;
}

Expected<uint256, Json::Value>
requiredUInt256(
    Json::Value const& params,
    Json::StaticString const& fieldName,
    std::string err)
{
    return required<uint256>(params, fieldName, err, "hex string");
}

Expected<uint256, Json::Value>
parseIndex(Json::Value const& params, Json::StaticString const& fieldName)
{
    if (auto const uNodeIndex = parse<uint256>(params))
    {
        return *uNodeIndex;
    }

    return invalidFieldError("malformedRequest", fieldName, "hex string");
}

Expected<uint256, Json::Value>
parseAccountRoot(Json::Value const& params, Json::StaticString const& fieldName)
{
    if (auto const account = parse<AccountID>(params))
    {
        return keylet::account(*account).key;
    }

    return invalidFieldError("malformedAddress", fieldName, "AccountID");
}

Expected<uint256, Json::Value>
parseCheck(Json::Value const& params, Json::StaticString const& fieldName)
{
    return parseIndex(params, fieldName);
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

        auto const issuer = parse<AccountID>(jo[jss::issuer]);
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

Expected<uint256, Json::Value>
parseDepositPreauth(Json::Value const& dp, Json::StaticString const& fieldName)
{
    if (!dp.isObject())
    {
        return parseIndex(dp, fieldName);
    }

    if ((dp.isMember(jss::authorized) ==
         dp.isMember(jss::authorized_credentials)))
    {
        return malformedError(
            "malformedRequest",
            "Must have exactly one of `authorized` and "
            "`authorized_credentials`.");
    }

    auto const owner = requiredAccountID(dp, jss::owner, "malformedOwner");
    if (!owner)
    {
        return Unexpected(owner.error());
    }

    if (dp.isMember(jss::authorized))
    {
        if (auto const authorized = parse<AccountID>(dp[jss::authorized]))
        {
            return keylet::depositPreauth(*owner, *authorized).key;
        }
        return invalidFieldError(
            "malformedAuthorized", jss::authorized, "Account");
    }

    auto const& ac(dp[jss::authorized_credentials]);
    STArray const arr = parseAuthorizeCredentials(ac);
    if (arr.empty() || (arr.size() > maxCredentialsArraySize))
    {
        return invalidFieldError(
            "malformedAuthorizedCredentials",
            jss::authorized_credentials,
            "Array");
    }

    auto const& sorted = credentials::makeSorted(arr);
    if (sorted.empty())
    {
        return invalidFieldError(
            "malformedAuthorizedCredentials",
            jss::authorized_credentials,
            "Array");
    }

    return keylet::depositPreauth(*owner, sorted).key;
}

Expected<uint256, Json::Value>
parseDirectoryNode(
    Json::Value const& params,
    Json::StaticString const& fieldName)
{
    if (!params.isObject())
    {
        return parseIndex(params, fieldName);
    }

    if (params.isMember(jss::sub_index) && !params[jss::sub_index].isIntegral())
    {
        return invalidFieldError("malformedSubIndex", jss::sub_index, "number");
    }

    if (params.isMember(jss::owner) == params.isMember(jss::dir_root))
    {
        return malformedError(
            "malformedRequest",
            "Cannot have both `owner` and `dir_root` fields.");
    }

    std::uint64_t uSubIndex = params.get(jss::sub_index, 0).asUInt();

    if (params.isMember(jss::dir_root))
    {
        if (auto const uDirRoot = parse<uint256>(params[jss::dir_root]))
        {
            return keylet::page(*uDirRoot, uSubIndex).key;
        }

        return invalidFieldError("malformedDirRoot", jss::dir_root, "hash");
    }

    if (params.isMember(jss::owner))
    {
        auto const ownerID = parse<AccountID>(params[jss::owner]);
        if (!ownerID)
        {
            return invalidFieldError("malformedOwner", jss::owner, "Account");
        }

        return keylet::page(keylet::ownerDir(*ownerID), uSubIndex).key;
    }

    return malformedError("malformedRequest", "");
}

Expected<uint256, Json::Value>
parseEscrow(Json::Value const& params, Json::StaticString const& fieldName)
{
    if (!params.isObject())
    {
        return parseIndex(params, fieldName);
    }

    auto const id = requiredAccountID(params, jss::owner, "malformedOwner");
    if (!id)
        return Unexpected(id.error());
    auto const seq = requiredUInt32(params, jss::seq, "malformedSeq");
    if (!seq)
        return Unexpected(seq.error());

    return keylet::escrow(*id, *seq).key;
}

Expected<uint256, Json::Value>
parseAmendments(Json::Value const& params, Json::StaticString const& fieldName)
{
    return parseIndex(params, fieldName);
}

Expected<uint256, Json::Value>
parseFeeSettings(Json::Value const& params, Json::StaticString const& fieldName)
{
    return parseIndex(params, fieldName);
}

Expected<uint256, Json::Value>
parseSignerList(Json::Value const& params, Json::StaticString const& fieldName)
{
    return parseIndex(params, fieldName);
}

Expected<uint256, Json::Value>
parseNegativeUNL(Json::Value const& params, Json::StaticString const& fieldName)
{
    return parseIndex(params, fieldName);
}

Expected<uint256, Json::Value>
parseLedgerHashes(
    Json::Value const& params,
    Json::StaticString const& fieldName)
{
    return parseIndex(params, fieldName);
}

Expected<uint256, Json::Value>
parseOffer(Json::Value const& params, Json::StaticString const& fieldName)
{
    if (!params.isObject())
    {
        return parseIndex(params, fieldName);
    }

    auto const id = requiredAccountID(params, jss::account, "malformedAccount");
    if (!id)
        return Unexpected(id.error());
    auto const seq = requiredUInt32(params, jss::seq, "malformedSeq");
    if (!seq)
        return Unexpected(seq.error());

    return keylet::offer(*id, *seq).key;
}

Expected<uint256, Json::Value>
parsePayChannel(Json::Value const& params, Json::StaticString const& fieldName)
{
    return parseIndex(params, fieldName);
}

Expected<uint256, Json::Value>
parseRippleState(
    Json::Value const& jvRippleState,
    Json::StaticString const& fieldName)
{
    Currency uCurrency;

    if (!jvRippleState.isObject())
    {
        return parseIndex(jvRippleState, fieldName);
    }

    if (!hasRequired(jvRippleState, {jss::currency, jss::accounts}))
    {
        return malformedError("malformedRequest", "");
    }

    if (!jvRippleState[jss::accounts].isArray() ||
        jvRippleState[jss::accounts].size() != 2)
    {
        return invalidFieldError(
            "malformedAccounts", jss::accounts, "array of Accounts");
    }

    auto const id1 = parse<AccountID>(jvRippleState[jss::accounts][0u]);
    auto const id2 = parse<AccountID>(jvRippleState[jss::accounts][1u]);
    if (!id1 || !id2)
    {
        return invalidFieldError(
            "malformedAccounts", jss::accounts, "array of Accounts");
    }
    if (id1 == id2)
    {
        return malformedError(
            "malformedAccounts", "Cannot have a trustline to self.");
    }

    if (!jvRippleState[jss::currency].isString() ||
        !to_currency(uCurrency, jvRippleState[jss::currency].asString()))
    {
        return invalidFieldError(
            "malformedCurrency", jss::currency, "Currency");
    }

    return keylet::line(*id1, *id2, uCurrency).key;
}

Expected<uint256, Json::Value>
parseTicket(Json::Value const& params, Json::StaticString const& fieldName)
{
    if (!params.isObject())
    {
        return parseIndex(params, fieldName);
    }

    auto const id = requiredAccountID(params, jss::account, "malformedAccount");
    if (!id)
        return Unexpected(id.error());
    auto const seq =
        requiredUInt32(params, jss::ticket_seq, "malformedTicketSeq");
    if (!seq)
        return Unexpected(seq.error());

    return getTicketIndex(*id, *seq);
}

Expected<uint256, Json::Value>
parseNFTokenPage(Json::Value const& params, Json::StaticString const& fieldName)
{
    return parseIndex(params, fieldName);
}

Expected<uint256, Json::Value>
parseNFTokenOffer(
    Json::Value const& params,
    Json::StaticString const& fieldName)
{
    return parseIndex(params, fieldName);
}

Expected<uint256, Json::Value>
parseAMM(Json::Value const& params, Json::StaticString const& fieldName)
{
    if (!params.isObject())
    {
        return parseIndex(params, fieldName);
    }

    if (!hasRequired(params, {jss::asset, jss::asset2}))
    {
        return malformedError("malformedRequest", "");
    }

    try
    {
        auto const issue = issueFromJson(params[jss::asset]);
        auto const issue2 = issueFromJson(params[jss::asset2]);
        return keylet::amm(issue, issue2).key;
    }
    catch (std::runtime_error const&)
    {
        return malformedError("malformedRequest", "");
    }
}

Expected<uint256, Json::Value>
parseBridge(Json::Value const& params, Json::StaticString const& fieldName)
{
    // return the keylet for the specified bridge or nullopt if the
    // request is malformed
    auto const maybeKeylet = [&]() -> std::optional<Keylet> {
        try
        {
            if (!params.isMember(jss::bridge_account))
                return std::nullopt;

            auto const& jsBridgeAccount = params[jss::bridge_account];
            auto const account = parse<AccountID>(jsBridgeAccount);
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

    return malformedError("malformedRequest", "");
}

Expected<uint256, Json::Value>
parseXChainOwnedClaimID(
    Json::Value const& claim_id,
    Json::StaticString const& fieldName)
{
    if (!claim_id.isObject())
    {
        return parseIndex(claim_id, fieldName);
    }

    if (!hasRequired(
            claim_id,
            {jss::IssuingChainDoor,
             jss::LockingChainDoor,
             jss::IssuingChainIssue,
             jss::LockingChainIssue,
             jss::xchain_owned_claim_id}))
    {
        return malformedError("malformedRequest", "");
    }

    // if not specified with a node id, a claim_id is specified by
    // four strings defining the bridge (locking_chain_door,
    // locking_chain_issue, issuing_chain_door, issuing_chain_issue)
    // and the claim id sequence number.
    auto const lockingChainDoor =
        parse<AccountID>(claim_id[jss::LockingChainDoor]);
    auto const issuingChainDoor =
        parse<AccountID>(claim_id[jss::IssuingChainDoor]);

    if (!(lockingChainDoor && issuingChainDoor))
    {
        return malformedError("malformedRequest", "");
    }

    Issue lockingChainIssue, issuingChainIssue;

    try
    {
        lockingChainIssue = issueFromJson(claim_id[jss::LockingChainIssue]);
        issuingChainIssue = issueFromJson(claim_id[jss::IssuingChainIssue]);
    }
    catch (std::runtime_error const& ex)
    {
        invalidFieldError("malformedIssue", jss::LockingChainIssue, "Issue");
    }

    auto const seq = requiredUInt32(
        claim_id, jss::xchain_owned_claim_id, "malformedXChainOwnedClaimID");
    if (!seq)
    {
        return Unexpected(seq.error());
    }

    STXChainBridge bridge_spec(
        *lockingChainDoor,
        lockingChainIssue,
        *issuingChainDoor,
        issuingChainIssue);
    Keylet keylet = keylet::xChainClaimID(bridge_spec, *seq);
    return keylet.key;
}

Expected<uint256, Json::Value>
parseXChainOwnedCreateAccountClaimID(
    Json::Value const& claim_id,
    Json::StaticString const& fieldName)
{
    if (!claim_id.isObject())
    {
        return parseIndex(claim_id, fieldName);
    }

    if (!hasRequired(
            claim_id,
            {jss::IssuingChainDoor,
             jss::LockingChainDoor,
             jss::IssuingChainIssue,
             jss::LockingChainIssue,
             jss::xchain_owned_create_account_claim_id}))
    {
        return malformedError("malformedRequest", "");
    }

    // if not specified with a node id, a create account claim_id is
    // specified by four strings defining the bridge
    // (locking_chain_door, locking_chain_issue, issuing_chain_door,
    // issuing_chain_issue) and the create account claim id sequence
    // number.
    auto const lockingChainDoor =
        parse<AccountID>(claim_id[jss::LockingChainDoor]);
    auto const issuingChainDoor =
        parse<AccountID>(claim_id[jss::IssuingChainDoor]);

    if (!(lockingChainDoor && issuingChainDoor))
    {
        return malformedError("malformedRequest", "");
    }

    Issue lockingChainIssue, issuingChainIssue;

    try
    {
        lockingChainIssue = issueFromJson(claim_id[jss::LockingChainIssue]);
        issuingChainIssue = issueFromJson(claim_id[jss::IssuingChainIssue]);
    }
    catch (std::runtime_error const& ex)
    {
        return malformedError("malformedRequest", "");
    }

    auto const seq = requiredUInt32(
        claim_id,
        jss::xchain_owned_create_account_claim_id,
        "malformedXChainOwnedCreateAccountClaimID");
    if (!seq)
    {
        return Unexpected(seq.error());
    }

    STXChainBridge bridge_spec(
        *lockingChainDoor,
        lockingChainIssue,
        *issuingChainDoor,
        issuingChainIssue);
    Keylet keylet = keylet::xChainCreateAccountClaimID(bridge_spec, *seq);
    return keylet.key;
}

Expected<uint256, Json::Value>
parseDID(Json::Value const& params, Json::StaticString const& fieldName)
{
    auto const account = parse<AccountID>(params);
    if (!account)
    {
        return invalidFieldError("malformedAddress", fieldName, "Account");
    }

    return keylet::did(*account).key;
}

Expected<uint256, Json::Value>
parseOracle(Json::Value const& params, Json::StaticString const& fieldName)
{
    if (!params.isObject())
    {
        return parseIndex(params, fieldName);
    }

    if (!hasRequired(params, {jss::oracle_document_id, jss::account}))
    {
        return malformedError("malformedRequest", "");
    }

    auto const id = requiredAccountID(params, jss::account, "malformedAccount");
    if (!id)
        return Unexpected(id.error());
    auto const seq =
        requiredUInt32(params, jss::oracle_document_id, "malformedDocumentID");
    if (!seq)
        return Unexpected(seq.error());

    return keylet::oracle(*id, *seq).key;
}

Expected<uint256, Json::Value>
parseCredential(Json::Value const& cred, Json::StaticString const& fieldName)
{
    if (!cred.isObject())
    {
        return parseIndex(cred, fieldName);
    }

    if (!hasRequired(cred, {jss::subject, jss::issuer, jss::credential_type}))
    {
        return malformedError("malformedRequest", "");
    }

    auto const subject =
        requiredAccountID(cred, jss::subject, "malformedSubject");
    if (!subject)
        return Unexpected(subject.error());

    auto const issuer = requiredAccountID(cred, jss::issuer, "malformedIssuer");
    if (!issuer)
        return Unexpected(issuer.error());

    auto const credType = requiredHexBlob(
        cred,
        jss::credential_type,
        maxCredentialTypeLength,
        "malformedCredentialType");
    if (!credType)
        return Unexpected(credType.error());

    return keylet::credential(
               *subject, *issuer, Slice(credType->data(), credType->size()))
        .key;
}

Expected<uint256, Json::Value>
parseMPTokenIssuance(
    Json::Value const& unparsedMPTIssuanceID,
    Json::StaticString const& fieldName)
{
    uint192 mptIssuanceID;
    if (!unparsedMPTIssuanceID.isString() ||
        !mptIssuanceID.parseHex(unparsedMPTIssuanceID.asString()))
    {
        return malformedError("malformedRequest", "");
    }

    return keylet::mptIssuance(mptIssuanceID).key;
}

Expected<uint256, Json::Value>
parseMPToken(Json::Value const& mptJson, Json::StaticString const& fieldName)
{
    if (!mptJson.isObject())
    {
        return parseIndex(mptJson, fieldName);
    }

    if (!hasRequired(mptJson, {jss::mpt_issuance_id, jss::account}))
    {
        return malformedError("malformedRequest", "");
    }

    uint192 mptIssuanceID;
    if (!mptJson[jss::mpt_issuance_id].isString() ||
        !mptIssuanceID.parseHex(mptJson[jss::mpt_issuance_id].asString()))
    {
        return invalidFieldError(
            "malformedMPTIssuanceID", jss::mpt_issuance_id, "Hash192");
    }

    auto const account = parse<AccountID>(mptJson[jss::account]);
    if (!account)
    {
        return invalidFieldError("malformedAddress", jss::account, "Account");
    }

    return keylet::mptoken(mptIssuanceID, *account).key;
}

using FunctionType = Expected<uint256, Json::Value> (*)(
    Json::Value const&,
    Json::StaticString const&);

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
        return malformedError("invalidParams", "Too many fields provided.")
            .value();
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
                auto const result =
                    ledgerEntry.parseFunction(params, ledgerEntry.fieldName);
                if (result)
                {
                    uNodeIndex = result.value();
                    found = true;
                }
                else
                {
                    return result.error();
                }
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
