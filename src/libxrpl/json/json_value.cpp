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

// Helper to convert boost::json::value to Json::Value (deep conversion)
static Value
fromBoostJson(boost::json::value const& jv)
{
    switch (jv.kind())
    {
        case boost::json::kind::null:
            return Value(nullValue);
        case boost::json::kind::bool_:
            return Value(jv.as_bool());
        case boost::json::kind::int64:
            return Value(static_cast<Value::Int>(jv.as_int64()));
        case boost::json::kind::uint64:
            return Value(static_cast<Value::UInt>(jv.as_uint64()));
        case boost::json::kind::double_:
            return Value(jv.as_double());
        case boost::json::kind::string:
            return Value(std::string(jv.as_string()));
        case boost::json::kind::array: {
            Value result(arrayValue);
            for (auto const& elem : jv.as_array())
                result.append(fromBoostJson(elem));
            return result;
        }
        case boost::json::kind::object: {
            Value result(objectValue);
            for (auto const& kv : jv.as_object())
                result[std::string(kv.key())] = fromBoostJson(kv.value());
            return result;
        }
    }
    return Value();  // unreachable
}

// //////////////////////////////////////////////////////////////////
// class Value::Value - Constructors
// //////////////////////////////////////////////////////////////////

Value::Value(ValueType type) : data_(nullptr)
{
    switch (type)
    {
        case nullValue:
            data_ = nullptr;
            break;
        case intValue:
            data_ = Int{0};
            break;
        case uintValue:
            data_ = UInt{0};
            break;
        case realValue:
            data_ = 0.0;
            break;
        case stringValue:
            data_ = std::string{};
            break;
        case booleanValue:
            data_ = false;
            break;
        case arrayValue:
            data_ = ArrayType{};
            break;
        case objectValue:
            data_ = ObjectType{};
            break;
    }
}

Value::Value(boost::json::value const& jv) : Value(fromBoostJson(jv))
{
}

Value::Value(boost::json::value&& jv) : Value(fromBoostJson(jv))
{
}

Value::Value(Int value) : data_(value)
{
}

Value::Value(UInt value) : data_(value)
{
}

Value::Value(int value) : data_(static_cast<Int>(value))
{
}

Value::Value(unsigned int value) : data_(static_cast<UInt>(value))
{
}

Value::Value(short value) : data_(static_cast<Int>(value))
{
}

Value::Value(unsigned short value) : data_(static_cast<UInt>(value))
{
}

Value::Value(double value) : data_(value)
{
}

Value::Value(char const* value) : data_(std::string(value ? value : ""))
{
}

Value::Value(xrpl::Number const& value) : data_(to_string(value))
{
}

Value::Value(std::string const& value) : data_(value)
{
}

Value::Value(std::string_view value) : data_(std::string(value))
{
}

Value::Value(StaticString const& value) : data_(std::string(value.c_str() ? value.c_str() : ""))
{
}

Value::Value(bool value) : data_(value)
{
}

Value::Value(std::nullptr_t) : data_(nullptr)
{
}

// Move constructor
Value::Value(Value&& other) noexcept : data_(std::move(other.data_))
{
    // Nullify the moved-from object (legacy behavior)
    other.data_ = nullptr;
}

// Move assignment
Value&
Value::operator=(Value&& other) noexcept
{
    if (this != &other)
    {
        data_ = std::move(other.data_);
        // Nullify the moved-from object (legacy behavior)
        other.data_ = nullptr;
    }
    return *this;
}

// //////////////////////////////////////////////////////////////////
// class Value - Type checking
// //////////////////////////////////////////////////////////////////

ValueType
Value::type() const
{
    return static_cast<ValueType>(data_.index());
}

// Helper to get Int from storage
Int
getInt(Value const& v)
{
    return std::get<Int>(v.data_);
}

// Helper to get UInt from storage
UInt
getUInt(Value const& v)
{
    return std::get<UInt>(v.data_);
}

// Helper to get double from storage
double
getDouble(Value const& v)
{
    return std::get<double>(v.data_);
}

// Helper to get bool from storage
bool
getBool(Value const& v)
{
    return std::get<bool>(v.data_);
}

// Helper to get string from storage
std::string const&
getString(Value const& v)
{
    return std::get<std::string>(v.data_);
}

