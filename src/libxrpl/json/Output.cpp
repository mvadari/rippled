#include <xrpl/json/Output.h>
#include <xrpl/json/Writer.h>
#include <xrpl/json/json_value.h>

#include <boost/json.hpp>

#include <string>

namespace Json {

void
outputJson(Json::Value const& value, Output const& out)
{
    // Use boost::json::serialize for compact output, consistent with FastWriter
    std::string s = boost::json::serialize(static_cast<boost::json::value const&>(value));
    out(boost::beast::string_view(s.data(), s.size()));
}

std::string
jsonAsString(Json::Value const& value)
{
    // Use boost::json::serialize for compact output, consistent with FastWriter
    return boost::json::serialize(static_cast<boost::json::value const&>(value));
}

}  // namespace Json
