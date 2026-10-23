/**
 * @file json_helper.hpp
 * @brief Lightweight JSON string builder and field extraction helper.
 */

#ifndef AEGIS_COMMON_JSON_HELPER_HPP
#define AEGIS_COMMON_JSON_HELPER_HPP

#include <string>
#include <string_view>
#include <optional>

namespace aegis {
namespace common {

class JsonHelper {
public:
    static std::string EscapeString(std::string_view input);
    static std::string FormatField(std::string_view key, std::string_view value, bool isLast = false);
    static std::string FormatField(std::string_view key, int64_t value, bool isLast = false);
    static std::string FormatField(std::string_view key, double value, int precision = 4, bool isLast = false);
    static std::string FormatField(std::string_view key, bool value, bool isLast = false);

    static std::optional<std::string> ExtractString(std::string_view json, std::string_view key);
    static std::optional<int64_t> ExtractInt64(std::string_view json, std::string_view key);
    static std::optional<double> ExtractDouble(std::string_view json, std::string_view key);
    static std::optional<bool> ExtractBool(std::string_view json, std::string_view key);
};

} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_JSON_HELPER_HPP
