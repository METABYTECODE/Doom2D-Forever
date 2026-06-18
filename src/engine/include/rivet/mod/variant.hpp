#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace rivet::mod {

/// Small dynamic value type for mod events and commands. Script bindings map onto this later.
using Variant = std::variant<std::monostate, bool, std::int64_t, double, std::string>;

[[nodiscard]] inline std::optional<bool> variant_as_bool(const Variant& value) {
    if (const auto* v = std::get_if<bool>(&value)) {
        return *v;
    }
    return std::nullopt;
}

[[nodiscard]] inline std::optional<std::int64_t> variant_as_int(const Variant& value) {
    if (const auto* v = std::get_if<std::int64_t>(&value)) {
        return *v;
    }
    if (const auto* v = std::get_if<double>(&value)) {
        return static_cast<std::int64_t>(*v);
    }
    return std::nullopt;
}

[[nodiscard]] inline std::optional<double> variant_as_number(const Variant& value) {
    if (const auto* v = std::get_if<double>(&value)) {
        return *v;
    }
    if (const auto* v = std::get_if<std::int64_t>(&value)) {
        return static_cast<double>(*v);
    }
    return std::nullopt;
}

[[nodiscard]] inline std::optional<std::string> variant_as_string(const Variant& value) {
    if (const auto* v = std::get_if<std::string>(&value)) {
        return *v;
    }
    return std::nullopt;
}

} // namespace rivet::mod
