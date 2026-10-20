// Auto-generated unit tests for ledger entry Mystery


#include <gtest/gtest.h>

#include <protocol_autogen/TestHelpers.h>

#include <xrpl/protocol/STLedgerEntry.h>
#include <xrpl/protocol_autogen/ledger_entries/Mystery.h>
#include <xrpl/protocol_autogen/ledger_entries/Ticket.h>

#include <string>

namespace xrpl::ledger_entries {

// 1 & 4) Set fields via builder setters, build, then read them back via
// wrapper getters. After build(), validate() should succeed for both the
// builder's STObject and the wrapper's SLE.
TEST(MysteryTests, BuilderSettersRoundTrip)
{
    UInt256 const index{1u};

    auto const accountValue = canonical_ACCOUNT();
    auto const sequenceValue = canonical_UINT32();
    auto const amountValue = canonical_AMOUNT();
    auto const digestValue = canonical_UINT256();
    auto const dataValue = canonical_VL();
    auto const ownerNodeValue = canonical_UINT64();
    auto const previousTxnIDValue = canonical_UINT256();
    auto const previousTxnLgrSeqValue = canonical_UINT32();

    MysteryBuilder builder{
        accountValue,
        sequenceValue,
        amountValue,
        digestValue,
        ownerNodeValue,
        previousTxnIDValue,
        previousTxnLgrSeqValue
    };

    builder.setData(dataValue);

    builder.setLedgerIndex(index);
    builder.setFlags(0x1u);

    EXPECT_TRUE(builder.validate());

    auto const entry = builder.build(index);

    EXPECT_TRUE(entry.validate());

    {
        auto const& expected = accountValue;
        auto const actual = entry.getAccount();
        expectEqualField(expected, actual, "sfAccount");
    }

    {
        auto const& expected = sequenceValue;
        auto const actual = entry.getSequence();
        expectEqualField(expected, actual, "sfSequence");
    }

    {
        auto const& expected = amountValue;
        auto const actual = entry.getAmount();
        expectEqualField(expected, actual, "sfAmount");
    }

    {
        auto const& expected = digestValue;
        auto const actual = entry.getDigest();
        expectEqualField(expected, actual, "sfDigest");
    }

    {
        auto const& expected = ownerNodeValue;
        auto const actual = entry.getOwnerNode();
        expectEqualField(expected, actual, "sfOwnerNode");
    }

    {
        auto const& expected = previousTxnIDValue;
        auto const actual = entry.getPreviousTxnID();
        expectEqualField(expected, actual, "sfPreviousTxnID");
    }

    {
        auto const& expected = previousTxnLgrSeqValue;
        auto const actual = entry.getPreviousTxnLgrSeq();
        expectEqualField(expected, actual, "sfPreviousTxnLgrSeq");
    }

    {
        auto const& expected = dataValue;
        auto const actualOpt = entry.getData();
        ASSERT_TRUE(actualOpt.has_value());
        expectEqualField(expected, *actualOpt, "sfData");
        EXPECT_TRUE(entry.hasData());
    }

    EXPECT_TRUE(entry.hasLedgerIndex());
    auto const ledgerIndex = entry.getLedgerIndex();
    ASSERT_TRUE(ledgerIndex.has_value());
    EXPECT_EQ(*ledgerIndex, index);
    EXPECT_EQ(entry.getKey(), index);
}

// 2 & 4) Start from an SLE, set fields directly on it, construct a builder
// from that SLE, build a new wrapper, and verify all fields (and validate()).
TEST(MysteryTests, BuilderFromSleRoundTrip)
{
    UInt256 const index{2u};

    auto const accountValue = canonical_ACCOUNT();
    auto const sequenceValue = canonical_UINT32();
    auto const amountValue = canonical_AMOUNT();
    auto const digestValue = canonical_UINT256();
    auto const dataValue = canonical_VL();
    auto const ownerNodeValue = canonical_UINT64();
    auto const previousTxnIDValue = canonical_UINT256();
    auto const previousTxnLgrSeqValue = canonical_UINT32();

    auto sle = std::make_shared<SLE>(Mystery::entryType, index);

    sle->at(sfAccount) = accountValue;
    sle->at(sfSequence) = sequenceValue;
    sle->at(sfAmount) = amountValue;
    sle->at(sfDigest) = digestValue;
    sle->at(sfData) = dataValue;
    sle->at(sfOwnerNode) = ownerNodeValue;
    sle->at(sfPreviousTxnID) = previousTxnIDValue;
    sle->at(sfPreviousTxnLgrSeq) = previousTxnLgrSeqValue;

    MysteryBuilder builderFromSle{sle};
    EXPECT_TRUE(builderFromSle.validate());

    auto const entryFromBuilder = builderFromSle.build(index);

    Mystery entryFromSle{sle};
    EXPECT_TRUE(entryFromBuilder.validate());
    EXPECT_TRUE(entryFromSle.validate());

    {
        auto const& expected = accountValue;

        auto const fromSle = entryFromSle.getAccount();
        auto const fromBuilder = entryFromBuilder.getAccount();

        expectEqualField(expected, fromSle, "sfAccount");
        expectEqualField(expected, fromBuilder, "sfAccount");
    }

    {
        auto const& expected = sequenceValue;

        auto const fromSle = entryFromSle.getSequence();
        auto const fromBuilder = entryFromBuilder.getSequence();

        expectEqualField(expected, fromSle, "sfSequence");
        expectEqualField(expected, fromBuilder, "sfSequence");
    }

    {
        auto const& expected = amountValue;

        auto const fromSle = entryFromSle.getAmount();
        auto const fromBuilder = entryFromBuilder.getAmount();

        expectEqualField(expected, fromSle, "sfAmount");
        expectEqualField(expected, fromBuilder, "sfAmount");
    }

    {
        auto const& expected = digestValue;

        auto const fromSle = entryFromSle.getDigest();
        auto const fromBuilder = entryFromBuilder.getDigest();

        expectEqualField(expected, fromSle, "sfDigest");
        expectEqualField(expected, fromBuilder, "sfDigest");
    }

    {
        auto const& expected = ownerNodeValue;

        auto const fromSle = entryFromSle.getOwnerNode();
        auto const fromBuilder = entryFromBuilder.getOwnerNode();

        expectEqualField(expected, fromSle, "sfOwnerNode");
        expectEqualField(expected, fromBuilder, "sfOwnerNode");
    }

    {
        auto const& expected = previousTxnIDValue;

        auto const fromSle = entryFromSle.getPreviousTxnID();
        auto const fromBuilder = entryFromBuilder.getPreviousTxnID();

        expectEqualField(expected, fromSle, "sfPreviousTxnID");
        expectEqualField(expected, fromBuilder, "sfPreviousTxnID");
    }

    {
        auto const& expected = previousTxnLgrSeqValue;

        auto const fromSle = entryFromSle.getPreviousTxnLgrSeq();
        auto const fromBuilder = entryFromBuilder.getPreviousTxnLgrSeq();

        expectEqualField(expected, fromSle, "sfPreviousTxnLgrSeq");
        expectEqualField(expected, fromBuilder, "sfPreviousTxnLgrSeq");
    }

    {
        auto const& expected = dataValue;

        auto const fromSleOpt = entryFromSle.getData();
        auto const fromBuilderOpt = entryFromBuilder.getData();

        ASSERT_TRUE(fromSleOpt.has_value());
        ASSERT_TRUE(fromBuilderOpt.has_value());

        expectEqualField(expected, *fromSleOpt, "sfData");
        expectEqualField(expected, *fromBuilderOpt, "sfData");
    }

    EXPECT_EQ(entryFromSle.getKey(), index);
    EXPECT_EQ(entryFromBuilder.getKey(), index);
}

// 3) Verify wrapper throws when constructed from wrong ledger entry type.
TEST(MysteryTests, WrapperThrowsOnWrongEntryType)
{
    UInt256 const index{3u};

    // Build a valid ledger entry of a different type
    // Ticket requires: Account, OwnerNode, TicketSequence, PreviousTxnID, PreviousTxnLgrSeq
    // Check requires: Account, Destination, SendMax, Sequence, OwnerNode, DestinationNode, PreviousTxnID, PreviousTxnLgrSeq
    TicketBuilder wrongBuilder{
        canonical_ACCOUNT(),
        canonical_UINT64(),
        canonical_UINT32(),
        canonical_UINT256(),
        canonical_UINT32()};
    auto wrongEntry = wrongBuilder.build(index);

    EXPECT_THROW(Mystery{wrongEntry.getSle()}, std::runtime_error);
}

// 4) Verify builder throws when constructed from wrong ledger entry type.
TEST(MysteryTests, BuilderThrowsOnWrongEntryType)
{
    UInt256 const index{4u};

    // Build a valid ledger entry of a different type
    TicketBuilder wrongBuilder{
        canonical_ACCOUNT(),
        canonical_UINT64(),
        canonical_UINT32(),
        canonical_UINT256(),
        canonical_UINT32()};
    auto wrongEntry = wrongBuilder.build(index);

    EXPECT_THROW(MysteryBuilder{wrongEntry.getSle()}, std::runtime_error);
}

// 5) Build with only required fields and verify optional fields return nullopt.
TEST(MysteryTests, OptionalFieldsReturnNullopt)
{
    UInt256 const index{3u};

    auto const accountValue = canonical_ACCOUNT();
    auto const sequenceValue = canonical_UINT32();
    auto const amountValue = canonical_AMOUNT();
    auto const digestValue = canonical_UINT256();
    auto const ownerNodeValue = canonical_UINT64();
    auto const previousTxnIDValue = canonical_UINT256();
    auto const previousTxnLgrSeqValue = canonical_UINT32();

    MysteryBuilder builder{
        accountValue,
        sequenceValue,
        amountValue,
        digestValue,
        ownerNodeValue,
        previousTxnIDValue,
        previousTxnLgrSeqValue
    };

    auto const entry = builder.build(index);

    // Verify optional fields are not present
    EXPECT_FALSE(entry.hasData());
    EXPECT_FALSE(entry.getData().has_value());
}
}
