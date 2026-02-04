#include <xrpl/basics/contract.h>
#include <xrpl/json/json_reader.h>
#include <xrpl/json/json_value.h>

#include <boost/json.hpp>

#include <istream>
#include <stdexcept>
#include <string>

namespace Json {
// Implementation of class Reader using boost::json
// ////////////////////////////////

// Helper to check nesting depth during parsing
static bool
checkNestingDepth(boost::json::value const& jv, unsigned depth, unsigned limit)
{
    if (depth > limit)
        return false;

    if (jv.is_array())
    {
        for (auto const& elem : jv.as_array())
        {
            if (!checkNestingDepth(elem, depth + 1, limit))
                return false;
        }
    }
    else if (jv.is_object())
    {
        for (auto const& kv : jv.as_object())
        {
            if (!checkNestingDepth(kv.value(), depth + 1, limit))
                return false;
        }
    }
    return true;
}

// Class Reader
// //////////////////////////////////////////////////////////////////

bool
Reader::parse(std::string const& document, Value& root)
{
    return parse(document.c_str(), document.c_str() + document.length(), root);
}

bool
Reader::parse(std::istream& sin, Value& root)
{
    std::string doc;
    std::getline(sin, doc, (char)EOF);
    return parse(doc, root);
}

bool
Reader::parse(char const* beginDoc, char const* endDoc, Value& root)
{
    document_ = std::string(beginDoc, endDoc);
    errors_.clear();

    boost::json::parse_options opts;
    opts.allow_comments = true;
    opts.max_depth = nest_limit;

    boost::system::error_code ec;
    boost::json::value jv = boost::json::parse(boost::json::string_view(beginDoc, endDoc - beginDoc), ec, {}, opts);

    if (ec)
    {
        ErrorInfo info;
        info.message_ = ec.message();
        info.token_.start_ = beginDoc;
        info.token_.end_ = endDoc;
        errors_.push_back(info);
        return false;
    }

    // Check that root is array or object
    if (!jv.is_null() && !jv.is_array() && !jv.is_object())
    {
        ErrorInfo info;
        info.message_ = "A valid JSON document must be either an array or an object value.";
        info.token_.start_ = beginDoc;
        info.token_.end_ = endDoc;
        errors_.push_back(info);
        return false;
    }

    // Check nesting depth
    if (!checkNestingDepth(jv, 0, nest_limit))
    {
        ErrorInfo info;
        info.message_ = "Syntax error: maximum nesting depth exceeded";
        info.token_.start_ = beginDoc;
        info.token_.end_ = endDoc;
        errors_.push_back(info);
        return false;
    }

    // Assign the parsed value to root
    root = Value(std::move(jv));
    return true;
}

void
Reader::getLocationLineAndColumn(Location location, int& line, int& column) const
{
    Location begin = document_.c_str();
    Location end = begin + document_.length();
    Location current = begin;
    Location lastLineStart = current;
    line = 0;

    while (current < location && current != end)
    {
        Char c = *current++;

        if (c == '\r')
        {
            if (*current == '\n')
                ++current;

            lastLineStart = current;
            ++line;
        }
        else if (c == '\n')
        {
            lastLineStart = current;
            ++line;
        }
    }

    // column & line start at 1
    column = int(location - lastLineStart) + 1;
    ++line;
}

std::string
Reader::getLocationLineAndColumn(Location location) const
{
    int line, column;
    getLocationLineAndColumn(location, line, column);
    return "Line " + std::to_string(line) + ", Column " + std::to_string(column);
}

std::string
Reader::getFormattedErrorMessages() const
{
    std::string formattedMessage;

    for (Errors::const_iterator itError = errors_.begin(); itError != errors_.end(); ++itError)
    {
        ErrorInfo const& error = *itError;
        formattedMessage += "* " + getLocationLineAndColumn(error.token_.start_) + "\n";
        formattedMessage += "  " + error.message_ + "\n";

        if (error.extra_)
            formattedMessage += "See " + getLocationLineAndColumn(error.extra_) + " for detail.\n";
    }

    return formattedMessage;
}

std::istream&
operator>>(std::istream& sin, Value& root)
{
    Json::Reader reader;
    bool ok = reader.parse(sin, root);

    // XRPL_ASSERT(ok, "Json::operator>>() : parse succeeded");
    if (!ok)
        xrpl::Throw<std::runtime_error>(reader.getFormattedErrorMessages());

    return sin;
}

}  // namespace Json
