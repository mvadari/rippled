// Implementation of Json::Value iterator classes

#include <xrpl/json/json_value.h>

#include <boost/json.hpp>

namespace Json {

// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// class Value::const_iterator
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////

Value::const_iterator::const_iterator(boost::json::object::const_iterator it, boost::json::object::const_iterator end)
    : type_(IteratorType::Object), objIt_(it), objEnd_(end)
{
}

Value::const_iterator::const_iterator(
    boost::json::array::const_iterator it,
    boost::json::array::const_iterator end,
    std::size_t index)
    : type_(IteratorType::Array), arrIt_(it), arrEnd_(end), arrayIndex_(index)
{
}

void
Value::const_iterator::updateCache() const
{
    if (cacheValid_)
        return;

    if (type_ == IteratorType::Object)
    {
        cachedValue_ = Value(objIt_->value());
    }
    else if (type_ == IteratorType::Array)
    {
        cachedValue_ = Value(*arrIt_);
    }
    cacheValid_ = true;
}

Value::const_iterator::reference
Value::const_iterator::operator*() const
{
    updateCache();
    return cachedValue_;
}

Value::const_iterator::pointer
Value::const_iterator::operator->() const
{
    updateCache();
    return &cachedValue_;
}

Value::const_iterator&
Value::const_iterator::operator++()
{
    cacheValid_ = false;
    if (type_ == IteratorType::Object)
        ++objIt_;
    else if (type_ == IteratorType::Array)
    {
        ++arrIt_;
        ++arrayIndex_;
    }
    return *this;
}

Value::const_iterator
Value::const_iterator::operator++(int)
{
    const_iterator tmp(*this);
    ++(*this);
    return tmp;
}

Value::const_iterator&
Value::const_iterator::operator--()
{
    cacheValid_ = false;
    if (type_ == IteratorType::Object)
        --objIt_;
    else if (type_ == IteratorType::Array)
    {
        --arrIt_;
        --arrayIndex_;
    }
    return *this;
}

Value::const_iterator
Value::const_iterator::operator--(int)
{
    const_iterator tmp(*this);
    --(*this);
    return tmp;
}

bool
Value::const_iterator::operator==(const_iterator const& other) const
{
    if (type_ != other.type_)
        return false;
    if (type_ == IteratorType::None)
        return true;
    if (type_ == IteratorType::Object)
        return objIt_ == other.objIt_;
    return arrIt_ == other.arrIt_;
}

bool
Value::const_iterator::operator!=(const_iterator const& other) const
{
    return !(*this == other);
}

Value
Value::const_iterator::key() const
{
    if (type_ == IteratorType::Object)
        return Value(std::string(objIt_->key()));
    return Value(static_cast<UInt>(arrayIndex_));
}

UInt
Value::const_iterator::index() const
{
    if (type_ == IteratorType::Array)
        return static_cast<UInt>(arrayIndex_);
    return static_cast<UInt>(-1);
}

std::string
Value::const_iterator::memberName() const
{
    if (type_ == IteratorType::Object)
        return std::string(objIt_->key());
    return "";
}

// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// class Value::iterator
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////

Value::iterator::iterator(boost::json::object::iterator it, boost::json::object::iterator end)
    : type_(IteratorType::Object), objIt_(it), objEnd_(end)
{
}

Value::iterator::iterator(boost::json::array::iterator it, boost::json::array::iterator end, std::size_t index)
    : type_(IteratorType::Array), arrIt_(it), arrEnd_(end), arrayIndex_(index)
{
}

Value::iterator::reference
Value::iterator::operator*() const
{
    if (type_ == IteratorType::Object)
        return static_cast<Value&>(objIt_->value());
    return static_cast<Value&>(*arrIt_);
}

Value::iterator::pointer
Value::iterator::operator->() const
{
    if (type_ == IteratorType::Object)
        return static_cast<Value*>(&objIt_->value());
    return static_cast<Value*>(&*arrIt_);
}

Value::iterator&
Value::iterator::operator++()
{
    if (type_ == IteratorType::Object)
        ++objIt_;
    else if (type_ == IteratorType::Array)
    {
        ++arrIt_;
        ++arrayIndex_;
    }
    return *this;
}

Value::iterator
Value::iterator::operator++(int)
{
    iterator tmp(*this);
    ++(*this);
    return tmp;
}

Value::iterator&
Value::iterator::operator--()
{
    if (type_ == IteratorType::Object)
        --objIt_;
    else if (type_ == IteratorType::Array)
    {
        --arrIt_;
        --arrayIndex_;
    }
    return *this;
}

Value::iterator
Value::iterator::operator--(int)
{
    iterator tmp(*this);
    --(*this);
    return tmp;
}

bool
Value::iterator::operator==(iterator const& other) const
{
    if (type_ != other.type_)
        return false;
    if (type_ == IteratorType::None)
        return true;
    if (type_ == IteratorType::Object)
        return objIt_ == other.objIt_;
    return arrIt_ == other.arrIt_;
}

bool
Value::iterator::operator!=(iterator const& other) const
{
    return !(*this == other);
}

Value
Value::iterator::key() const
{
    if (type_ == IteratorType::Object)
        return Value(std::string(objIt_->key()));
    return Value(static_cast<UInt>(arrayIndex_));
}

UInt
Value::iterator::index() const
{
    if (type_ == IteratorType::Array)
        return static_cast<UInt>(arrayIndex_);
    return static_cast<UInt>(-1);
}

std::string
Value::iterator::memberName() const
{
    if (type_ == IteratorType::Object)
        return std::string(objIt_->key());
    return "";
}

Value::iterator::operator const_iterator() const
{
    if (type_ == IteratorType::Object)
    {
        // Need to convert non-const iterator to const iterator
        // boost::json::object::const_iterator can be constructed from iterator
        return const_iterator(objIt_, objEnd_);
    }
    else if (type_ == IteratorType::Array)
    {
        return const_iterator(arrIt_, arrEnd_, arrayIndex_);
    }
    return const_iterator();
}

}  // namespace Json
