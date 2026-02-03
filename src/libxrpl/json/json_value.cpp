#include <xrpl/beast/core/LexicalCast.h>
#include <xrpl/beast/utility/instrumentation.h>
#include <xrpl/json/detail/json_assert.h>
#include <xrpl/json/json_forwards.h>
#include <xrpl/json/json_value.h>
#include <xrpl/json/json_writer.h>

#include <boost/json.hpp>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>

namespace Json {

Value const Value::null;

// Static null reference for const operator[]
Value const&
Value::nullRef()
{
    return null;
}

// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// class Value::Value - Constructors
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////

Value::Value(ValueType type) : boost::json::value()
{
    switch (type)
    {
        case nullValue:
            // Default is null
            break;
        case intValue:
            *static_cast<boost::json::value*>(this) = std::int64_t{0};
            break;
        case uintValue:
            *static_cast<boost::json::value*>(this) = std::uint64_t{0};
            break;
        case realValue:
            *static_cast<boost::json::value*>(this) = 0.0;
            break;
        case stringValue:
            *static_cast<boost::json::value*>(this) = "";
            break;
        case arrayValue:
            *static_cast<boost::json::value*>(this) = boost::json::array{};
            break;
        case objectValue:
            *static_cast<boost::json::value*>(this) = boost::json::object{};
            break;
        case booleanValue:
            *static_cast<boost::json::value*>(this) = false;
            break;
    }
}

Value::Value(boost::json::value const& jv) : boost::json::value(jv)
{
}

Value::Value(boost::json::value&& jv) : boost::json::value(std::move(jv))
{
}

Value::Value(Int value) : boost::json::value(std::int64_t{value})
{
}

Value::Value(UInt value) : boost::json::value(std::uint64_t{value})
{
}

Value::Value(std::int64_t value) : boost::json::value(value)
{
}

Value::Value(std::uint64_t value) : boost::json::value(value)
{
}

Value::Value(double value) : boost::json::value(value)
{
}

Value::Value(char const* value) : boost::json::value(value ? value : "")
{
}

Value::Value(xrpl::Number const& value) : boost::json::value(to_string(value))
{
}

Value::Value(std::string const& value) : boost::json::value(value)
{
}

Value::Value(std::string_view value) : boost::json::value(value)
{
}

Value::Value(StaticString const& value) : boost::json::value(value.c_str() ? value.c_str() : "")
{
}

Value::Value(bool value) : boost::json::value(value)
{
}

Value::Value(std::nullptr_t) : boost::json::value(nullptr)
{
}

// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// class Value - Type checking
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////

ValueType
Value::type() const
{
    switch (kind())
    {
        case boost::json::kind::null:
            return nullValue;
        case boost::json::kind::bool_:
            return booleanValue;
        case boost::json::kind::int64:
            return intValue;
        case boost::json::kind::uint64:
            return uintValue;
        case boost::json::kind::double_:
            return realValue;
        case boost::json::kind::string:
            return stringValue;
        case boost::json::kind::array:
            return arrayValue;
        case boost::json::kind::object:
            return objectValue;
    }
    return nullValue;  // unreachable
}

static int
integerCmp(Int i, UInt ui)
{
    // All negative numbers are less than all unsigned numbers.
    if (i < 0)
        return -1;

    // Now we can safely compare.
    return (i < ui) ? -1 : (i == ui) ? 0 : 1;
}

bool
operator<(Value const& x, Value const& y)
{
    auto xType = x.type();
    auto yType = y.type();

    if (auto signum = xType - yType)
    {
        if (xType == intValue && yType == uintValue)
            signum = integerCmp(x.asInt(), y.asUInt());
        else if (xType == uintValue && yType == intValue)
            signum = -integerCmp(y.asInt(), x.asUInt());
        return signum < 0;
    }

    switch (xType)
    {
        case nullValue:
            return false;

        case intValue:
            return x.as_int64() < y.as_int64();

        case uintValue:
            return x.as_uint64() < y.as_uint64();

        case realValue:
            return x.as_double() < y.as_double();

        case booleanValue:
            return x.as_bool() < y.as_bool();

        case stringValue:
            return x.as_string() < y.as_string();

        case arrayValue: {
            auto const& xa = x.as_array();
            auto const& ya = y.as_array();
            if (xa.size() != ya.size())
                return xa.size() < ya.size();
            for (std::size_t i = 0; i < xa.size(); ++i)
            {
                Value const& xv = Value::asValue(xa[i]);
                Value const& yv = Value::asValue(ya[i]);
                if (xv < yv)
                    return true;
                if (yv < xv)
                    return false;
            }
            return false;
        }

        case objectValue: {
            auto const& xo = x.as_object();
            auto const& yo = y.as_object();
            if (xo.size() != yo.size())
                return xo.size() < yo.size();
            auto xit = xo.begin();
            auto yit = yo.begin();
            for (; xit != xo.end(); ++xit, ++yit)
            {
                if (xit->key() < yit->key())
                    return true;
                if (yit->key() < xit->key())
                    return false;
                Value const& xv = Value::asValue(xit->value());
                Value const& yv = Value::asValue(yit->value());
                if (xv < yv)
                    return true;
                if (yv < xv)
                    return false;
            }
            return false;
        }

        default:
            break;
    }

    return false;
}

bool
operator==(Value const& x, Value const& y)
{
    auto xType = x.type();
    auto yType = y.type();

    if (xType != yType)
    {
        if (xType == intValue && yType == uintValue)
            return !integerCmp(x.asInt(), y.asUInt());
        if (xType == uintValue && yType == intValue)
            return !integerCmp(y.asInt(), x.asUInt());
        return false;
    }

    // Use boost::json's built-in comparison
    return static_cast<boost::json::value const&>(x) == static_cast<boost::json::value const&>(y);
}

char const*
Value::asCString() const
{
    XRPL_ASSERT(type() == stringValue, "Json::Value::asCString : valid type");
    auto const& s = as_string();
    // Return nullptr for empty strings (legacy behavior)
    if (s.empty())
        return nullptr;
    return s.c_str();
}

std::string
Value::asString() const
{
    switch (type())
    {
        case nullValue:
            return "";

        case stringValue:
            return std::string(as_string());

        case booleanValue:
            return as_bool() ? "true" : "false";

        case intValue:
            return std::to_string(as_int64());

        case uintValue:
            return std::to_string(as_uint64());

        case realValue:
            return std::to_string(as_double());

        case arrayValue:
        case objectValue:
            JSON_ASSERT_MESSAGE(false, "Type is not convertible to string");

        default:
            break;
    }

    return "";
}

Value::Int
Value::asInt() const
{
    switch (type())
    {
        case nullValue:
            return 0;

        case intValue:
            return static_cast<Int>(as_int64());

        case uintValue: {
            auto val = as_uint64();
            JSON_ASSERT_MESSAGE(val < static_cast<std::uint64_t>(maxInt), "integer out of signed integer range");
            return static_cast<Int>(val);
        }

        case realValue: {
            auto val = as_double();
            JSON_ASSERT_MESSAGE(val >= minInt && val <= maxInt, "Real out of signed integer range");
            return static_cast<Int>(val);
        }

        case booleanValue:
            return as_bool() ? 1 : 0;

        case stringValue: {
            auto const& str = as_string();
            return beast::lexicalCastThrow<int>(std::string(str));
        }

        case arrayValue:
        case objectValue:
            JSON_ASSERT_MESSAGE(false, "Type is not convertible to int");

        default:
            break;
    }

    return 0;
}

UInt
Value::asAbsUInt() const
{
    switch (type())
    {
        case nullValue:
            return 0;

        case intValue: {
            auto val = as_int64();
            if (val < 0)
                return static_cast<UInt>(-val);
            return static_cast<UInt>(val);
        }

        case uintValue:
            return static_cast<UInt>(as_uint64());

        case realValue: {
            auto val = as_double();
            if (val < 0)
            {
                JSON_ASSERT_MESSAGE(-val <= maxUInt, "Real out of unsigned integer range");
                return static_cast<UInt>(-val);
            }
            JSON_ASSERT_MESSAGE(val <= maxUInt, "Real out of unsigned integer range");
            return static_cast<UInt>(val);
        }

        case booleanValue:
            return as_bool() ? 1 : 0;

        case stringValue: {
            auto const& str = as_string();
            auto const temp = beast::lexicalCastThrow<std::int64_t>(std::string(str));
            if (temp < 0)
            {
                JSON_ASSERT_MESSAGE(
                    -temp <= static_cast<std::int64_t>(maxUInt), "String out of unsigned integer range");
                return static_cast<UInt>(-temp);
            }
            JSON_ASSERT_MESSAGE(temp <= static_cast<std::int64_t>(maxUInt), "String out of unsigned integer range");
            return static_cast<UInt>(temp);
        }

        case arrayValue:
        case objectValue:
            JSON_ASSERT_MESSAGE(false, "Type is not convertible to int");

        default:
            break;
    }

    return 0;
}

Value::UInt
Value::asUInt() const
{
    switch (type())
    {
        case nullValue:
            return 0;

        case intValue: {
            auto val = as_int64();
            JSON_ASSERT_MESSAGE(val >= 0, "Negative integer can not be converted to unsigned integer");
            return static_cast<UInt>(val);
        }

        case uintValue:
            return static_cast<UInt>(as_uint64());

        case realValue: {
            auto val = as_double();
            JSON_ASSERT_MESSAGE(val >= 0 && val <= maxUInt, "Real out of unsigned integer range");
            return static_cast<UInt>(val);
        }

        case booleanValue:
            return as_bool() ? 1 : 0;

        case stringValue: {
            auto const& str = as_string();
            return beast::lexicalCastThrow<unsigned int>(std::string(str));
        }

        case arrayValue:
        case objectValue:
            JSON_ASSERT_MESSAGE(false, "Type is not convertible to uint");

        default:
            break;
    }

    return 0;
}

double
Value::asDouble() const
{
    switch (type())
    {
        case nullValue:
            return 0.0;

        case intValue:
            return static_cast<double>(as_int64());

        case uintValue:
            return static_cast<double>(as_uint64());

        case realValue:
            return as_double();

        case booleanValue:
            return as_bool() ? 1.0 : 0.0;

        case stringValue:
        case arrayValue:
        case objectValue:
            JSON_ASSERT_MESSAGE(false, "Type is not convertible to double");

        default:
            break;
    }

    return 0.0;
}

bool
Value::asBool() const
{
    switch (type())
    {
        case nullValue:
            return false;

        case intValue:
            return as_int64() != 0;

        case uintValue:
            return as_uint64() != 0;

        case realValue:
            return as_double() != 0.0;

        case booleanValue:
            return as_bool();

        case stringValue:
            return !as_string().empty();

        case arrayValue:
            return !as_array().empty();

        case objectValue:
            return !as_object().empty();

        default:
            break;
    }

    return false;
}

bool
Value::isConvertibleTo(ValueType other) const
{
    switch (type())
    {
        case nullValue:
            return true;

        case intValue: {
            auto val = as_int64();
            return (other == nullValue && val == 0) || other == intValue || (other == uintValue && val >= 0) ||
                other == realValue || other == stringValue || other == booleanValue;
        }

        case uintValue: {
            auto val = as_uint64();
            return (other == nullValue && val == 0) ||
                (other == intValue && val <= static_cast<std::uint64_t>(maxInt)) || other == uintValue ||
                other == realValue || other == stringValue || other == booleanValue;
        }

        case realValue: {
            auto val = as_double();
            return (other == nullValue && val == 0.0) || (other == intValue && val >= minInt && val <= maxInt) ||
                (other == uintValue && val >= 0 && val <= maxUInt &&
                 std::fabs(round(val) - val) < std::numeric_limits<double>::epsilon()) ||
                other == realValue || other == stringValue || other == booleanValue;
        }

        case booleanValue:
            return (other == nullValue && !as_bool()) || other == intValue || other == uintValue ||
                other == realValue || other == stringValue || other == booleanValue;

        case stringValue:
            return other == stringValue || (other == nullValue && as_string().empty());

        case arrayValue:
            return other == arrayValue || (other == nullValue && as_array().empty());

        case objectValue:
            return other == objectValue || (other == nullValue && as_object().empty());

        default:
            break;
    }

    return false;
}

/// Number of values in array or object
Value::UInt
Value::size() const
{
    switch (type())
    {
        case nullValue:
        case intValue:
        case uintValue:
        case realValue:
        case booleanValue:
        case stringValue:
            return 0;

        case arrayValue:
            return static_cast<UInt>(as_array().size());

        case objectValue:
            return static_cast<UInt>(as_object().size());

        default:
            break;
    }

    return 0;
}

Value::operator bool() const
{
    if (isNull())
        return false;

    if (isString())
        return !as_string().empty();

    return !(isArray() || isObject()) || size();
}

void
Value::clear()
{
    auto t = type();
    XRPL_ASSERT(t == nullValue || t == arrayValue || t == objectValue, "Json::Value::clear : valid type");

    switch (t)
    {
        case arrayValue:
            as_array().clear();
            break;

        case objectValue:
            as_object().clear();
            break;

        default:
            break;
    }
}

Value&
Value::operator[](UInt index)
{
    auto t = type();
    XRPL_ASSERT(t == nullValue || t == arrayValue, "Json::Value::operator[](UInt) : valid type");

    if (t == nullValue)
        *static_cast<boost::json::value*>(this) = boost::json::array{};

    auto& arr = as_array();

    // Expand array if needed
    while (arr.size() <= index)
        arr.push_back(nullptr);

    return asValue(arr[index]);
}

Value const&
Value::operator[](UInt index) const
{
    auto t = type();
    XRPL_ASSERT(t == nullValue || t == arrayValue, "Json::Value::operator[](UInt) const : valid type");

    if (t == nullValue)
        return null;

    auto const& arr = as_array();
    if (index >= arr.size())
        return null;

    return asValue(arr[index]);
}

Value&
Value::operator[](int index)
{
    return (*this)[static_cast<UInt>(index)];
}

Value const&
Value::operator[](int index) const
{
    return (*this)[static_cast<UInt>(index)];
}

Value&
Value::operator[](char const* key)
{
    return resolveReference(key);
}

Value&
Value::resolveReference(char const* key)
{
    auto t = type();
    XRPL_ASSERT(t == nullValue || t == objectValue, "Json::Value::resolveReference : valid type");

    if (t == nullValue)
        *static_cast<boost::json::value*>(this) = boost::json::object{};

    auto& obj = as_object();

    // Insert if not present
    if (!obj.contains(key))
        obj[key] = nullptr;

    return asValue(obj[key]);
}

Value
Value::get(UInt index, Value const& defaultValue) const
{
    auto t = type();
    if (t != arrayValue)
        return defaultValue;

    auto const& arr = as_array();
    if (index >= arr.size())
        return defaultValue;

    // If the element is null, return the default value (legacy behavior)
    auto const& elem = arr[index];
    if (elem.is_null())
        return defaultValue;

    return Value(elem);
}

bool
Value::isValidIndex(UInt index) const
{
    return index < size();
}

Value const&
Value::operator[](char const* key) const
{
    auto t = type();
    XRPL_ASSERT(t == nullValue || t == objectValue, "Json::Value::operator[](const char*) const : valid type");

    if (t == nullValue)
        return null;

    auto const& obj = as_object();
    auto it = obj.find(key);
    if (it == obj.end())
        return null;

    return asValue(it->value());
}

Value&
Value::operator[](std::string const& key)
{
    return (*this)[key.c_str()];
}

Value const&
Value::operator[](std::string const& key) const
{
    return (*this)[key.c_str()];
}

Value&
Value::operator[](StaticString const& key)
{
    return resolveReference(key.c_str());
}

Value const&
Value::operator[](StaticString const& key) const
{
    return (*this)[key.c_str()];
}

Value&
Value::operator[](std::string_view key)
{
    return resolveReference(std::string(key).c_str());
}

Value const&
Value::operator[](std::string_view key) const
{
    return (*this)[std::string(key).c_str()];
}

Value&
Value::append(Value const& value)
{
    auto t = type();
    if (t == nullValue)
        *static_cast<boost::json::value*>(this) = boost::json::array{};

    as_array().push_back(static_cast<boost::json::value const&>(value));
    return asValue(as_array().back());
}

Value&
Value::append(Value&& value)
{
    auto t = type();
    if (t == nullValue)
        *static_cast<boost::json::value*>(this) = boost::json::array{};

    as_array().push_back(static_cast<boost::json::value&&>(value));
    // Nullify the moved-from value (legacy behavior)
    static_cast<boost::json::value&>(value) = nullptr;
    return asValue(as_array().back());
}

Value
Value::get(char const* key, Value const& defaultValue) const
{
    auto t = type();
    if (t != objectValue)
        return defaultValue;

    auto const& obj = as_object();
    auto it = obj.find(key);
    if (it == obj.end())
        return defaultValue;

    return Value(it->value());
}

Value
Value::get(std::string const& key, Value const& defaultValue) const
{
    return get(key.c_str(), defaultValue);
}

Value
Value::removeMember(char const* key)
{
    auto t = type();
    XRPL_ASSERT(t == nullValue || t == objectValue, "Json::Value::removeMember : valid type");

    if (t == nullValue)
        return null;

    auto& obj = as_object();
    auto it = obj.find(key);
    if (it == obj.end())
        return null;

    Value old(it->value());
    obj.erase(it);
    return old;
}

Value
Value::removeMember(std::string const& key)
{
    return removeMember(key.c_str());
}

bool
Value::isMember(char const* key) const
{
    if (type() != objectValue)
        return false;

    return as_object().contains(key);
}

bool
Value::isMember(std::string const& key) const
{
    return isMember(key.c_str());
}

bool
Value::isMember(StaticString const& key) const
{
    return isMember(key.c_str());
}

Value::Members
Value::getMemberNames() const
{
    auto t = type();
    XRPL_ASSERT(t == nullValue || t == objectValue, "Json::Value::getMemberNames : valid type");

    if (t == nullValue)
        return Value::Members();

    auto const& obj = as_object();
    Members members;
    members.reserve(obj.size());

    for (auto const& kv : obj)
        members.push_back(std::string(kv.key()));

    return members;
}

std::string
Value::toStyledString() const
{
    StyledWriter writer;
    return writer.write(*this);
}

Value::const_iterator
Value::begin() const
{
    switch (type())
    {
        case arrayValue:
            return const_iterator(as_array().begin(), as_array().end(), 0);

        case objectValue:
            return const_iterator(as_object().begin(), as_object().end());

        default:
            break;
    }

    return const_iterator();
}

Value::const_iterator
Value::end() const
{
    switch (type())
    {
        case arrayValue:
            return const_iterator(as_array().end(), as_array().end(), static_cast<UInt>(as_array().size()));

        case objectValue:
            return const_iterator(as_object().end(), as_object().end());

        default:
            break;
    }

    return const_iterator();
}

Value::iterator
Value::begin()
{
    switch (type())
    {
        case arrayValue:
            return iterator(as_array().begin(), as_array().end(), 0);

        case objectValue:
            return iterator(as_object().begin(), as_object().end());

        default:
            break;
    }

    return iterator();
}

Value::iterator
Value::end()
{
    switch (type())
    {
        case arrayValue:
            return iterator(as_array().end(), as_array().end(), static_cast<UInt>(as_array().size()));

        case objectValue:
            return iterator(as_object().end(), as_object().end());

        default:
            break;
    }

    return iterator();
}

}  // namespace Json
