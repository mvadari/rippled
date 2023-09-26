//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2017 Ripple Labs Inc.

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

#include <ripple/protocol/Feature.h>
#include <ripple/protocol/digest.h>
#include <ripple/protocol/jss.h>
#include <test/jtx.h>
#include <test/jtx/PluginEnv.h>
#include <test/jtx/TestHelpers.h>

namespace ripple {

namespace test {

inline FeatureBitset
supported_amendments_plugins()
{
    static const FeatureBitset ids = [] {
        auto const& sa = ripple::detail::supportedAmendments();
        std::vector<uint256> feats;
        feats.reserve(sa.size());
        // for (auto const& [s, vote] : sa)
        // {
        //     (void)vote;
        //     if (auto const f = getRegisteredFeature(s))
        //         feats.push_back(*f);
        //     else
        //         Throw<std::runtime_error>(
        //             "Unknown feature: " + s + "  in supportedAmendments.");
        // }
        return FeatureBitset(feats);
    }();
    return ids;
}

template <class... Args>
static uint256
indexHash(std::uint16_t space, Args const&... args)
{
    return sha512Half(space, args...);
}

// Helper function that returns the owner count of an account root.
std::uint32_t
ownerCount(test::jtx::Env const& env, test::jtx::Account const& acct)
{
    std::uint32_t ret{0};
    if (auto const sleAcct = env.le(acct))
        ret = sleAcct->at(sfOwnerCount);
    return ret;
}

class Plugins_test : public beast::unit_test::suite
{
    std::unique_ptr<Config>
    makeConfig(std::string pluginPath)
    {
        auto cfg = test::jtx::envconfig();
        cfg->PLUGINS.push_back(pluginPath);
        return cfg;
    }

    void
    testTransactorLoading()
    {
        testcase("Load Plugin Transactors");

        using namespace jtx;
        Account const alice{"alice"};

        // plugin that doesn't exist
        {
            try
            {
                reinitialize();
                // this should crash
                PluginEnv env{
                    *this,
                    makeConfig("plugin_test_faketest.xrplugin"),
                    FeatureBitset{supported_amendments_plugins()}};
                BEAST_EXPECT(false);
            }
            catch (std::runtime_error)
            {
                BEAST_EXPECT(true);
            }
        }

        // valid plugin that exists
        {
            reinitialize();
            PluginEnv env{
                *this,
                makeConfig("plugin_test_setregularkey.xrplugin"),
                FeatureBitset{supported_amendments_plugins()}};
            env.fund(XRP(5000), alice);
            BEAST_EXPECT(env.balance(alice) == XRP(5000));
            env.close();
        }

        // valid plugin with custom SType/SField
        {
            reinitialize();
            PluginEnv env{
                *this,
                makeConfig("plugin_test_trustset.xrplugin"),
                FeatureBitset{supported_amendments_plugins()}};
            env.fund(XRP(5000), alice);
            BEAST_EXPECT(env.balance(alice) == XRP(5000));
            env.close();
        }

        // valid plugin with other features
        {
            reinitialize();
            PluginEnv env{
                *this,
                makeConfig("plugin_test_escrowcreate.xrplugin"),
                FeatureBitset{supported_amendments_plugins()}};
            env.fund(XRP(5000), alice);
            BEAST_EXPECT(env.balance(alice) == XRP(5000));
            env.close();
        }
    }

    void
    testBasicTransactor()
    {
        testcase("Normal Plugin Transactor");

        using namespace jtx;
        Account const alice{"alice"};
        Account const bob{"bob"};

        reinitialize();
        PluginEnv env{
            *this,
            makeConfig("plugin_test_setregularkey.xrplugin"),
            FeatureBitset{supported_amendments_plugins()}};
        env.fund(XRP(5000), alice);
        BEAST_EXPECT(env.balance(alice) == XRP(5000));

        // empty (but valid) transaction
        Json::Value jv;
        jv[jss::TransactionType] = "SetRegularKey2";
        jv[jss::Account] = alice.human();
        env(jv);

        // a transaction that actually sets the regular key of the account
        Json::Value jv2;
        jv2[jss::TransactionType] = "SetRegularKey2";
        jv2[jss::Account] = alice.human();
        jv2[sfRegularKey.jsonName] = to_string(bob.id());
        env(jv2);
        auto const accountRoot = env.le(alice);
        BEAST_EXPECT(
            accountRoot->isFieldPresent(sfRegularKey) &&
            (accountRoot->getAccountID(sfRegularKey) == bob.id()));

        env.close();
    }

