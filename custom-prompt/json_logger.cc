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

void log_debug(
    char const* file, char const* func, std::intmax_t line, char const* msg,
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
        char const* actual_delimiter = ", ";
        for (auto const& msg_arg : msg_args)
        {
            std::intmax_t const* intval;
            std::string_view const* svval;
            if ((intval = std::get_if<std::intmax_t>(&msg_arg.second)) != nullptr)
            {
                oss << delimiter << "\"" << msg_arg.first << "\":" << *intval;
            }
            else if ((svval = std::get_if<std::string_view>(&msg_arg.second)) != nullptr)
            {
                oss << delimiter << "\"" << msg_arg.first << "\":\"" << *svval << "\"";
            }
            delimiter = actual_delimiter;
        }
        oss << RIGHT_CURLY_BRACKET;
    }
    oss << RIGHT_CURLY_BRACKET "\n";
    std::clog << oss.str();
}
