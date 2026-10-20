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

class MysteryGuessBuilder;

/**
 * @brief Transaction: MysteryGuess
 *
 * Type: ttMYSTERY_GUESS (97)
 * Delegable: Delegation::NotDelegable
 * Amendment: featureMystery
 * Privileges: Privilege::NoPriv
 *
 * Immutable wrapper around STTx providing type-safe field access.
 * Use MysteryGuessBuilder to construct new transactions.
 */
class MysteryGuess : public TransactionBase
{
public:
    static constexpr xrpl::TxType txType = ttMYSTERY_GUESS;

    /**
     * @brief Construct a MysteryGuess transaction wrapper from an existing STTx object.
     * @throws std::runtime_error if the transaction type doesn't match.
     */
    explicit MysteryGuess(std::shared_ptr<STTx const> tx)
        : TransactionBase(std::move(tx))
    {
        // Verify transaction type
        if (tx_->getTxnType() != txType)
        {
            throw std::runtime_error("Invalid transaction type for MysteryGuess");
        }
    }

    // Transaction-specific field getters

    /**
     * @brief Get sfOwner (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_ACCOUNT::type::value_type
    getOwner() const
    {
        return this->tx_->at(sfOwner);
    }

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

    /**
     * @brief Get sfGuess (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_VL::type::value_type
    getGuess() const
    {
        return this->tx_->at(sfGuess);
    }
};

/**
 * @brief Builder for MysteryGuess transactions.
 *
 * Provides a fluent interface for constructing transactions with method chaining.
 * Uses STObject internally for flexible transaction construction.
 * Inherits common field setters from TransactionBuilderBase.
 */
class MysteryGuessBuilder : public TransactionBuilderBase<MysteryGuessBuilder>
{
public:
    /**
     * @brief Construct a new MysteryGuessBuilder with required fields.
     * @param account The account initiating the transaction.
     * @param owner The sfOwner field value.
     * @param offerSequence The sfOfferSequence field value.
     * @param guess The sfGuess field value.
     * @param sequence Optional sequence number for the transaction.
     * @param fee Optional fee for the transaction.
     */
    MysteryGuessBuilder(SF_ACCOUNT::type::value_type account,
                     std::decay_t<typename SF_ACCOUNT::type::value_type> const& owner,                     std::decay_t<typename SF_UINT32::type::value_type> const& offerSequence,                     std::decay_t<typename SF_VL::type::value_type> const& guess,                    std::optional<SF_UINT32::type::value_type> sequence = std::nullopt,
                    std::optional<SF_AMOUNT::type::value_type> fee = std::nullopt
)
        : TransactionBuilderBase<MysteryGuessBuilder>(ttMYSTERY_GUESS, account, sequence, fee)
    {
        setOwner(owner);
        setOfferSequence(offerSequence);
        setGuess(guess);
    }

    /**
     * @brief Construct a MysteryGuessBuilder from an existing STTx object.
     * @param tx The existing transaction to copy from.
     * @throws std::runtime_error if the transaction type doesn't match.
     */
    MysteryGuessBuilder(std::shared_ptr<STTx const> tx)
    {
        if (tx->getTxnType() != ttMYSTERY_GUESS)
        {
            throw std::runtime_error("Invalid transaction type for MysteryGuessBuilder");
        }
        object_ = *tx;
    }

    /**
     * @brief Transaction-specific field setters
     */

    /**
     * @brief Set sfOwner (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    MysteryGuessBuilder&
    setOwner(std::decay_t<typename SF_ACCOUNT::type::value_type> const& value)
    {
        object_[sfOwner] = value;
        return *this;
    }

    /**
     * @brief Set sfOfferSequence (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    MysteryGuessBuilder&
    setOfferSequence(std::decay_t<typename SF_UINT32::type::value_type> const& value)
    {
        object_[sfOfferSequence] = value;
        return *this;
    }

    /**
     * @brief Set sfGuess (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    MysteryGuessBuilder&
    setGuess(std::decay_t<typename SF_VL::type::value_type> const& value)
    {
        object_[sfGuess] = value;
        return *this;
    }

    /**
     * @brief Build and return the MysteryGuess wrapper.
     * @param publicKey The public key for signing.
     * @param secretKey The secret key for signing.
     * @return The constructed transaction wrapper.
     */
    MysteryGuess
    build(PublicKey const& publicKey, SecretKey const& secretKey)
    {
        sign(publicKey, secretKey);
        return MysteryGuess{std::make_shared<STTx>(std::move(object_))};
    }
};

}  // namespace xrpl::transactions