    void
    testPluginSTypeSField()
    {
        testcase("Plugin STypes and SFields");

        using namespace jtx;
        Account const alice{"alice"};
        Account const bob{"bob"};

        std::string const amendmentName = "featurePluginTest";
        auto const trustSet2Amendment =
            sha512Half(Slice(amendmentName.data(), amendmentName.size()));

        reinitialize();
        PluginEnv env{
            *this,
            makeConfig("plugin_test_trustset.xrplugin"),
            FeatureBitset{supported_amendments_plugins()},
            nullptr,
            beast::severities::kError,
            trustSet2Amendment};

        env.fund(XRP(5000), alice);
        env.fund(XRP(5000), bob);
        IOU const USD = bob["USD"];
        // sanity checks
        BEAST_EXPECT(env.balance(alice) == XRP(5000));
        BEAST_EXPECT(env.balance(bob) == XRP(5000));

        // valid transaction without any custom fields
        {
            Json::Value jv;
            jv[jss::TransactionType] = "TrustSet2";
            jv[jss::Account] = alice.human();
            {
                auto& ja = jv[jss::LimitAmount] =
                    USD(1000).value().getJson(JsonOptions::none);
                ja[jss::issuer] = bob.human();
            }
            env(jv);
            env.close();
            auto const trustline = env.le(keylet::line(alice, USD.issue()));
            BEAST_EXPECT(trustline != nullptr);
        }

        // valid transaction that uses QualityIn2
        {
            Json::Value jv;
            jv[jss::TransactionType] = "TrustSet2";
            jv[jss::Account] = alice.human();
            {
                auto& ja = jv[jss::LimitAmount] =
                    USD(1000).value().getJson(JsonOptions::none);
                ja[jss::issuer] = bob.human();
            }
            jv["QualityIn2"] = "101";
            env(jv);
            env.close();
            auto const trustline = env.le(keylet::line(alice, USD.issue()));
            BEAST_EXPECT(trustline != nullptr);
        }

        // valid transaction that uses FakeElement
        {
            Json::Value jv;
            jv[jss::TransactionType] = "TrustSet2";
            jv[jss::Account] = alice.human();
            {
                auto& ja = jv[jss::LimitAmount] =
                    USD(1000).value().getJson(JsonOptions::none);
                ja[jss::issuer] = bob.human();
            }
            jv["QualityIn2"] = "101";
            {
                Json::Value array(Json::arrayValue);
                Json::Value obj;
                Json::Value innerObj;
                innerObj[jss::Account] = bob.human();
                obj["FakeElement"] = innerObj;
                array.append(obj);
                jv["FakeArray"] = array;
            }
            env(jv);
            env.close();
            auto const trustline = env.le(keylet::line(alice, USD.issue()));
            BEAST_EXPECT(trustline != nullptr);
        }

        env.close();
    }

    void
    testPluginLedgerObjectInvariantCheck()
    {
        testcase("Plugin Ledger Objects and Invariant Checks");

        using namespace jtx;
        Account const alice{"alice"};
        Account const bob{"bob"};

        std::string const amendmentName = "featurePluginTest2";
        auto const newEscrowCreateAmendment =
            sha512Half(Slice(amendmentName.data(), amendmentName.size()));

        reinitialize();
        PluginEnv env{
            *this,
            makeConfig("plugin_test_escrowcreate.xrplugin"),
            FeatureBitset{supported_amendments_plugins()},
            nullptr,
            beast::severities::kError,
            newEscrowCreateAmendment};

        env.fund(XRP(5000), alice);
        env.fund(XRP(5000), bob);
        // sanity checks
        BEAST_EXPECT(env.balance(alice) == XRP(5000));
        BEAST_EXPECT(env.balance(bob) == XRP(5000));

        static const std::uint16_t ltNEW_ESCROW = 0x0074;
        static const std::uint16_t NEW_ESCROW_NAMESPACE = 't';
        auto new_escrow_keylet = [](const AccountID& src,
                                    std::uint32_t seq) noexcept -> Keylet {
            return {ltNEW_ESCROW, indexHash(NEW_ESCROW_NAMESPACE, src, seq)};
        };

        // valid transaction
        {
            auto const seq = env.seq(alice);
            Json::Value jv;
            jv[jss::TransactionType] = "NewEscrowCreate";
            jv[jss::Account] = alice.human();
            jv[jss::Amount] = "10000";
            jv[jss::Destination] = alice.human();
            jv[sfFinishAfter.jsonName] =
                env.now().time_since_epoch().count() + 10;

            env(jv);
            auto const escrow = env.le(new_escrow_keylet(alice, seq));
            BEAST_EXPECT(escrow != nullptr);
        }

        // invalid transaction that triggers the invariant check
        {
            BEAST_EXPECT(ownerCount(env, bob) == 0);
            Json::Value jv;
            jv[jss::TransactionType] = "NewEscrowCreate";
            jv[jss::Account] = bob.human();
            jv[jss::Amount] = "0";
            jv[jss::Destination] = bob.human();
            jv[sfFinishAfter.jsonName] =
                env.now().time_since_epoch().count() + 10;

            env(jv, ter(tecINVARIANT_FAILED));
            BEAST_EXPECT(ownerCount(env, bob) == 0);
        }

        env.close();
    }

    void
    run() override
    {
        using namespace test::jtx;
        testTransactorLoading();
        testBasicTransactor();
        testPluginSTypeSField();
        testPluginLedgerObjectInvariantCheck();
    }
};

BEAST_DEFINE_TESTSUITE(Plugins, plugins, ripple);

}  // namespace test
}  // namespace ripple
