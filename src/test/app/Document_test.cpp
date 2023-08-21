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

#include <ripple/basics/strHex.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/protocol/jss.h>
#include <test/jtx.h>

#include <algorithm>
#include <iterator>

namespace ripple {
namespace test {

// Helper function that returns the owner count of an account root.
std::uint32_t
ownerCount(test::jtx::Env const& env, test::jtx::Account const& acct)
{
    std::uint32_t ret{0};
    if (auto const sleAcct = env.le(acct))
        ret = sleAcct->at(sfOwnerCount);
    return ret;
}

bool
checkVL(Slice const& result, std::string expected)
{
    Serializer s;
    s.addRaw(result);
    return s.getString() == expected;
}

struct Document_test : public beast::unit_test::suite
{
    void
    testEnabled(FeatureBitset features)
    {
        testcase("Enabled");

        using namespace jtx;
        // If the Document amendment is not enabled, you should not be able
        // to set or delete Documents.
        Env env{*this, features - featureDocument};
        Account const alice{"alice"};
        env.fund(XRP(5000), alice);
        env.close();

        BEAST_EXPECT(ownerCount(env, alice) == 0);
        env(document::setValid(alice), ter(temDISABLED));
        env.close();

        BEAST_EXPECT(ownerCount(env, alice) == 0);
        env(document::del(alice, 0), ter(temDISABLED));
        env.close();
    }

    void
    testAccountReserve(FeatureBitset features)
    {
        // Verify that the reserve behaves as expected for minting.
        testcase("Account reserve");

        using namespace test::jtx;

        Env env{*this, features};
        Account const alice{"alice"};

        // Fund alice enough to exist, but not enough to meet
        // the reserve for creating a Document.
        auto const acctReserve = env.current()->fees().accountReserve(0);
        auto const incReserve = env.current()->fees().increment;
        env.fund(acctReserve, alice);
        env.close();
        BEAST_EXPECT(env.balance(alice) == acctReserve);
        BEAST_EXPECT(ownerCount(env, alice) == 0);

        // alice does not have enough XRP to cover the reserve for a Document
        env(document::setValid(alice), ter(tecINSUFFICIENT_RESERVE));
        env.close();
        BEAST_EXPECT(ownerCount(env, alice) == 0);

        // Pay alice almost enough to make the reserve for a Document.
        env(pay(env.master, alice, incReserve + drops(19)));
        BEAST_EXPECT(env.balance(alice) == acctReserve + incReserve + drops(9));
        env.close();

        // alice still does not have enough XRP for the reserve of a Document.
        env(document::setValid(alice), ter(tecINSUFFICIENT_RESERVE));
        env.close();
        BEAST_EXPECT(ownerCount(env, alice) == 0);

        // Pay alice enough to make the reserve for a Document.
        env(pay(env.master, alice, drops(11)));
        env.close();

        // Now alice can create a Document.
        env(document::setValid(alice));
        env.close();
        BEAST_EXPECT(ownerCount(env, alice) == 1);

        // alice deletes her Document.
        env(document::del(alice, 0));
        BEAST_EXPECT(ownerCount(env, alice) == 0);
        env.close();
    }

    void
    testSetInvalid(FeatureBitset features)
    {
        testcase("Invalid Set");

        using namespace jtx;
        using namespace std::chrono;

        Env env(*this);
        Account const alice{"alice"};
        env.fund(XRP(5000), alice);
        env.close();

        //----------------------------------------------------------------------
        // preflight

        // invalid flags
        BEAST_EXPECT(ownerCount(env, alice) == 0);
        env(document::setValid(alice),
            txflags(0x00010000),
            ter(temINVALID_FLAG));
        env.close();
        BEAST_EXPECT(ownerCount(env, alice) == 0);

        // no Data/URI fields
        env(document::set(alice, 0), ter(temEMPTY_DOCUMENT));
        env.close();
        BEAST_EXPECT(ownerCount(env, alice) == 0);

        // both Data/URI empty fields
        env(document::set(alice, 0),
            document::uri(""),
            document::data(""),
            ter(temEMPTY_DOCUMENT));
        env.close();
        BEAST_EXPECT(ownerCount(env, alice) == 0);

        // uri is too long
        const std::string longString(257, 'a');
        env(document::set(alice, 2),
            document::uri(longString),
            ter(temMALFORMED));
        env.close();
        BEAST_EXPECT(ownerCount(env, alice) == 0);

        // data is too long
        env(document::set(alice, 4),
            document::data(longString),
            ter(temMALFORMED));
        env.close();
        BEAST_EXPECT(ownerCount(env, alice) == 0);

        // Modifying a Document to become empty is checked in testSetModify
    }

    void
    testDeleteInvalid(FeatureBitset features)
    {
        testcase("Invalid Delete");

        using namespace jtx;
        using namespace std::chrono;

        Env env(*this);
        Account const alice{"alice"};
        Account const bob{"bob"};
        env.fund(XRP(5000), alice, bob);
        env.close();

        //----------------------------------------------------------------------
        // preflight

        // invalid flags
        BEAST_EXPECT(ownerCount(env, alice) == 0);
        env(document::del(alice, 23),
            txflags(0x00010000),
            ter(temINVALID_FLAG));
        env.close();
        BEAST_EXPECT(ownerCount(env, alice) == 0);

        //----------------------------------------------------------------------
        // doApply

        // Document doesn't exist
        env(document::del(alice, 0), ter(tecNO_ENTRY));
        env.close();
        BEAST_EXPECT(ownerCount(env, alice) == 0);

        // Account owns a document, but wrong DocumentNumber
        env(document::set(bob, 2), document::uri("uri"));
        BEAST_EXPECT(ownerCount(env, bob) == 1);
        env(document::del(bob, 3), ter(tecNO_ENTRY));
        env.close();
        BEAST_EXPECT(ownerCount(env, bob) == 1);
    }

