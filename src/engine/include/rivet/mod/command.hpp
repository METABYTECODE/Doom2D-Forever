#pragma once

#include <string>
#include <unordered_map>

#include <rivet/mod/variant.hpp>

namespace rivet::mod {

/// Inbound mod/game command. Handlers are registered by name on the command router.
struct Command {
    std::string name;
    std::unordered_map<std::string, Variant> args;
};

enum class CommandStatus {
    Ok,
    Unknown,
    InvalidArgs,
    Rejected,
};

} // namespace rivet::mod
