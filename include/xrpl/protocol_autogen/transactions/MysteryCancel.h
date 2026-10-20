// This file is auto-generated. Do not edit.
#pragma once

#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol/STParsedJSON.h>
#include <xrpl/protocol/jss.h>
#include <xrpl/protocol_autogen/TransactionBase.h>
#include <xrpl/protocol_autogen/TransactionBuilderBase.h>
#include <xrpl/json/json_value.h>

#include <stdexcept>
#include <optional>

namespace xrpl::transactions {

class MysteryCancelBuilder;

/**
 * @brief Transaction: MysteryCancel
 *
 * Type: ttMYSTERY_CANCEL (98)
 * Delegable: Delegation::NotDelegable
 * Amendment: featureMystery
 * Privileges: Privilege::NoPriv
 *
 * Immutable wrapper around STTx providing type-safe field access.
 * Use MysteryCancelBuilder to construct new transactions.
 */
class MysteryCancel : public TransactionBase
{
public:
    static constexpr xrpl::TxType txType = ttMYSTERY_CANCEL;

    /**
     * @brief Construct a MysteryCancel transaction wrapper from an existing STTx object.
     * @throws std::runtime_error if the transaction type doesn't match.
     */
    explicit MysteryCancel(std::shared_ptr<STTx const> tx)
        : TransactionBase(std::move(tx))
    {
        // Verify transaction type
        if (tx_->getTxnType() != txType)
        {
            throw std::runtime_error("Invalid transaction type for MysteryCancel");
        }
    }

    // Transaction-specific field getters

    /**
     * @brief Get sfOfferSequence (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_UINT32::type::value_type
    getOfferSequence() const
    {
        return this->tx_->at(sfOfferSequence);
    }
};

/**
 * @brief Builder for MysteryCancel transactions.
 *
 * Provides a fluent interface for constructing transactions with method chaining.
 * Uses STObject internally for flexible transaction construction.
 * Inherits common field setters from TransactionBuilderBase.
 */
class MysteryCancelBuilder : public TransactionBuilderBase<MysteryCancelBuilder>
{
public:
    /**
     * @brief Construct a new MysteryCancelBuilder with required fields.
     * @param account The account initiating the transaction.
     * @param offerSequence The sfOfferSequence field value.
     * @param sequence Optional sequence number for the transaction.
     * @param fee Optional fee for the transaction.
     */
    MysteryCancelBuilder(SF_ACCOUNT::type::value_type account,
                     std::decay_t<typename SF_UINT32::type::value_type> const& offerSequence,                    std::optional<SF_UINT32::type::value_type> sequence = std::nullopt,
                    std::optional<SF_AMOUNT::type::value_type> fee = std::nullopt
)
        : TransactionBuilderBase<MysteryCancelBuilder>(ttMYSTERY_CANCEL, account, sequence, fee)
    {
        setOfferSequence(offerSequence);
    }

    /**
     * @brief Construct a MysteryCancelBuilder from an existing STTx object.
     * @param tx The existing transaction to copy from.
     * @throws std::runtime_error if the transaction type doesn't match.
     */
    MysteryCancelBuilder(std::shared_ptr<STTx const> tx)
    {
        if (tx->getTxnType() != ttMYSTERY_CANCEL)
        {
            throw std::runtime_error("Invalid transaction type for MysteryCancelBuilder");
        }
        object_ = *tx;
    }

    /**
     * @brief Transaction-specific field setters
     */

    /**
     * @brief Set sfOfferSequence (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    MysteryCancelBuilder&
    setOfferSequence(std::decay_t<typename SF_UINT32::type::value_type> const& value)
    {
        object_[sfOfferSequence] = value;
        return *this;
    }

    /**
     * @brief Build and return the MysteryCancel wrapper.
     * @param publicKey The public key for signing.
     * @param secretKey The secret key for signing.
     * @return The constructed transaction wrapper.
     */
    MysteryCancel
    build(PublicKey const& publicKey, SecretKey const& secretKey)
    {
        sign(publicKey, secretKey);
        return MysteryCancel{std::make_shared<STTx>(std::move(object_))};
    }
};

}  // namespace xrpl::transactions