// Helper to get array from storage
Value::ArrayType const&
getArray(Value const& v)
{
    return std::get<Value::ArrayType>(v.data_);
}

// Helper to get object from storage
Value::ObjectType const&
getObject(Value const& v)
{
    return std::get<Value::ObjectType>(v.data_);
}

static int
integerCmp(Int i, UInt ui)
{
    // All negative numbers are less than all unsigned numbers.
    if (i < 0)
        return -1;

    // Now we can safely compare.
    return (static_cast<UInt>(i) < ui) ? -1 : (static_cast<UInt>(i) == ui) ? 0 : 1;
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
            return getInt(x) < getInt(y);

        case uintValue:
            return getUInt(x) < getUInt(y);

        case realValue:
            return getDouble(x) < getDouble(y);

        case booleanValue:
            return getBool(x) < getBool(y);

        case stringValue:
            return getString(x) < getString(y);

        case arrayValue: {
            auto const& xa = getArray(x);
            auto const& ya = getArray(y);
            if (xa.size() != ya.size())
                return xa.size() < ya.size();
            for (std::size_t i = 0; i < xa.size(); ++i)
            {
                if (xa[i] < ya[i])
                    return true;
                if (ya[i] < xa[i])
                    return false;
            }
            return false;
        }

        case objectValue: {
            auto const& xo = getObject(x);
            auto const& yo = getObject(y);
            if (xo.size() != yo.size())
                return xo.size() < yo.size();
            auto xit = xo.begin();
            auto yit = yo.begin();
            for (; xit != xo.end(); ++xit, ++yit)
            {
                if (xit->first < yit->first)
                    return true;
                if (yit->first < xit->first)
                    return false;
                if (xit->second < yit->second)
                    return true;
                if (yit->second < xit->second)
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

    // Compare based on type
    return x.data_ == y.data_;
}

char const*
Value::asCString() const
{
    XRPL_ASSERT(type() == stringValue, "Json::Value::asCString : valid type");
    auto const& s = getString(*this);
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
            return getString(*this);

        case booleanValue:
            return getBool(*this) ? "true" : "false";

        case intValue:
            return std::to_string(getInt(*this));

        case uintValue:
            return std::to_string(getUInt(*this));

        case realValue:
            return std::to_string(getDouble(*this));

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
            return getInt(*this);

        case uintValue: {
            auto val = getUInt(*this);
            JSON_ASSERT_MESSAGE(val <= static_cast<UInt>(maxInt), "integer out of signed integer range");
            return static_cast<Int>(val);
        }

        case realValue: {
            auto val = getDouble(*this);
            JSON_ASSERT_MESSAGE(val >= minInt && val <= maxInt, "Real out of signed integer range");
            return static_cast<Int>(val);
        }

        case booleanValue:
            return getBool(*this) ? 1 : 0;

        case stringValue: {
            auto const& str = getString(*this);
            return beast::lexicalCastThrow<std::int64_t>(str);
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
            auto val = getInt(*this);
            if (val < 0)
                return static_cast<UInt>(-val);
            return static_cast<UInt>(val);
        }

        case uintValue:
            return getUInt(*this);

        case realValue: {
            auto val = getDouble(*this);
            if (val < 0)
            {
                JSON_ASSERT_MESSAGE(-val <= maxUInt, "Real out of unsigned integer range");
                return static_cast<UInt>(-val);
            }
            JSON_ASSERT_MESSAGE(val <= maxUInt, "Real out of unsigned integer range");
            return static_cast<UInt>(val);
        }

        case booleanValue:
            return getBool(*this) ? 1 : 0;

        case stringValue: {
            auto const& str = getString(*this);
            auto const temp = beast::lexicalCastThrow<std::int64_t>(str);
            if (temp < 0)
            {
                // With 64-bit UInt, any negated int64 value fits
                return static_cast<UInt>(-temp);
            }
            // Positive int64 always fits in uint64
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
            auto val = getInt(*this);
            JSON_ASSERT_MESSAGE(val >= 0, "Negative integer can not be converted to unsigned integer");
            return static_cast<UInt>(val);
        }

        case uintValue:
            return getUInt(*this);

        case realValue: {
            auto val = getDouble(*this);
            JSON_ASSERT_MESSAGE(val >= 0 && val <= maxUInt, "Real out of unsigned integer range");
            return static_cast<UInt>(val);
        }

        case booleanValue:
            return getBool(*this) ? 1 : 0;

        case stringValue: {
            auto const& str = getString(*this);
            return beast::lexicalCastThrow<std::uint64_t>(str);
        }

        case arrayValue:
        case objectValue:
            JSON_ASSERT_MESSAGE(false, "Type is not convertible to uint");

        default:
            break;
    }

    return 0;
}

std::int32_t
Value::asInt32() const
{
    auto val = asInt();
    JSON_ASSERT_MESSAGE(
        val >= std::numeric_limits<std::int32_t>::min() && val <= std::numeric_limits<std::int32_t>::max(),
        "Value out of 32-bit signed integer range");
    return static_cast<std::int32_t>(val);
}

std::uint32_t
Value::asUInt32() const
{
    auto val = asUInt();
    JSON_ASSERT_MESSAGE(val <= std::numeric_limits<std::uint32_t>::max(), "Value out of 32-bit unsigned integer range");
    return static_cast<std::uint32_t>(val);
}

double
Value::asDouble() const
{
    switch (type())
    {
        case nullValue:
            return 0.0;

        case intValue:
            return static_cast<double>(getInt(*this));

        case uintValue:
            return static_cast<double>(getUInt(*this));

        case realValue:
            return getDouble(*this);

        case booleanValue:
            return getBool(*this) ? 1.0 : 0.0;

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
            return getInt(*this) != 0;

        case uintValue:
            return getUInt(*this) != 0;

        case realValue:
            return getDouble(*this) != 0.0;

        case booleanValue:
            return getBool(*this);

        case stringValue:
            return !getString(*this).empty();

        case arrayValue:
            return !getArray(*this).empty();

        case objectValue:
            return !getObject(*this).empty();

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
            auto val = getInt(*this);
            return (other == nullValue && val == 0) || other == intValue || (other == uintValue && val >= 0) ||
                other == realValue || other == stringValue || other == booleanValue;
        }

        case uintValue: {
            auto val = getUInt(*this);
            return (other == nullValue && val == 0) || (other == intValue && val <= static_cast<UInt>(maxInt)) ||
                other == uintValue || other == realValue || other == stringValue || other == booleanValue;
        }

        case realValue: {
            auto val = getDouble(*this);
            return (other == nullValue && val == 0.0) || (other == intValue && val >= minInt && val <= maxInt) ||
                (other == uintValue && val >= 0 && val <= maxUInt &&
                 std::fabs(round(val) - val) < std::numeric_limits<double>::epsilon()) ||
                other == realValue || other == stringValue || other == booleanValue;
        }

        case booleanValue:
            return (other == nullValue && !getBool(*this)) || other == intValue || other == uintValue ||
                other == realValue || other == stringValue || other == booleanValue;

        case stringValue:
            return other == stringValue || (other == nullValue && getString(*this).empty());

        case arrayValue:
            return other == arrayValue || (other == nullValue && getArray(*this).empty());

        case objectValue:
            return other == objectValue || (other == nullValue && getObject(*this).empty());

        default:
            break;
    }

    return false;
}

