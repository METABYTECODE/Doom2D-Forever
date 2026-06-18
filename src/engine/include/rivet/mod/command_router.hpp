#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <rivet/mod/command.hpp>

namespace rivet::mod {

/// Dispatches inbound mod commands to registered handlers.
class CommandRouter {
public:
    using Handler = std::function<CommandStatus(const Command&)>;

    void register_handler(std::string command_name, Handler handler);
    [[nodiscard]] CommandStatus execute(const Command& command);
    void enqueue(Command command);
    void flush_queue();
    void clear();

private:
    std::mutex mutex_;
    std::unordered_map<std::string, Handler> handlers_;
    std::vector<Command> queue_;
};

} // namespace rivet::mod
