#include <d2df/tools/json_util.hpp>

namespace d2df::tools {

std::string json_escape(std::string_view text) {
    std::string out;
    out.reserve(text.size() + 8);
    for (char c : text) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                out += '?';
            } else {
                out.push_back(c);
            }
        }
    }
    return out;
}

std::optional<std::string> json_extract_string(std::string_view object, std::string_view key) {
    const std::string needle = std::string("\"") + std::string(key) + "\":\"";
    const auto start = object.find(needle);
    if (start == std::string_view::npos) {
        return std::nullopt;
    }

    std::string out;
    bool escape = false;
    for (std::size_t i = start + needle.size(); i < object.size(); ++i) {
        const char c = object[i];
        if (escape) {
            switch (c) {
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case 't':
                out.push_back('\t');
                break;
            case '\\':
            case '"':
                out.push_back(c);
                break;
            default:
                out.push_back(c);
                break;
            }
            escape = false;
            continue;
        }
        if (c == '\\') {
            escape = true;
            continue;
        }
        if (c == '"') {
            return out;
        }
        out.push_back(c);
    }
    return std::nullopt;
}

std::optional<std::uint64_t> json_extract_uint64(std::string_view object, std::string_view key) {
    const std::string needle = std::string("\"") + std::string(key) + "\":";
    const auto start = object.find(needle);
    if (start == std::string_view::npos) {
        return std::nullopt;
    }

    std::uint64_t value = 0;
    bool found = false;
    for (std::size_t i = start + needle.size(); i < object.size(); ++i) {
        const char c = object[i];
        if (c >= '0' && c <= '9') {
            value = value * 10 + static_cast<std::uint64_t>(c - '0');
            found = true;
            continue;
        }
        if (found) {
            return value;
        }
        if (c != ' ' && c != '\t') {
            break;
        }
    }
    return found ? std::optional<std::uint64_t>{value} : std::nullopt;
}

} // namespace d2df::tools
