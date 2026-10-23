#include "aegis/common/json_helper.hpp"
#include <sstream>
#include <iomanip>
#include <cctype>

namespace aegis {
namespace common {

std::string JsonHelper::EscapeString(std::string_view input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:   out += c; break;
        }
    }
    return out;
}

std::string JsonHelper::FormatField(std::string_view key, std::string_view value, bool isLast) {
    std::string out = "\"" + std::string(key) + "\":\"" + EscapeString(value) + "\"";
    if (!isLast) out += ",";
    return out;
}

std::string JsonHelper::FormatField(std::string_view key, int64_t value, bool isLast) {
    std::string out = "\"" + std::string(key) + "\":" + std::to_string(value);
    if (!isLast) out += ",";
    return out;
}

std::string JsonHelper::FormatField(std::string_view key, double value, int precision, bool isLast) {
    std::ostringstream oss;
    oss << "\"" << key << "\":" << std::fixed << std::setprecision(precision) << value;
    if (!isLast) oss << ",";
    return oss.str();
}

std::string JsonHelper::FormatField(std::string_view key, bool value, bool isLast) {
    std::string out = "\"" + std::string(key) + "\":" + (value ? "true" : "false");
    if (!isLast) out += ",";
    return out;
}

std::optional<std::string> JsonHelper::ExtractString(std::string_view json, std::string_view key) {
    std::string pattern = "\"" + std::string(key) + "\"";
    auto pos = json.find(pattern);
    if (pos == std::string_view::npos) return std::nullopt;

    pos = json.find(':', pos + pattern.size());
    if (pos == std::string_view::npos) return std::nullopt;

    auto startQuote = json.find('\"', pos + 1);
    if (startQuote == std::string_view::npos) return std::nullopt;

    auto endQuote = json.find('\"', startQuote + 1);
    if (endQuote == std::string_view::npos) return std::nullopt;

    return std::string(json.substr(startQuote + 1, endQuote - startQuote - 1));
}

std::optional<int64_t> JsonHelper::ExtractInt64(std::string_view json, std::string_view key) {
    std::string pattern = "\"" + std::string(key) + "\"";
    auto pos = json.find(pattern);
    if (pos == std::string_view::npos) return std::nullopt;

    pos = json.find(':', pos + pattern.size());
    if (pos == std::string_view::npos) return std::nullopt;

    size_t start = pos + 1;
    while (start < json.size() && (json[start] == ' ' || json[start] == '\t')) ++start;
    if (start >= json.size()) return std::nullopt;

    size_t end = start;
    while (end < json.size() && (std::isdigit(static_cast<unsigned char>(json[end])) || json[end] == '-')) ++end;

    if (start == end) return std::nullopt;
    try {
        return std::stoll(std::string(json.substr(start, end - start)));
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<double> JsonHelper::ExtractDouble(std::string_view json, std::string_view key) {
    std::string pattern = "\"" + std::string(key) + "\"";
    auto pos = json.find(pattern);
    if (pos == std::string_view::npos) return std::nullopt;

    pos = json.find(':', pos + pattern.size());
    if (pos == std::string_view::npos) return std::nullopt;

    size_t start = pos + 1;
    while (start < json.size() && (json[start] == ' ' || json[start] == '\t')) ++start;
    if (start >= json.size()) return std::nullopt;

    size_t end = start;
    while (end < json.size() && (std::isdigit(static_cast<unsigned char>(json[end])) || json[end] == '-' || json[end] == '.')) ++end;

    if (start == end) return std::nullopt;
    try {
        return std::stod(std::string(json.substr(start, end - start)));
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<bool> JsonHelper::ExtractBool(std::string_view json, std::string_view key) {
    std::string pattern = "\"" + std::string(key) + "\"";
    auto pos = json.find(pattern);
    if (pos == std::string_view::npos) return std::nullopt;

    pos = json.find(':', pos + pattern.size());
    if (pos == std::string_view::npos) return std::nullopt;

    if (json.find("true", pos) != std::string_view::npos) return true;
    if (json.find("false", pos) != std::string_view::npos) return false;
    return std::nullopt;
}

} // namespace common
} // namespace aegis
