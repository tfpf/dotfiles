#ifndef JSON_LOGGER_HH_
#define JSON_LOGGER_HH_

#include <cstdint>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using JSONKey = char const*;
using JSONValue = std::variant<int, long, long long, long long unsigned, long unsigned, unsigned, std::string_view>;

void log_debug(
    char const*, char const*, std::intmax_t, char const* msg, std::vector<std::pair<JSONKey, JSONValue>> = {}
);

#ifndef NDEBUG
#define LOG_DEBUG(...) log_debug(__FILE__, __func__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_DEBUG(...)
#endif

#endif
