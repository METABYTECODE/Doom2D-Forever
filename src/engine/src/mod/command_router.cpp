#include <rivet/mod/command_router.hpp>

namespace rivet::mod {

void CommandRouter::register_handler(std::string command_name, Handler handler) {
    std::scoped_lock lock(mutex_);
    handlers_[std::move(command_name)] = std::move(handler);
}

CommandStatus CommandRouter::execute(const Command& command) {
    Handler handler;
    {
        std::scoped_lock lock(mutex_);
        const auto it = handlers_.find(command.name);
        if (it == handlers_.end()) {
            return CommandStatus::Unknown;
        }
        handler = it->second;
    }

    return handler(command);
}

void CommandRouter::enqueue(Command command) {
    std::scoped_lock lock(mutex_);
    queue_.push_back(std::move(command));
}

void CommandRouter::flush_queue() {
    std::vector<Command> pending;
    {
        std::scoped_lock lock(mutex_);
        pending.swap(queue_);
    }

    for (const auto& command : pending) {
        (void)execute(command);
    }
}

void CommandRouter::clear() {
    std::scoped_lock lock(mutex_);
    handlers_.clear();
    queue_.clear();
}

} // namespace rivet::mod
