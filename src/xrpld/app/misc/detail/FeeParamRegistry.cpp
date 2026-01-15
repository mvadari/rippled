#include <xrpld/app/misc/FeeParamRegistry.h>

#include <xrpl/beast/utility/instrumentation.h>
#include <xrpl/protocol/SField.h>

namespace xrpl {

namespace {

// Helper functions for accessing FeeSetup/Fees fields
XRPAmount
getBaseFeeFromSetup(FeeSetup const& s)
{
    return s.reference_fee;
}
XRPAmount
getAccountReserveFromSetup(FeeSetup const& s)
{
    return s.account_reserve;
}
XRPAmount
getOwnerReserveFromSetup(FeeSetup const& s)
{
    return s.owner_reserve;
}

XRPAmount
getBaseFeeFromFees(Fees const& f)
{
    return f.base;
}
XRPAmount
getAccountReserveFromFees(Fees const& f)
{
    return f.reserve;
}
XRPAmount
getOwnerReserveFromFees(Fees const& f)
{
    return f.increment;
}

// The registry of all fee parameters.
// Order: base_fee, account_reserve, owner_reserve
// Note: This cannot be constexpr because SFields are runtime-initialized.
std::array<FeeParamDescriptor, kFeeParamCount> const&
getRegistry()
{
    static std::array<FeeParamDescriptor, kFeeParamCount> const registry = {{
        {
            .name = "base_fee",
            .humanLabel = "base fee",
            .legacyField64 = &sfBaseFee,
            .legacyField32 = nullptr,
            .xrpFeesField = &sfBaseFeeDrops,
            .configKey = "reference_fee",
            .getFromSetup = getBaseFeeFromSetup,
            .getFromFees = getBaseFeeFromFees,
        },
        {
            .name = "account_reserve",
            .humanLabel = "base reserve",
            .legacyField64 = nullptr,
            .legacyField32 = &sfReserveBase,
            .xrpFeesField = &sfReserveBaseDrops,
            .configKey = "account_reserve",
            .getFromSetup = getAccountReserveFromSetup,
            .getFromFees = getAccountReserveFromFees,
        },
        {
            .name = "owner_reserve",
            .humanLabel = "reserve increment",
            .legacyField64 = nullptr,
            .legacyField32 = &sfReserveIncrement,
            .xrpFeesField = &sfReserveIncrementDrops,
            .configKey = "owner_reserve",
            .getFromSetup = getOwnerReserveFromSetup,
            .getFromFees = getOwnerReserveFromFees,
        },
    }};
    return registry;
}

// Runtime validation of registry consistency (called once at startup)
void
validateRegistry(std::array<FeeParamDescriptor, kFeeParamCount> const& registry)
{
    // Verify all descriptors have required fields
    for (auto const& d : registry)
    {
        XRPL_ASSERT(
            !d.name.empty(),
            "xrpl::FeeParamRegistry : descriptor name must not be empty");
        XRPL_ASSERT(
            !d.humanLabel.empty(),
            "xrpl::FeeParamRegistry : descriptor humanLabel must not be empty");
        XRPL_ASSERT(
            d.xrpFeesField != nullptr,
            "xrpl::FeeParamRegistry : xrpFeesField must not be null");
        XRPL_ASSERT(
            d.legacyField32 != nullptr || d.legacyField64 != nullptr,
            "xrpl::FeeParamRegistry : at least one legacy field must be set");
        XRPL_ASSERT(
            !d.configKey.empty(),
            "xrpl::FeeParamRegistry : configKey must not be empty");
        XRPL_ASSERT(
            d.getFromSetup != nullptr,
            "xrpl::FeeParamRegistry : getFromSetup must not be null");
        XRPL_ASSERT(
            d.getFromFees != nullptr,
            "xrpl::FeeParamRegistry : getFromFees must not be null");
    }

    // Verify all names are unique
    for (std::size_t i = 0; i < registry.size(); ++i)
    {
        for (std::size_t j = i + 1; j < registry.size(); ++j)
        {
            XRPL_ASSERT(
                registry[i].name != registry[j].name,
                "xrpl::FeeParamRegistry : descriptor names must be unique");
        }
    }
}

}  // namespace

std::span<FeeParamDescriptor const, kFeeParamCount>
feeParamRegistry()
{
    // Get registry and validate on first access
    auto const& registry = getRegistry();
    static bool const validated = [&registry]() {
        validateRegistry(registry);
        return true;
    }();
    (void)validated;  // Suppress unused variable warning
    return registry;
}

FeeParamDescriptor const*
findFeeParam(std::string_view name)
{
    for (auto const& d : feeParamRegistry())
    {
        if (d.name == name)
            return &d;
    }
    return nullptr;
}

FeeParamDescriptor const*
findFeeParamByXRPField(SF_AMOUNT const& field)
{
    for (auto const& d : feeParamRegistry())
    {
        if (d.xrpFeesField == &field)
            return &d;
    }
    return nullptr;
}

}  // namespace xrpl
