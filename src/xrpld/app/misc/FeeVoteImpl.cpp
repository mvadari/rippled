#include <xrpld/app/ledger/Ledger.h>
#include <xrpld/app/misc/FeeParamRegistry.h>
#include <xrpld/app/misc/FeeVote.h>

#include <xrpl/beast/utility/Journal.h>
#include <xrpl/protocol/STValidation.h>
#include <xrpl/protocol/st.h>

namespace xrpl {

namespace detail {

class VotableValue
{
private:
    using value_type = XRPAmount;
    value_type const current_;  // The current setting
    value_type const target_;   // The setting we want
    std::map<value_type, int> voteMap_;

public:
    VotableValue(value_type current, value_type target)
        : current_(current), target_(target)
    {
        // Add our vote
        ++voteMap_[target_];
    }

    void
    addVote(value_type vote)
    {
        ++voteMap_[vote];
    }

    void
    noVote()
    {
        addVote(current_);
    }

    value_type
    current() const
    {
        return current_;
    }

    std::pair<value_type, bool>
    getVotes() const;
};

auto
VotableValue::getVotes() const -> std::pair<value_type, bool>
{
    value_type ourVote = current_;
    int weight = 0;
    for (auto const& [key, val] : voteMap_)
    {
        // Take most voted value between current and target, inclusive
        if ((key <= std::max(target_, current_)) &&
            (key >= std::min(target_, current_)) && (val > weight))
        {
            ourVote = key;
            weight = val;
        }
    }

    return {ourVote, ourVote != current_};
}

}  // namespace detail

//------------------------------------------------------------------------------

class FeeVoteImpl : public FeeVote
{
private:
    FeeSetup target_;
    beast::Journal const journal_;

public:
    FeeVoteImpl(FeeSetup const& setup, beast::Journal journal);

    void
    doValidation(Fees const& lastFees, Rules const& rules, STValidation& val)
        override;

