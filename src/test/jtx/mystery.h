#pragma once

#include <test/jtx/Account.h>

#include <xrpl/basics/Slice.h>
#include <xrpl/basics/strHex.h>
#include <xrpl/json/json_value.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/STAmount.h>
#include <xrpl/protocol/TxFlags.h>
#include <xrpl/protocol/digest.h>
#include <xrpl/protocol/jss.h>

#include <cstdint>
#include <string>

/**
 * Mystery operations.
 */
namespace xrpl::test::jtx::mystery {

/**
 * The Digest that a Mystery stores for a phrase.
 */
inline UInt256
digest(std::string const& phrase)
{
    return sha512Half(makeSlice(phrase));
}

inline json::Value
create(Account const& account, STAmount const& amount, std::string const& phrase)
{
    json::Value jv;
    jv[jss::TransactionType] = jss::MysteryCreate;
    jv[jss::Flags] = tfFullyCanonicalSig;
    jv[jss::Account] = account.human();
    jv[jss::Amount] = amount.getJson(JsonOptions::Values::None);
    jv[sfDigest.jsonName] = to_string(digest(phrase));
    return jv;
}

inline json::Value
guess(Account const& account, Account const& owner, std::uint32_t seq, std::string const& phrase)
{
    json::Value jv;
    jv[jss::TransactionType] = jss::MysteryGuess;
    jv[jss::Flags] = tfFullyCanonicalSig;
    jv[jss::Account] = account.human();
    jv[sfOwner.jsonName] = owner.human();
    jv[sfOfferSequence.jsonName] = seq;
    jv[sfGuess.jsonName] = strHex(phrase);
    return jv;
}

inline json::Value
cancel(Account const& account, std::uint32_t seq)
{
    json::Value jv;
    jv[jss::TransactionType] = jss::MysteryCancel;
    jv[jss::Flags] = tfFullyCanonicalSig;
    jv[jss::Account] = account.human();
    jv[sfOfferSequence.jsonName] = seq;
    return jv;
}

}  // namespace xrpl::test::jtx::mystery