// Helper to get mutable array from storage
Value::ArrayType&
getMutableArray(Value& v)
{
    return std::get<Value::ArrayType>(v.data_);
}

// Helper to get mutable object from storage
Value::ObjectType&
getMutableObject(Value& v)
{
    return std::get<Value::ObjectType>(v.data_);
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
            return static_cast<UInt>(getArray(*this).size());

        case objectValue:
            return static_cast<UInt>(getObject(*this).size());

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
        return !getString(*this).empty();

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
            getMutableArray(*this).clear();
            break;

        case objectValue:
            getMutableObject(*this).clear();
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
        data_ = ArrayType{};

    auto& arr = getMutableArray(*this);

    // Expand array if needed
    while (arr.size() <= index)
        arr.push_back(Value());

    return arr[index];
}

Value const&
Value::operator[](UInt index) const
{
    auto t = type();
    XRPL_ASSERT(t == nullValue || t == arrayValue, "Json::Value::operator[](UInt) const : valid type");

    if (t == nullValue)
        return null;

    auto const& arr = getArray(*this);
    if (index >= arr.size())
        return null;

    return arr[index];
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
        data_ = ObjectType{};

    auto& obj = getMutableObject(*this);

    // Insert if not present (use transparent lookup)
    auto it = obj.find(key);
    if (it == obj.end())
        obj[key] = Value();

    return obj[key];
}

