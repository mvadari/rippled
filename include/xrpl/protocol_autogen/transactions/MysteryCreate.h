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

class MysteryCreateBuilder;

/**
 * @brief Transaction: MysteryCreate
 *
 * Type: ttMYSTERY_CREATE (96)
 * Delegable: Delegation::NotDelegable
 * Amendment: featureMystery
 * Privileges: Privilege::NoPriv
 *
 * Immutable wrapper around STTx providing type-safe field access.
 * Use MysteryCreateBuilder to construct new transactions.
 */
class MysteryCreate : public TransactionBase
{
public:
    static constexpr xrpl::TxType txType = ttMYSTERY_CREATE;

    /**
     * @brief Construct a MysteryCreate transaction wrapper from an existing STTx object.
     * @throws std::runtime_error if the transaction type doesn't match.
     */
    explicit MysteryCreate(std::shared_ptr<STTx const> tx)
        : TransactionBase(std::move(tx))
    {
        // Verify transaction type
        if (tx_->getTxnType() != txType)
        {
            throw std::runtime_error("Invalid transaction type for MysteryCreate");
        }
    }

    // Transaction-specific field getters

    /**
     * @brief Get sfAmount (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_AMOUNT::type::value_type
    getAmount() const
    {
        return this->tx_->at(sfAmount);
    }

    /**
     * @brief Get sfDigest (SoeRequired)
     * @return The field value.
     */
    [[nodiscard]]
    SF_UINT256::type::value_type
    getDigest() const
    {
        return this->tx_->at(sfDigest);
    }

    /**
     * @brief Get sfData (SoeOptional)
     * @return The field value, or std::nullopt if not present.
     */
    [[nodiscard]]
    protocol_autogen::Optional<SF_VL::type::value_type>
    getData() const
    {
        if (hasData())
        {
            return this->tx_->at(sfData);
        }
        return std::nullopt;
    }

    /**
     * @brief Check if sfData is present.
     * @return True if the field is present, false otherwise.
     */
    [[nodiscard]]
    bool
    hasData() const
    {
        return this->tx_->isFieldPresent(sfData);
    }
};

/**
 * @brief Builder for MysteryCreate transactions.
 *
 * Provides a fluent interface for constructing transactions with method chaining.
 * Uses STObject internally for flexible transaction construction.
 * Inherits common field setters from TransactionBuilderBase.
 */
class MysteryCreateBuilder : public TransactionBuilderBase<MysteryCreateBuilder>
{
public:
    /**
     * @brief Construct a new MysteryCreateBuilder with required fields.
     * @param account The account initiating the transaction.
     * @param amount The sfAmount field value.
     * @param digest The sfDigest field value.
     * @param sequence Optional sequence number for the transaction.
     * @param fee Optional fee for the transaction.
     */
    MysteryCreateBuilder(SF_ACCOUNT::type::value_type account,
                     std::decay_t<typename SF_AMOUNT::type::value_type> const& amount,                     std::decay_t<typename SF_UINT256::type::value_type> const& digest,                    std::optional<SF_UINT32::type::value_type> sequence = std::nullopt,
                    std::optional<SF_AMOUNT::type::value_type> fee = std::nullopt
)
        : TransactionBuilderBase<MysteryCreateBuilder>(ttMYSTERY_CREATE, account, sequence, fee)
    {
        setAmount(amount);
        setDigest(digest);
    }

    /**
     * @brief Construct a MysteryCreateBuilder from an existing STTx object.
     * @param tx The existing transaction to copy from.
     * @throws std::runtime_error if the transaction type doesn't match.
     */
    MysteryCreateBuilder(std::shared_ptr<STTx const> tx)
    {
        if (tx->getTxnType() != ttMYSTERY_CREATE)
        {
            throw std::runtime_error("Invalid transaction type for MysteryCreateBuilder");
        }
        object_ = *tx;
    }

    /**
     * @brief Transaction-specific field setters
     */

    /**
     * @brief Set sfAmount (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    MysteryCreateBuilder&
    setAmount(std::decay_t<typename SF_AMOUNT::type::value_type> const& value)
    {
        object_[sfAmount] = value;
        return *this;
    }

    /**
     * @brief Set sfDigest (SoeRequired)
     * @return Reference to this builder for method chaining.
     */
    MysteryCreateBuilder&
    setDigest(std::decay_t<typename SF_UINT256::type::value_type> const& value)
    {
        object_[sfDigest] = value;
        return *this;
    }

    /**
     * @brief Set sfData (SoeOptional)
     * @return Reference to this builder for method chaining.
     */
    MysteryCreateBuilder&
    setData(std::decay_t<typename SF_VL::type::value_type> const& value)
    {
        object_[sfData] = value;
        return *this;
    }

    /**
     * @brief Build and return the MysteryCreate wrapper.
     * @param publicKey The public key for signing.
     * @param secretKey The secret key for signing.
     * @return The constructed transaction wrapper.
     */
    MysteryCreate
    build(PublicKey const& publicKey, SecretKey const& secretKey)
    {
        sign(publicKey, secretKey);
        return MysteryCreate{std::make_shared<STTx>(std::move(object_))};
    }
};

}  // namespace xrpl::transactions
