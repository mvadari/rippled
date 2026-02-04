#ifndef XRPL_JSON_JSON_VALUE_H_INCLUDED
#define XRPL_JSON_JSON_VALUE_H_INCLUDED

#include <xrpl/basics/Number.h>
#include <xrpl/json/json_forwards.h>

#include <boost/json.hpp>

#include <cstring>
#include <limits>
#include <string>
#include <vector>

/** \brief JSON (JavaScript Object Notation).
 */
namespace Json {

/** \brief Type of the value held by a Value object.
 *  Kept for API compatibility - maps to boost::json::kind
 */
enum ValueType {
    nullValue = 0,  ///< 'null' value
    intValue,       ///< signed integer value
    uintValue,      ///< unsigned integer value
    realValue,      ///< double value
    stringValue,    ///< UTF-8 string value
    booleanValue,   ///< bool value
    arrayValue,     ///< array value (ordered list)
    objectValue     ///< object value (collection of name/value pairs).
};

/** \brief Lightweight wrapper to tag static string.
 *  Kept for API compatibility.
 */
class StaticString
{
public:
    constexpr explicit StaticString(char const* czString) : str_(czString)
    {
    }

    constexpr
    operator char const*() const
    {
        return str_;
    }

    constexpr char const*
    c_str() const
    {
        return str_;
    }

private:
    char const* str_;
};

inline bool
operator==(StaticString x, StaticString y)
{
    return strcmp(x.c_str(), y.c_str()) == 0;
}

inline bool
operator!=(StaticString x, StaticString y)
{
    return !(x == y);
}

inline bool
operator==(std::string const& x, StaticString y)
{
    return strcmp(x.c_str(), y.c_str()) == 0;
}

inline bool
operator!=(std::string const& x, StaticString y)
{
    return !(x == y);
}

inline bool
operator==(StaticString x, std::string const& y)
{
    return y == x;
}

inline bool
operator!=(StaticString x, std::string const& y)
{
    return !(y == x);
}

/** \brief Represents a <a HREF="http://www.json.org">JSON</a> value.
 *
 * This class inherits from boost::json::value and provides the legacy API
 * for compatibility while using boost::json internally.
 *
 * It can represent:
 * - signed integer (64-bit)
 * - unsigned integer (64-bit)
 * - double
 * - UTF-8 string
 * - boolean
 * - 'null'
 * - an ordered list of Value (array)
 * - collection of name/value pairs (object)
 *
 * The type of the held value is represented by a #ValueType and
 * can be obtained using type().
 *
 * @note This class inherits from boost::json::value and adds no data members,
 *       so it has the same memory layout. This allows safe casting between
 *       boost::json::value& and Json::Value&.
 */
class Value : public boost::json::value
{
public:
    using Members = std::vector<std::string>;
    using UInt = Json::UInt;
    using Int = Json::Int;
    using ArrayIndex = UInt;

    static Value const null;
    static constexpr Int minInt = std::numeric_limits<Int>::min();
    static constexpr Int maxInt = std::numeric_limits<Int>::max();
    static constexpr UInt maxUInt = std::numeric_limits<UInt>::max();

    // Forward iterator declarations
    class const_iterator;
    class iterator;

    //----------------------------------------------------------------------
    // Constructors
    //----------------------------------------------------------------------

    /** \brief Create a default Value of the given type. */
    Value(ValueType type = nullValue);

    /** \brief Construct from boost::json::value (implicit conversion) */
    Value(boost::json::value const& jv);
    Value(boost::json::value&& jv);

    Value(Int value);
    Value(UInt value);
    Value(int value);
    Value(unsigned int value);
    Value(short value);
    Value(unsigned short value);
    Value(double value);
    Value(char const* value);
    Value(xrpl::Number const& value);
    Value(StaticString const& value);
    Value(std::string const& value);
    Value(std::string_view value);
    Value(bool value);
    Value(std::nullptr_t);

    // Copy
    Value(Value const& other) = default;
    Value&
    operator=(Value const& other) = default;

    // Move (nullify source for legacy compatibility)
    Value(Value&& other) noexcept : boost::json::value(std::move(other))
    {
        // Nullify the moved-from object (legacy behavior)
        static_cast<boost::json::value&>(other) = nullptr;
    }
    Value&
    operator=(Value&& other) noexcept
    {
        if (this != &other)
        {
            boost::json::value::operator=(std::move(other));
            // Nullify the moved-from object (legacy behavior)
            static_cast<boost::json::value&>(other) = nullptr;
        }
        return *this;
    }