Value
Value::get(UInt index, Value const& defaultValue) const
{
    auto t = type();
    if (t != arrayValue)
        return defaultValue;

    auto const& arr = getArray(*this);
    if (index >= arr.size())
        return defaultValue;

    // If the element is null, return the default value (legacy behavior)
    auto const& elem = arr[index];
    if (elem.isNull())
        return defaultValue;

    return elem;
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

    auto const& obj = getObject(*this);
    auto it = obj.find(key);
    if (it == obj.end())
        return null;

    return it->second;
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
        data_ = ArrayType{};

    auto& arr = getMutableArray(*this);
    arr.push_back(value);
    return arr.back();
}

Value&
Value::append(Value&& value)
{
    auto t = type();
    if (t == nullValue)
        data_ = ArrayType{};

    auto& arr = getMutableArray(*this);
    arr.push_back(std::move(value));
    // Nullify the moved-from value (legacy behavior) - already handled by move
    // constructor
    return arr.back();
}

Value
Value::get(char const* key, Value const& defaultValue) const
{
    auto t = type();
    if (t != objectValue)
        return defaultValue;

    auto const& obj = getObject(*this);
    auto it = obj.find(key);
    if (it == obj.end())
        return defaultValue;

    return it->second;
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

    auto& obj = getMutableObject(*this);
    auto it = obj.find(key);
    if (it == obj.end())
        return null;

    Value old(std::move(it->second));
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

    return getObject(*this).contains(key);
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

    auto const& obj = getObject(*this);
    Members members;
    members.reserve(obj.size());

    for (auto const& kv : obj)
        members.push_back(kv.first);

    return members;
}

std::string
Value::toStyledString() const
{
    StyledWriter writer;
    return writer.write(*this);
}

// Convert Json::Value to boost::json::value for serialization
boost::json::value
Value::toBoostJson() const
{
    switch (type())
    {
        case nullValue:
            return nullptr;
        case intValue:
            return getInt(*this);
        case uintValue:
            return getUInt(*this);
        case realValue:
            return getDouble(*this);
        case booleanValue:
            return getBool(*this);
        case stringValue:
            return boost::json::string(getString(*this));
        case arrayValue: {
            boost::json::array arr;
            for (auto const& elem : getArray(*this))
                arr.push_back(elem.toBoostJson());
            return arr;
        }
        case objectValue: {
            boost::json::object obj;
            for (auto const& kv : getObject(*this))
                obj[kv.first] = kv.second.toBoostJson();
            return obj;
        }
    }
    return nullptr;  // unreachable
}

Value::const_iterator
Value::begin() const
{
    switch (type())
    {
        case arrayValue: {
            auto const& arr = getArray(*this);
            return const_iterator(arr.begin(), arr.end(), 0);
        }

        case objectValue: {
            auto const& obj = getObject(*this);
            return const_iterator(obj.begin(), obj.end());
        }

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
        case arrayValue: {
            auto const& arr = getArray(*this);
            return const_iterator(arr.end(), arr.end(), static_cast<UInt>(arr.size()));
        }

        case objectValue: {
            auto const& obj = getObject(*this);
            return const_iterator(obj.end(), obj.end());
        }

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
        case arrayValue: {
            auto& arr = getMutableArray(*this);
            return iterator(arr.begin(), arr.end(), 0);
        }

        case objectValue: {
            auto& obj = getMutableObject(*this);
            return iterator(obj.begin(), obj.end());
        }

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
        case arrayValue: {
            auto& arr = getMutableArray(*this);
            return iterator(arr.end(), arr.end(), static_cast<UInt>(arr.size()));
        }

        case objectValue: {
            auto& obj = getMutableObject(*this);
            return iterator(obj.end(), obj.end());
        }

        default:
            break;
    }

    return iterator();
}

}  // namespace Json
