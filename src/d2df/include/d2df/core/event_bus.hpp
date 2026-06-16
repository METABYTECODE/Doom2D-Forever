#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace d2df {

/// Type-safe publish/subscribe bus for decoupled game systems.
/// ReplicationSystem (Phase 9) will subscribe here without touching gameplay code.
class EventBus {
public:
    using SubscriptionId = std::size_t;

    template <typename Event>
    SubscriptionId subscribe(std::function<void(const Event&)> handler) {
        std::lock_guard lock(mutex_);

        const auto type = std::type_index(typeid(Event));
        const auto id = next_id_++;

        auto wrapper = [handler = std::move(handler)](const void* raw) {
            handler(*static_cast<const Event*>(raw));
        };

        subscribers_[type].push_back({id, std::move(wrapper)});
        return id;
    }

    template <typename Event>
    void publish(const Event& event) {
        std::vector<std::function<void(const void*)>> handlers;
        {
            std::lock_guard lock(mutex_);
            const auto it = subscribers_.find(std::type_index(typeid(Event)));
            if (it == subscribers_.end()) {
                return;
            }
            handlers.reserve(it->second.size());
            for (const auto& entry : it->second) {
                handlers.push_back(entry.handler);
            }
        }

        for (const auto& handler : handlers) {
            handler(&event);
        }
    }

    void clear() {
        std::lock_guard lock(mutex_);
        subscribers_.clear();
    }

private:
    struct Entry {
        SubscriptionId id;
        std::function<void(const void*)> handler;
    };

    std::mutex mutex_;
    SubscriptionId next_id_ = 1;
    std::unordered_map<std::type_index, std::vector<Entry>> subscribers_;
};

} // namespace d2df
