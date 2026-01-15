#ifndef XRPL_APP_MISC_FEEPARAMREGISTRY_H_INCLUDED
#define XRPL_APP_MISC_FEEPARAMREGISTRY_H_INCLUDED

#include <xrpld/core/Config.h>

#include <xrpl/protocol/Fees.h>
#include <xrpl/protocol/Rules.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/STObject.h>
#include <xrpl/protocol/XRPAmount.h>

#include <array>
#include <span>
#include <string_view>

namespace xrpl {

/**
 * Describes a single fee parameter that can be voted on via the consensus
 * process. Each descriptor contains the metadata needed to:
 * - Identify the parameter (name, human label)
 * - Map to on-ledger fields (legacy and XRPFees variants)
 * - Map to config keys
 * - Read/write values from FeeSetup and Fees structs
 */
struct FeeParamDescriptor
{
    // Stable identifier for this parameter
    std::string_view name;

    // Human-readable label for logging
    std::string_view humanLabel;

    // Legacy field ID (used when featureXRPFees is disabled)
    // For base_fee: sfBaseFee (UINT64)
    // For reserves: sfReserveBase, sfReserveIncrement (UINT32)
    SF_UINT64 const* legacyField64 = nullptr;
    SF_UINT32 const* legacyField32 = nullptr;

    // XRPFees field ID (used when featureXRPFees is enabled)
    SF_AMOUNT const* xrpFeesField;

    // Config key name (e.g., "reference_fee", "account_reserve")
    std::string_view configKey;

    // Function to get the target value from FeeSetup
    XRPAmount (*getFromSetup)(FeeSetup const&);

    // Function to get the current value from Fees
    XRPAmount (*getFromFees)(Fees const&);
};

// Number of fee parameters in the registry
inline constexpr std::size_t kFeeParamCount = 3;

/**
 * Returns the registry of all fee parameters.
 * The registry is a fixed-size array describing:
 * - base_fee (reference transaction cost)
 * - account_reserve (reserve base)
 * - owner_reserve (reserve increment)
 */
std::span<FeeParamDescriptor const, kFeeParamCount>
feeParamRegistry();

/**
 * Find a fee parameter descriptor by name.
 * Returns nullptr if not found.
 */
FeeParamDescriptor const*
findFeeParam(std::string_view name);

/**
 * Find a fee parameter descriptor by its XRPFees field.
 * Returns nullptr if not found.
 */
FeeParamDescriptor const*
findFeeParamByXRPField(SF_AMOUNT const& field);

}  // namespace xrpl

#endif