    ~Value() = default;

    //----------------------------------------------------------------------
    // Type checking (legacy API)
    //----------------------------------------------------------------------

    ValueType
    type() const;

    bool
    isNull() const
    {
        return is_null();
    }
    bool
    isBool() const
    {
        return is_bool();
    }
    bool
    isInt() const
    {
        return is_int64();
    }
    bool
    isUInt() const
    {
        return is_uint64();
    }
    bool
    isIntegral() const
    {
        return is_int64() || is_uint64() || is_bool();
    }
    bool
    isDouble() const
    {
        return is_double();
    }
    bool
    isNumeric() const
    {
        return is_int64() || is_uint64() || is_double() || is_bool();
    }
    bool
    isString() const
    {
        return is_string();
    }
    bool
    isArray() const
    {
        return is_array();
    }
    bool
    isArrayOrNull() const
    {
        return is_array() || is_null();
    }
    bool
    isObject() const
    {
        return is_object();
    }
    bool
    isObjectOrNull() const
    {
        return is_object() || is_null();
    }

    bool
    isConvertibleTo(ValueType other) const;

    //----------------------------------------------------------------------
    // Value accessors (legacy API)
    //----------------------------------------------------------------------

    char const*
    asCString() const;
    std::string
    asString() const;
    Int
    asInt() const;
    UInt
    asUInt() const;
    /** Return as 32-bit signed integer, throws if out of range */
    std::int32_t
    asInt32() const;
    /** Return as 32-bit unsigned integer, throws if out of range */
    std::uint32_t
    asUInt32() const;
    double
    asDouble() const;
    bool
    asBool() const;

    /** Return absolute value as unsigned */
    UInt
    asAbsUInt() const;

    //----------------------------------------------------------------------
    // Size and clear
    //----------------------------------------------------------------------

    UInt
    size() const;
    explicit
    operator bool() const;
    void
    clear();

    //----------------------------------------------------------------------
    // Array access
    //----------------------------------------------------------------------

    Value&
    operator[](UInt index);
    Value const&
    operator[](UInt index) const;
    Value&
    operator[](int index);
    Value const&
    operator[](int index) const;
    Value&
    operator[](unsigned int index)
    {
        return (*this)[static_cast<UInt>(index)];
    }
    Value const&
    operator[](unsigned int index) const
    {
        return (*this)[static_cast<UInt>(index)];
    }
    Value&
    operator[](std::size_t index)
    {
        return (*this)[static_cast<UInt>(index)];
    }
    Value const&
    operator[](std::size_t index) const
    {
        return (*this)[static_cast<UInt>(index)];
    }
    Value
    get(UInt index, Value const& defaultValue) const;
    bool
    isValidIndex(UInt index) const;
    Value&
    append(Value const& value);
    Value&
    append(Value&& value);

    //----------------------------------------------------------------------
    // Object access
    //----------------------------------------------------------------------

    Value&
    operator[](char const* key);
    Value const&
    operator[](char const* key) const;
    Value&
    operator[](std::string const& key);
    Value const&
    operator[](std::string const& key) const;
    Value&
    operator[](StaticString const& key);
    Value const&
    operator[](StaticString const& key) const;
    Value&
    operator[](std::string_view key);
    Value const&
    operator[](std::string_view key) const;

    Value
    get(char const* key, Value const& defaultValue) const;
    Value
    get(std::string const& key, Value const& defaultValue) const;

    Value
    removeMember(char const* key);
    Value
    removeMember(std::string const& key);

    bool
    isMember(char const* key) const;
    bool
    isMember(std::string const& key) const;
    bool
    isMember(StaticString const& key) const;

    Members
    getMemberNames() const;

    //----------------------------------------------------------------------
    // Output
    //----------------------------------------------------------------------

    std::string
    toStyledString() const;

    //----------------------------------------------------------------------
    // Iteration
    //----------------------------------------------------------------------

    const_iterator
    begin() const;
    const_iterator
    end() const;
    iterator
    begin();
    iterator
    end();

    //----------------------------------------------------------------------
    // Comparison
    //----------------------------------------------------------------------

