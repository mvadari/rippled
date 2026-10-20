#include <test/jtx/Account.h>
#include <test/jtx/Env.h>
#include <test/jtx/TestHelpers.h>
#include <test/jtx/amount.h>
#include <test/jtx/mystery.h>
#include <test/jtx/pay.h>
#include <test/jtx/ter.h>

#include <xrpl/basics/base_uint.h>
#include <xrpl/basics/strHex.h>
#include <xrpl/beast/unit_test/suite.h>
#include <xrpl/json/json_value.h>
#include <xrpl/json/to_string.h>
#include <xrpl/protocol/Feature.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/Protocol.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/SeqProxy.h>
#include <xrpl/protocol/TER.h>
#include <xrpl/protocol/jss.h>

#include <string>

namespace xrpl::test {

struct Mystery_test : public beast::unit_test::Suite
{
    static inline std::string const kPhrase = "the butler did it";

    static Keylet
    mysteryKeylet(jtx::Account const& owner, std::uint32_t seq)
    {
        return keylet::mystery(owner.id(), SeqProxy::rawSequence(seq));
    }

    void
    testEnabled(FeatureBitset features)
    {
        testcase("featureMystery Enabled");

        using namespace jtx;
        Env env{*this, features - featureMystery};
        Account const alice{"alice"};
        Account const bob{"bob"};
        env.fund(XRP(5000), alice, bob);
        env.close();

        env(mystery::create(alice, XRP(100), kPhrase), Ter(temDISABLED));
        env(mystery::guess(bob, alice, 1, kPhrase), Ter(temDISABLED));
        env(mystery::cancel(alice, 1), Ter(temDISABLED));
    }

    void
    testPreflight(FeatureBitset features)
    {
        testcase("Preflight");

        using namespace jtx;
        Env env{*this, features};
        Account const alice{"alice"};
        Account const bob{"bob"};
        Account const gw{"gw"};
        env.fund(XRP(5000), alice, bob, gw);
        env.close();

        // The prize must be a positive XRP amount.
        env(mystery::create(alice, gw["USD"](100), kPhrase), Ter(temBAD_AMOUNT));
        env(mystery::create(alice, XRP(0), kPhrase), Ter(temBAD_AMOUNT));

        // The Digest must not be zero.
        {
            auto jv = mystery::create(alice, XRP(100), kPhrase);
            jv[sfDigest.jsonName] = to_string(UInt256{});
            env(jv, Ter(temMALFORMED));
        }

        // The clue must not be empty or too long.
        {
            auto jv = mystery::create(alice, XRP(100), kPhrase);
            jv[sfData.jsonName] = "";
            env(jv, Ter(temMALFORMED));
            jv[sfData.jsonName] = strHex(std::string(kMaxMysteryDataLength + 1, 'a'));
            env(jv, Ter(temMALFORMED));
        }

        // The guess must not be empty or too long.
        env(mystery::guess(bob, alice, 1, ""), Ter(temMALFORMED));
        env(mystery::guess(bob, alice, 1, std::string(kMaxMysteryGuessLength + 1, 'a')),
            Ter(temMALFORMED));
    }

    void
    testReserve(FeatureBitset features)
    {
        testcase("Reserve");

        using namespace jtx;
        Env env{*this, features};
        Account const alice{"alice"};
        auto const acctReserve = env.current()->fees().reserve;
        auto const incReserve = env.current()->fees().increment;
        auto const baseFee = env.current()->fees().base;
        env.fund(acctReserve, alice);
        env.close();

        // alice cannot cover the reserve for the Mystery.
        env(mystery::create(alice, drops(1), kPhrase), Ter(tecINSUFFICIENT_RESERVE));
        env.close();

        // alice covers the reserve, but not the reserve plus the prize.
        env(pay(env.master, alice, incReserve + drops(2 * baseFee)));
        env.close();
        env(mystery::create(alice, drops(1), kPhrase), Ter(tecUNFUNDED));
        env.close();
        BEAST_EXPECT(ownerCount(env, alice) == 0);
    }

