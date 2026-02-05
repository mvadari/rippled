#include <xrpl/json/json_forwards.h>
#include <xrpl/json/json_value.h>
#include <xrpl/json/json_writer.h>

#include <boost/json.hpp>

#include <cstdio>
#include <ostream>
#include <sstream>
#include <string>

namespace Json {

// Helper functions for legacy API compatibility

std::string
valueToString(Int value)
{
    return std::to_string(value);
}

std::string
valueToString(UInt value)
{
    return std::to_string(value);
}

std::string
valueToString(double value)
{
    // Allocate a buffer that is more than large enough to store the 16 digits
    // of precision requested below.
    char buffer[32];
    // Print into the buffer. We need not request the alternative representation
    // that always has a decimal point because JSON doesn't distinguish the
    // concepts of reals and integers.
#if defined(_MSC_VER) && defined(__STDC_SECURE_LIB__)
    sprintf_s(buffer, sizeof(buffer), "%.16g", value);
#else
    snprintf(buffer, sizeof(buffer), "%.16g", value);
#endif
    return buffer;
}

std::string
valueToString(bool value)
{
    return value ? "true" : "false";
}

std::string
valueToQuotedString(char const* value)
{
    if (value == nullptr)
        return "\"\"";
    // Use boost::json::serialize on a string to get proper escaping
    boost::json::string str(value);
    return boost::json::serialize(str);
}

// Class FastWriter
// //////////////////////////////////////////////////////////////////

std::string
FastWriter::write(Value const& root)
{
    // Use boost::json::serialize for compact output
    return boost::json::serialize(root.toBoostJson());
}

void
FastWriter::writeValue(Value const& value)
{
    // This method is no longer used since we use boost::json::serialize
    // Kept for API compatibility
    document_ = boost::json::serialize(value.toBoostJson());
}

// Class StyledWriter
// //////////////////////////////////////////////////////////////////

// Helper function for pretty printing boost::json values
static void
prettyPrint(std::ostream& os, boost::json::value const& jv, std::string* indent, std::string const& indentStr)
{
    std::string indent_;
    if (!indent)
        indent = &indent_;

    switch (jv.kind())
    {
        case boost::json::kind::object: {
            auto const& obj = jv.get_object();
            if (obj.empty())
            {
                os << "{}";
            }
            else
            {
                os << "{";
                indent->append(indentStr);
                auto it = obj.begin();
                for (;;)
                {
                    os << "\n" << *indent;
                    os << boost::json::serialize(it->key()) << " : ";
                    prettyPrint(os, it->value(), indent, indentStr);
                    if (++it == obj.end())
                        break;
                    os << ",";
                }
                indent->resize(indent->size() - indentStr.size());
                os << "\n" << *indent << "}";
            }
            break;
        }

        case boost::json::kind::array: {
            auto const& arr = jv.get_array();
            if (arr.empty())
            {
                os << "[]";
            }
            else
            {
                os << "[";
                indent->append(indentStr);
                auto it = arr.begin();
                for (;;)
                {
                    os << "\n" << *indent;
                    prettyPrint(os, *it, indent, indentStr);
                    if (++it == arr.end())
                        break;
                    os << ",";
                }
                indent->resize(indent->size() - indentStr.size());
                os << "\n" << *indent << "]";
            }
            break;
        }

        case boost::json::kind::string:
            os << boost::json::serialize(jv.get_string());
            break;

        case boost::json::kind::uint64:
            os << jv.get_uint64();
            break;

        case boost::json::kind::int64:
            os << jv.get_int64();
            break;

        case boost::json::kind::double_:
            os << valueToString(jv.get_double());
            break;

        case boost::json::kind::bool_:
            os << (jv.get_bool() ? "true" : "false");
            break;

        case boost::json::kind::null:
            os << "null";
            break;
    }
}

StyledWriter::StyledWriter() : rightMargin_(74), indentSize_(3)
{
}

std::string
StyledWriter::write(Value const& root)
{
    std::ostringstream os;
    std::string indent;
    prettyPrint(os, root.toBoostJson(), &indent, "   ");
    os << "\n";
    return os.str();
}

void
StyledWriter::writeValue(Value const& value)
{
    // Not used in new implementation, kept for API compatibility
}

void
StyledWriter::writeArrayValue(Value const& value)
{
    // Not used in new implementation, kept for API compatibility
}

bool
StyledWriter::isMultilineArray(Value const& value)
{
    // Not used in new implementation, kept for API compatibility
    return true;
}

void
StyledWriter::pushValue(std::string const& value)
{
    // Not used in new implementation, kept for API compatibility
}

void
StyledWriter::writeIndent()
{
    // Not used in new implementation, kept for API compatibility
}

void
StyledWriter::writeWithIndent(std::string const& value)
{
    // Not used in new implementation, kept for API compatibility
}

void
StyledWriter::indent()
{
    // Not used in new implementation, kept for API compatibility
}

void
StyledWriter::unindent()
{
    // Not used in new implementation, kept for API compatibility
}

// Class StyledStreamWriter
// //////////////////////////////////////////////////////////////////

StyledStreamWriter::StyledStreamWriter(std::string indentation)
    : document_(nullptr), rightMargin_(74), indentation_(std::move(indentation))
{
}

void
StyledStreamWriter::write(std::ostream& out, Value const& root)
{
    std::string indent;
    prettyPrint(out, root.toBoostJson(), &indent, indentation_);
    out << "\n";
}

void
StyledStreamWriter::writeValue(Value const& value)
{
    // Not used in new implementation, kept for API compatibility
}

void
StyledStreamWriter::writeArrayValue(Value const& value)
{
    // Not used in new implementation, kept for API compatibility
}

bool
StyledStreamWriter::isMultilineArray(Value const& value)
{
    // Not used in new implementation, kept for API compatibility
    return true;
}

void
StyledStreamWriter::pushValue(std::string const& value)
{
    // Not used in new implementation, kept for API compatibility
}

void
StyledStreamWriter::writeIndent()
{
    // Not used in new implementation, kept for API compatibility
}

void
StyledStreamWriter::writeWithIndent(std::string const& value)
{
    // Not used in new implementation, kept for API compatibility
}

void
StyledStreamWriter::indent()
{
    // Not used in new implementation, kept for API compatibility
}

void
StyledStreamWriter::unindent()
{
    // Not used in new implementation, kept for API compatibility
}

std::ostream&
operator<<(std::ostream& sout, Value const& root)
{
    Json::StyledStreamWriter writer;
    writer.write(sout, root);
    return sout;
}

}  // namespace Json