    void
    testSetValidInitial(FeatureBitset features)
    {
        testcase("Valid Initial Set");

        using namespace jtx;
        using namespace std::chrono;

        Env env(*this);
        Account const alice{"alice"};
        Account const bob{"bob"};
        Account const charlie{"charlie"};
        env.fund(XRP(5000), alice, bob, charlie);
        env.close();
        BEAST_EXPECT(ownerCount(env, alice) == 0);
        BEAST_EXPECT(ownerCount(env, bob) == 0);
        BEAST_EXPECT(ownerCount(env, charlie) == 0);

        // only URI
        env(document::set(alice, 0), document::uri("uri"));
        BEAST_EXPECT(ownerCount(env, alice) == 1);

        // only Data
        env(document::set(bob, 1), document::data("data"));
        BEAST_EXPECT(ownerCount(env, bob) == 1);

        // both URI and Data
        env(document::set(charlie, 65536),
            document::uri("uri"),
            document::data("data"));
        BEAST_EXPECT(ownerCount(env, charlie) == 1);
    }

    void
    testSetModify(FeatureBitset features)
    {
        testcase("Modify Document with Set");

        using namespace jtx;
        using namespace std::chrono;

        Env env(*this);
        Account const alice{"alice"};
        env.fund(XRP(5000), alice);
        env.close();
        BEAST_EXPECT(ownerCount(env, alice) == 0);
        auto const ar = env.le(alice);

        // Create Document
        std::string const initialURI = "uri";
        std::uint32_t const docNum = 3;
        {
            env(document::set(alice, docNum), document::uri(initialURI));
            BEAST_EXPECT(ownerCount(env, alice) == 1);
            auto const sleDoc = env.le(keylet::document(alice.id(), docNum));
            BEAST_EXPECT(sleDoc);
            BEAST_EXPECT(checkVL((*sleDoc)[sfURI], initialURI));
            BEAST_EXPECT((*sleDoc)[sfDocumentNumber] == docNum);
            BEAST_EXPECT(!sleDoc->isFieldPresent(sfData));
        }

        // Try to delete URI, fails because no elements are set
        {
            env(document::set(alice, docNum),
                document::uri(""),
                ter(tecEMPTY_DOCUMENT));
            BEAST_EXPECT(ownerCount(env, alice) == 1);
            auto const sleDoc = env.le(keylet::document(alice.id(), docNum));
            BEAST_EXPECT(checkVL((*sleDoc)[sfURI], initialURI));
            BEAST_EXPECT(!sleDoc->isFieldPresent(sfData));
        }

        // Set Data
        std::string const initialData = "data";
        {
            env(document::set(alice, docNum), document::data("data"));
            BEAST_EXPECT(ownerCount(env, alice) == 1);
            auto const sleDoc = env.le(keylet::document(alice.id(), docNum));
            BEAST_EXPECT(checkVL((*sleDoc)[sfURI], initialURI));
            BEAST_EXPECT(checkVL((*sleDoc)[sfData], initialData));
        }

        // Remove URI
        {
            env(document::set(alice, docNum), document::uri(""));
            BEAST_EXPECT(ownerCount(env, alice) == 1);
            auto const sleDoc = env.le(keylet::document(alice.id(), docNum));
            BEAST_EXPECT(!sleDoc->isFieldPresent(sfURI));
            BEAST_EXPECT(checkVL((*sleDoc)[sfData], initialData));
        }

        // Remove Data + set URI
        std::string const secondURI = "uri2";
        {
            env(document::set(alice, docNum),
                document::uri(secondURI),
                document::data(""));
            BEAST_EXPECT(ownerCount(env, alice) == 1);
            auto const sleDoc = env.le(keylet::document(alice.id(), docNum));
            BEAST_EXPECT(checkVL((*sleDoc)[sfURI], secondURI));
            BEAST_EXPECT(!sleDoc->isFieldPresent(sfData));
        }

        // Remove URI + set Data
        std::string const secondData = "data2";
        {
            env(document::set(alice, docNum),
                document::uri(""),
                document::data(secondData));
            BEAST_EXPECT(ownerCount(env, alice) == 1);
            auto const sleDoc = env.le(keylet::document(alice.id(), docNum));
            BEAST_EXPECT(!sleDoc->isFieldPresent(sfURI));
            BEAST_EXPECT(checkVL((*sleDoc)[sfData], secondData));
        }

        // Delete Document
        {
            env(document::del(alice, docNum));
            BEAST_EXPECT(ownerCount(env, alice) == 0);
            auto const sleDoc = env.le(keylet::document(alice.id(), docNum));
            BEAST_EXPECT(!sleDoc);
        }
    }

    void
    run() override
    {
        using namespace test::jtx;
        FeatureBitset const all{supported_amendments()};
        testEnabled(all);
        testAccountReserve(all);
        testSetInvalid(all);
        testDeleteInvalid(all);
        testSetModify(all);
    }
};

BEAST_DEFINE_TESTSUITE(Document, app, ripple);

}  // namespace test
}  // namespace ripple
