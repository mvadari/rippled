//
// Created by Noah Kramer on 5/8/23.
//

#ifndef RIPPLED_STPLUGINTYPE_H
#define RIPPLED_STPLUGINTYPE_H

#include <ripple/basics/Buffer.h>
#include <ripple/basics/Slice.h>
#include <ripple/protocol/STBase.h>
#include <cassert>
#include <cstring>
#include <memory>

namespace ripple {

class STPluginType : public STBase
{
    Buffer value_;

public:
    using value_type = Slice;

    STPluginType() = default;
    STPluginType(STPluginType const& rhs);

    STPluginType(SField const& f, void const* data, std::size_t size);
    STPluginType(SField const& f, Buffer&& b);
    STPluginType(SField const& n);
    STPluginType(SerialIter&, SField const& name = sfGeneric);

    std::size_t
    size() const;

    std::uint8_t const*
    data() const;

    int
    getSType() const override;

    std::string
    getText() const override;

    void
    add(Serializer& s) const override;

    bool
    isEquivalent(const STBase& t) const override;

    bool
    isDefault() const override;

    STPluginType&
    operator=(Slice const& slice);

    value_type
    value() const noexcept;

    STPluginType&
    operator=(Buffer&& buffer);

    void
    setValue(Buffer&& b);

private:
    STBase*
    copy(std::size_t n, void* buf) const override;
    STBase*
    move(std::size_t n, void* buf) override;

    friend class detail::STVar;
};

inline STPluginType::STPluginType(STPluginType const& rhs)
        : STBase(rhs), value_(rhs.data(), rhs.size())
{
}

inline STPluginType::STPluginType(SField const& f, void const* data, std::size_t size)
        : STBase(f), value_(data, size)
{
}

inline STPluginType::STPluginType(SField const& f, Buffer&& b)
        : STBase(f), value_(std::move(b))
{
}

inline STPluginType::STPluginType(SField const& n) : STBase(n)
{
}

inline std::size_t
STPluginType::size() const
{
    return value_.size();
}

inline std::uint8_t const*
STPluginType::data() const
{
    return reinterpret_cast<std::uint8_t const*>(value_.data());
}

inline STPluginType&
STPluginType::operator=(Slice const& slice)
{
    value_ = Buffer(slice.data(), slice.size());
    return *this;
}

inline STPluginType::value_type
STPluginType::value() const noexcept
{
    return value_;
}

inline STPluginType&
STPluginType::operator=(Buffer&& buffer)
{
    value_ = std::move(buffer);
    return *this;
}

inline void
STPluginType::setValue(Buffer&& b)
{
    value_ = std::move(b);
}

} // namespace ripple
#endif //RIPPLED_STPLUGINTYPE_H
