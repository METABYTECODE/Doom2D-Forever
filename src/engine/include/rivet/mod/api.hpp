#pragma once

#include <rivet/mod/command_router.hpp>
#include <rivet/mod/data_query.hpp>
#include <rivet/mod/event_hub.hpp>

namespace rivet::mod {

/// Facade for the early-stage mod API: events out, commands in, data hooks.
/// Engine provides transport only. Game code owns semantics and handlers.
class Api {
public:
    EventHub& events() { return events_; }
    [[nodiscard]] const EventHub& events() const { return events_; }

    CommandRouter& commands() { return commands_; }
    [[nodiscard]] const CommandRouter& commands() const { return commands_; }

    DataRegistry& data() { return data_; }
    [[nodiscard]] const DataRegistry& data() const { return data_; }

    void emit(const Event& event);
    [[nodiscard]] CommandStatus execute_command(const Command& command);
    void enqueue_command(Command command);
    void flush_commands();

    [[nodiscard]] std::optional<std::unordered_map<std::string, Variant>> query_data(const DataQuery& query) const;

private:
    EventHub events_;
    CommandRouter commands_;
    DataRegistry data_;
};

} // namespace rivet::mod
