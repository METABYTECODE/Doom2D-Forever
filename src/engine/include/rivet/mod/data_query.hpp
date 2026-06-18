#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include <rivet/mod/variant.hpp>

namespace rivet::mod {

/// Opaque read request. Engine forwards it; game code defines path semantics later.
struct DataQuery {
    std::string path;
    std::unordered_map<std::string, Variant> params;
};

using DataHandler = std::function<std::optional<std::unordered_map<std::string, Variant>>(const DataQuery&)>;

/// Hook registry for read-only data. Engine stores handlers only — never builds game state itself.
class DataRegistry {
public:
    void register_handler(std::string path, DataHandler handler);
    [[nodiscard]] std::optional<std::unordered_map<std::string, Variant>> query(const DataQuery& query) const;
    void clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, DataHandler> handlers_;
};

} // namespace rivet::mod