    friend bool
    operator==(Value const& a, Value const& b);
    friend bool
    operator<(Value const& a, Value const& b);

private:
    // Helper to convert boost::json::value& to Value&
    static Value&
    asValue(boost::json::value& v)
    {
        return static_cast<Value&>(v);
    }
    static Value const&
    asValue(boost::json::value const& v)
    {
        return static_cast<Value const&>(v);
    }

    // Static null value for returning references
    static Value const&
    nullRef();

    // Resolve reference (for object key access)
    Value&
    resolveReference(char const* key);
};

inline Value
to_json(xrpl::Number const& number)
{
    return to_string(number);
}

inline bool
operator!=(Value const& x, Value const& y)
{
    return !(x == y);
}

inline bool
operator<=(Value const& x, Value const& y)
{
    return !(y < x);
}

inline bool
operator>(Value const& x, Value const& y)
{
    return y < x;
}

inline bool
operator>=(Value const& x, Value const& y)
{
    return !(x < y);
}

//==============================================================================
// Iterators
//==============================================================================

/** \brief const iterator for object and array value.
 *
 * For objects, iterates over key-value pairs.
 * For arrays, iterates over elements.
 */
class Value::const_iterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = Value;
    using difference_type = std::ptrdiff_t;
    using pointer = Value const*;
    using reference = Value const&;

    const_iterator() = default;

    // Construct from boost::json iterators
    explicit const_iterator(boost::json::object::const_iterator it, boost::json::object::const_iterator end);
    explicit const_iterator(
        boost::json::array::const_iterator it,
        boost::json::array::const_iterator end,
        std::size_t index);

    reference
    operator*() const;
    pointer
    operator->() const;

    const_iterator&
    operator++();
    const_iterator
    operator++(int);
    const_iterator&
    operator--();
    const_iterator
    operator--(int);

    bool
    operator==(const_iterator const& other) const;
    bool
    operator!=(const_iterator const& other) const;

    /// Return either the index or the member name of the referenced value
    Value
    key() const;

    /// Return the index of the referenced Value. -1 if it is not an array
    UInt
    index() const;

    /// Return the member name of the referenced Value. "" if not an object
    std::string
    memberName() const;

private:
    enum class IteratorType { None, Object, Array };

    IteratorType type_{IteratorType::None};
    boost::json::object::const_iterator objIt_;
    boost::json::object::const_iterator objEnd_;
    boost::json::array::const_iterator arrIt_;
    boost::json::array::const_iterator arrEnd_;
    std::size_t arrayIndex_{0};

    // Cached value for dereferencing
    mutable Value cachedValue_;
    mutable bool cacheValid_{false};

    void
    updateCache() const;
};

/** \brief Iterator for object and array value.
 */
class Value::iterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = Value;
    using difference_type = std::ptrdiff_t;
    using pointer = Value*;
    using reference = Value&;

    iterator() = default;

    // Construct from boost::json iterators
    explicit iterator(boost::json::object::iterator it, boost::json::object::iterator end);
    explicit iterator(boost::json::array::iterator it, boost::json::array::iterator end, std::size_t index);

    reference
    operator*() const;
    pointer
    operator->() const;

    iterator&
    operator++();
    iterator
    operator++(int);
    iterator&
    operator--();
    iterator
    operator--(int);

    bool
    operator==(iterator const& other) const;
    bool
    operator!=(iterator const& other) const;

    /// Return either the index or the member name of the referenced value
    Value
    key() const;

    /// Return the index of the referenced Value. -1 if it is not an array
    UInt
    index() const;

    /// Return the member name of the referenced Value. "" if not an object
    std::string
    memberName() const;

    // Convert to const_iterator
    operator const_iterator() const;

private:
    enum class IteratorType { None, Object, Array };

    IteratorType type_{IteratorType::None};
    boost::json::object::iterator objIt_;
    boost::json::object::iterator objEnd_;
    boost::json::array::iterator arrIt_;
    boost::json::array::iterator arrEnd_;
    std::size_t arrayIndex_{0};
};

// Legacy type aliases for backward compatibility
using ValueIterator = Value::iterator;
using ValueConstIterator = Value::const_iterator;

}  // namespace Json

#endif  // XRPL_JSON_JSON_VALUE_H_INCLUDED
