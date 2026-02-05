// Implementation of Json::Value iterator classes

#include <xrpl/json/json_value.h>

namespace Json {

// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// class Value::const_iterator
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////

Value::const_iterator::const_iterator(ObjectType::const_iterator it, ObjectType::const_iterator end)
    : type_(IteratorType::Object), objIt_(it), objEnd_(end)
{
}

Value::const_iterator::const_iterator(ArrayType::const_iterator it, ArrayType::const_iterator end, std::size_t index)
    : type_(IteratorType::Array), arrIt_(it), arrEnd_(end), arrayIndex_(index)
{
}

Value::const_iterator::reference
Value::const_iterator::operator*() const
{
    if (type_ == IteratorType::Object)
        return objIt_->second;
    return *arrIt_;
}

Value::const_iterator::pointer
Value::const_iterator::operator->() const
{
    if (type_ == IteratorType::Object)
        return &objIt_->second;
    return &*arrIt_;
}

Value::const_iterator&
Value::const_iterator::operator++()
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
        return Value(objIt_->first);
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
        return objIt_->first;
    return "";
}

// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// class Value::iterator
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////

Value::iterator::iterator(ObjectType::iterator it, ObjectType::iterator end)
    : type_(IteratorType::Object), objIt_(it), objEnd_(end)
{
}

Value::iterator::iterator(ArrayType::iterator it, ArrayType::iterator end, std::size_t index)
    : type_(IteratorType::Array), arrIt_(it), arrEnd_(end), arrayIndex_(index)
{
}

Value::iterator::reference
Value::iterator::operator*() const
{
    if (type_ == IteratorType::Object)
        return objIt_->second;
    return *arrIt_;
}

Value::iterator::pointer
Value::iterator::operator->() const
{
    if (type_ == IteratorType::Object)
        return &objIt_->second;
    return &*arrIt_;
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
        return Value(objIt_->first);
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
        return objIt_->first;
    return "";
}

Value::iterator::operator const_iterator() const
{
    if (type_ == IteratorType::Object)
    {
        return const_iterator(objIt_, objEnd_);
    }
    else if (type_ == IteratorType::Array)
    {
        return const_iterator(arrIt_, arrEnd_, arrayIndex_);
    }
    return const_iterator();
}

}  // namespace Json
