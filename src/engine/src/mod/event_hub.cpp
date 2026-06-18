#include <rivet/mod/event_hub.hpp>

namespace rivet::mod {

void EventHub::subscribe(std::string event_name, Handler handler) {
    std::scoped_lock lock(mutex_);
    handlers_[std::move(event_name)].push_back(std::move(handler));
}

void EventHub::emit(const Event& event) {
    std::vector<Handler> handlers;
    {
        std::scoped_lock lock(mutex_);
        if (const auto it = handlers_.find(event.name); it != handlers_.end()) {
            handlers = it->second;
        }
    }

    for (const auto& handler : handlers) {
        handler(event);
    }
}

void EventHub::clear() {
    std::scoped_lock lock(mutex_);
    handlers_.clear();
}

} // namespace rivet::mod
