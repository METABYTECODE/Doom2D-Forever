#include <rivet/core/event_bus.hpp>

namespace rivet::core {

void EventBus::clear() {
    std::lock_guard lock(mutex_);
    subscribers_.clear();
}

} // namespace rivet::core
