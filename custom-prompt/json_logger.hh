#ifndef JSON_LOGGER_HH_
#define JSON_LOGGER_HH_

#include <cstdint>
#include <iostream>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

class JSONString
{
private:
    std::string_view sv;

public:
    JSONString(char const*);
    JSONString(std::string const&);
    JSONString(std::string_view const&);
    JSONString(JSONString const&);

    friend std::ostream& operator<<(std::ostream&, JSONString const&);
};

using JSONKey = JSONString;
using JSONValue = std::variant<int, long, long long, long long unsigned, long unsigned, unsigned, JSONString>;

class JSONLogger
{
public:
    void log_debug(
        char const*, char const*, std::uintmax_t, char const* msg, std::vector<std::pair<JSONKey, JSONValue>> = {}
    );
};

#ifndef NDEBUG
#define LOG_DEBUG(logger, ...) logger.log_debug(__FILE__, __func__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_DEBUG(...)
#endif

#endif