    void
    doVoting(
        std::shared_ptr<ReadView const> const& lastClosedLedger,
        std::vector<std::shared_ptr<STValidation>> const& parentValidations,
        std::shared_ptr<SHAMap> const& initialPosition) override;
};

//--------------------------------------------------------------------------

FeeVoteImpl::FeeVoteImpl(FeeSetup const& setup, beast::Journal journal)
    : target_(setup), journal_(journal)
{
}

void
FeeVoteImpl::doValidation(
    Fees const& lastFees,
    Rules const& rules,
    STValidation& v)
{
    // Values should always be in a valid range (because the voting process
    // will ignore out-of-range values) but if we detect such a case, we do
    // not send a value.
    if (rules.enabled(featureXRPFees))
    {
        for (auto const& param : feeParamRegistry())
        {
            auto const current = param.getFromFees(lastFees);
            auto const target = param.getFromSetup(target_);

            if (current != target)
            {
                JLOG(journal_.info())
                    << "Voting for " << param.humanLabel << " of " << target;

                v[*param.xrpFeesField] = target;
            }
        }
    }
    else
    {
        for (auto const& param : feeParamRegistry())
        {
            auto const current = param.getFromFees(lastFees);
            auto const target = param.getFromSetup(target_);

            if (current != target)
            {
                JLOG(journal_.info())
                    << "Voting for " << param.humanLabel << " of " << target;

                // Legacy mode uses either UINT32 or UINT64 fields
                if (param.legacyField64)
                {
                    if (auto const f = target.dropsAs<std::uint64_t>())
                        v[*param.legacyField64] = *f;
                }
                else if (param.legacyField32)
                {
                    if (auto const f = target.dropsAs<std::uint32_t>())
                        v[*param.legacyField32] = *f;
                }
            }
        }
    }
}

void
FeeVoteImpl::doVoting(
    std::shared_ptr<ReadView const> const& lastClosedLedger,
    std::vector<std::shared_ptr<STValidation>> const& set,
    std::shared_ptr<SHAMap> const& initialPosition)
{
    // LCL must be flag ledger
    XRPL_ASSERT(
        lastClosedLedger && isFlagLedger(lastClosedLedger->seq()),
        "xrpl::FeeVoteImpl::doVoting : has a flag ledger");

    auto const& currentFees = lastClosedLedger->fees();
    auto const& rules = lastClosedLedger->rules();
    auto const registry = feeParamRegistry();

    // Create VotableValue objects for each fee parameter
    std::array<detail::VotableValue, kFeeParamCount> votes = {{
        detail::VotableValue(
            registry[0].getFromFees(currentFees),
            registry[0].getFromSetup(target_)),
        detail::VotableValue(
            registry[1].getFromFees(currentFees),
            registry[1].getFromSetup(target_)),
        detail::VotableValue(
            registry[2].getFromFees(currentFees),
            registry[2].getFromSetup(target_)),
    }};

    // Process votes from validations
    if (rules.enabled(featureXRPFees))
    {
        for (auto const& val : set)
        {
            if (!val->isTrusted())
                continue;

            for (std::size_t i = 0; i < kFeeParamCount; ++i)
            {
                auto const& param = registry[i];
                if (auto const field = ~val->at(~*param.xrpFeesField);
                    field && field->native())
                {
                    auto const vote = field->xrp();
                    if (isLegalAmountSigned(vote))
                        votes[i].addVote(vote);
                    else
                        votes[i].noVote();
                }
                else
                {
                    votes[i].noVote();
                }
            }
        }
    }
    else
    {
        for (auto const& val : set)
        {
            if (!val->isTrusted())
                continue;

            for (std::size_t i = 0; i < kFeeParamCount; ++i)
            {
                auto const& param = registry[i];
                std::optional<XRPAmount> voteValue;

                // Read from either UINT64 or UINT32 legacy field
                if (param.legacyField64)
                {
                    if (auto const field = val->at(~*param.legacyField64))
                    {
                        using XRPType = XRPAmount::value_type;
                        auto const v = *field;
                        if (v <= static_cast<std::uint64_t>(
                                     std::numeric_limits<XRPType>::max()) &&
                            isLegalAmountSigned(
                                XRPAmount{unsafe_cast<XRPType>(v)}))
                            voteValue = XRPAmount{unsafe_cast<XRPType>(v)};
                    }
                }
                else if (param.legacyField32)
                {
                    if (auto const field = val->at(~*param.legacyField32))
                    {
                        // uint32_t always fits in XRPAmount::value_type
                        // (int64_t), so no range check needed
                        auto const v = XRPAmount{
                            static_cast<XRPAmount::value_type>(*field)};
                        if (isLegalAmountSigned(v))
                            voteValue = v;
                    }
                }

                if (voteValue)
                    votes[i].addVote(*voteValue);
                else
                    votes[i].noVote();
            }
        }
    }

    // Get vote results for each parameter
    std::array<std::pair<XRPAmount, bool>, kFeeParamCount> results;
    for (std::size_t i = 0; i < kFeeParamCount; ++i)
    {
        results[i] = votes[i].getVotes();
    }

    auto const seq = lastClosedLedger->header().seq + 1;

    // Check if any parameter needs a vote
    bool needsVote = false;
    for (auto const& result : results)
    {
        if (result.second)
        {
            needsVote = true;
            break;
        }
    }

    // add transactions to our position
    if (needsVote)
    {
        JLOG(journal_.warn())
            << "We are voting for a fee change: " << results[0].first << "/"
            << results[1].first << "/" << results[2].first;

        STTx feeTx(ttFEE, [&](auto& obj) {
            obj[sfAccount] = AccountID();
            obj[sfLedgerSequence] = seq;
            if (rules.enabled(featureXRPFees))
            {
                for (std::size_t i = 0; i < kFeeParamCount; ++i)
                {
                    obj[*registry[i].xrpFeesField] = results[i].first;
                }
            }
            else
            {
                // Without the featureXRPFees amendment, these fields are
                // required.
                for (std::size_t i = 0; i < kFeeParamCount; ++i)
                {
                    auto const& param = registry[i];
                    if (param.legacyField64)
                    {
                        obj[*param.legacyField64] =
                            results[i].first.dropsAs<std::uint64_t>(
                                votes[i].current());
                    }
                    else if (param.legacyField32)
                    {
                        obj[*param.legacyField32] =
                            results[i].first.dropsAs<std::uint32_t>(
                                votes[i].current());
                    }
                }
                obj[sfReferenceFeeUnits] = Config::FEE_UNITS_DEPRECATED;
            }
        });

        uint256 txID = feeTx.getTransactionID();

        JLOG(journal_.warn()) << "Vote: " << txID;

        Serializer s;
        feeTx.add(s);

        if (!initialPosition->addGiveItem(
                SHAMapNodeType::tnTRANSACTION_NM,
                make_shamapitem(txID, s.slice())))
        {
            JLOG(journal_.warn()) << "Ledger already had fee change";
        }
    }
}

//------------------------------------------------------------------------------

std::unique_ptr<FeeVote>
make_FeeVote(FeeSetup const& setup, beast::Journal journal)
{
    return std::make_unique<FeeVoteImpl>(setup, journal);
}

}  // namespace xrpl
