#pragma once

#include <string>
#include <unordered_map>

#include <rivet/mod/variant.hpp>

namespace rivet::mod {

/// String-keyed event payload exposed to mods and future script runtimes.
struct Event {
    std::string name;
    std::unordered_map<std::string, Variant> data;
};

} // namespace rivet::mod