    void
    testGuess(FeatureBitset features)
    {
        testcase("Guess");

        using namespace jtx;
        Env env{*this, features};
        Account const alice{"alice"};
        Account const bob{"bob"};
        Account const carol{"carol"};
        env.fund(XRP(5000), alice, bob, carol);
        env.close();

        auto const baseFee = env.current()->fees().base;
        auto const seq = env.seq(alice);
        env(mystery::create(alice, XRP(100), kPhrase));
        env.close();
        BEAST_EXPECT(env.le(mysteryKeylet(alice, seq)));
        BEAST_EXPECT(ownerCount(env, alice) == 1);
        BEAST_EXPECT(env.balance(alice) == XRP(4900) - baseFee);

        // A guess for a Mystery that does not exist.
        env(mystery::guess(bob, alice, seq + 1, kPhrase), Ter(tecNO_ENTRY));

        // The owner cannot claim their own prize.
        env(mystery::guess(alice, alice, seq, kPhrase), Ter(tecNO_PERMISSION));

        // A wrong guess costs the fee and changes nothing else.
        auto const bobBefore = env.balance(bob);
        env(mystery::guess(bob, alice, seq, "the gardener did it"), Ter(tecNO_PERMISSION));
        env.close();
        BEAST_EXPECT(env.balance(bob) == bobBefore - baseFee);
        BEAST_EXPECT(env.le(mysteryKeylet(alice, seq)));

        // The correct guess pays bob and deletes the Mystery.
        env(mystery::guess(bob, alice, seq, kPhrase));
        env.close();
        BEAST_EXPECT(env.balance(bob) == bobBefore - 2 * baseFee + XRP(100));
        BEAST_EXPECT(!env.le(mysteryKeylet(alice, seq)));
        BEAST_EXPECT(ownerCount(env, alice) == 0);

        // Only the first correct guess wins.
        env(mystery::guess(carol, alice, seq, kPhrase), Ter(tecNO_ENTRY));
    }

    void
    testCancel(FeatureBitset features)
    {
        testcase("Cancel");

        using namespace jtx;
        Env env{*this, features};
        Account const alice{"alice"};
        Account const bob{"bob"};
        env.fund(XRP(5000), alice, bob);
        env.close();

        auto const baseFee = env.current()->fees().base;
        auto const seq = env.seq(alice);
        env(mystery::create(alice, XRP(100), kPhrase));
        env.close();

        // bob does not own a Mystery with that sequence.
        env(mystery::cancel(bob, seq), Ter(tecNO_ENTRY));

        env(mystery::cancel(alice, seq));
        env.close();
        BEAST_EXPECT(!env.le(mysteryKeylet(alice, seq)));
        BEAST_EXPECT(ownerCount(env, alice) == 0);
        BEAST_EXPECT(env.balance(alice) == XRP(5000) - 2 * baseFee);

        env(mystery::cancel(alice, seq), Ter(tecNO_ENTRY));
        env(mystery::guess(bob, alice, seq, kPhrase), Ter(tecNO_ENTRY));
    }

    void
    testLedgerEntry(FeatureBitset features)
    {
        testcase("ledger_entry");

        using namespace jtx;
        Env env{*this, features};
        Account const alice{"alice"};
        env.fund(XRP(5000), alice);
        env.close();

        auto const seq = env.seq(alice);
        auto jv = mystery::create(alice, XRP(100), kPhrase);
        jv[sfData.jsonName] = strHex(std::string("Look where transactions are born."));
        env(jv);
        env.close();

        json::Value params;
        params[jss::ledger_index] = jss::validated;
        params[jss::mystery][jss::owner] = alice.human();
        params[jss::mystery][jss::seq] = seq;
        auto const result = env.rpc("json", "ledger_entry", to_string(params))[jss::result];
        BEAST_EXPECT(result[jss::index] == to_string(mysteryKeylet(alice, seq).key));
        BEAST_EXPECT(result[jss::node][sfData.jsonName] == jv[sfData.jsonName]);
        BEAST_EXPECT(result[jss::node][sfDigest.jsonName] == to_string(mystery::digest(kPhrase)));
    }

    void
    run() override
    {
        using namespace test::jtx;
        FeatureBitset const all{testableAmendments()};
        testEnabled(all);
        testPreflight(all);
        testReserve(all);
        testGuess(all);
        testCancel(all);
        testLedgerEntry(all);
    }
};

BEAST_DEFINE_TESTSUITE(Mystery, app, xrpl);

}  // namespace xrpl::test
