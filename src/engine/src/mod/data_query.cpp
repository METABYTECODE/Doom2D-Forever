#include <rivet/mod/data_query.hpp>

namespace rivet::mod {

void DataRegistry::register_handler(std::string path, DataHandler handler) {
    std::scoped_lock lock(mutex_);
    handlers_[std::move(path)] = std::move(handler);
}

std::optional<std::unordered_map<std::string, Variant>> DataRegistry::query(const DataQuery& query) const {
    DataHandler handler;
    {
        std::scoped_lock lock(mutex_);
        const auto it = handlers_.find(query.path);
        if (it == handlers_.end()) {
            return std::nullopt;
        }
        handler = it->second;
    }

    return handler(query);
}

void DataRegistry::clear() {
    std::scoped_lock lock(mutex_);
    handlers_.clear();
}

} // namespace rivet::mod
