#include <cstdint>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "json_logger.hh"

#define LEFT_CURLY_BRACKET "\x7B"
#define RIGHT_CURLY_BRACKET "\x7D"

JSONString::JSONString(char const* s): sv(s){}
JSONString::JSONString(std::string const& s): sv(s){}
JSONString::JSONString(std::string_view const& s): sv(s){}
JSONString::JSONString(JSONString const& s): sv(s.sv){}

std::ostream& operator<<(std::ostream& ostream, JSONString json_string){
    ostream << '"';
    for(auto const& c: json_string.sv){
        if(std::isprint(c)){
            ostream << c;
            continue;
        }
        switch (c)
        {
        case '\t':
            ostream << "\\t";
        case '\n':
            ostream << "\\n";
        case '\v':
            ostream << "\\v";
        case '"':
            ostream << "\\\"";
        default:
            ostream << "\\x" << static_cast<int>(c);
        }
    }
    ostream << '"';
}

void log_debug(
    char const* file, char const* func, std::uintmax_t line, char const* msg,
    std::vector<std::pair<JSONKey, JSONValue>> msg_args
)
{
    std::ostringstream oss;
    std::time_t curr_time = std::time(nullptr);
    char curr_time_buf[32];
    std::strftime(curr_time_buf, sizeof curr_time_buf / sizeof *curr_time_buf, "%FT%T%z", std::localtime(&curr_time));
    oss << LEFT_CURLY_BRACKET "\"created\":\"" << curr_time_buf << "\"";
    oss << ",\"source\":" LEFT_CURLY_BRACKET "\"file\":\"" << file << "\",\"func\":\"" << func
        << "\",\"line\":" << line << RIGHT_CURLY_BRACKET;
    oss << ",\"msg\":\"" << msg << "\"";

    if (!msg_args.empty())
    {
        oss << ",\"msg_args\":" LEFT_CURLY_BRACKET;
        char const* delimiter = "";
        char const* actual_delimiter = ",";
        for (auto const& msg_arg : msg_args)
        {
            std::visit(
                [&](auto const& val)
                {
                    oss << delimiter << "\"" << msg_arg.first << "\":" << val;
                },
                msg_arg.second
            );
            delimiter = actual_delimiter;
        }
        oss << RIGHT_CURLY_BRACKET;
    }
    oss << RIGHT_CURLY_BRACKET "\n";
    std::clog << oss.str();
}
