//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2025 Ripple Labs Inc.

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

#include <test/jtx.h>

#include <xrpl/protocol/Feature.h>

namespace ripple {

class SavedSignature_test : public beast::unit_test::suite
{
public:
    void
    testBasic()
    {
        using namespace test::jtx;

        testcase("basic");
        Env env{*this, testable_amendments()};
        Account const alice("alice");
        Account const bob("bob");
        Account const charlie("charlie");
        env.fund(XRP(10000), alice, bob, charlie);

        env(signers(alice, 2, {{bob, 1}, {charlie, 1}}));
        env(fset(alice, asfDisableMaster), sig(alice));
        env.close();
        env.require(owners(alice, 1));

        auto json = pay(alice, bob, XRP(5));
        json[jss::Flags] = tfSaveSignature;
        JTx jt(std::forward<Json::Value>(json));
        jt.fill_sig = false;

        env(jt.jv, msig(bob), fee(20));
        env.close();
        std::cout << env.meta()->getJson(0).toStyledString() << std::endl;
        env.require(owners(bob, 1));

        env(jt.jv, msig(charlie), fee(20));
        env.close();
        std::cout << env.meta()->getJson(0).toStyledString() << std::endl;
        env.require(owners(bob, 0));

        std::cout << env.balance(alice) << std::endl;
    }

    void
    run() override
    {
        testBasic();
    }
};

BEAST_DEFINE_TESTSUITE(SavedSignature, app, ripple);

}  // namespace ripple
