#pragma once

#include <memory>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>

namespace d2df {

class ServiceRegistry {
public:
    template <typename T>
    void register_service(std::shared_ptr<T> service) {
        services_[std::type_index(typeid(T))] = std::move(service);
    }

    template <typename T>
    T& get() {
        const auto it = services_.find(std::type_index(typeid(T)));
        if (it == services_.end()) {
            throw std::runtime_error("ServiceRegistry: service not registered");
        }
        return *static_cast<T*>(it->second.get());
    }

    template <typename T>
    bool has() const {
        return services_.contains(std::type_index(typeid(T)));
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> services_;
};

} // namespace d2df
