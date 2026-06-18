#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <rivet/mod/event.hpp>

namespace rivet::mod {

/// Mod-facing event bus. Uses string event names and variant payloads for future script binding.
class EventHub {
public:
    using Handler = std::function<void(const Event&)>;

    void subscribe(std::string event_name, Handler handler);
    void emit(const Event& event);
    void clear();

private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::vector<Handler>> handlers_;
};

} // namespace rivet::mod
