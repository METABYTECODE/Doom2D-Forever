#include <rivet/mod/api.hpp>

namespace rivet::mod {

void Api::emit(const Event& event) {
    events_.emit(event);
}

CommandStatus Api::execute_command(const Command& command) {
    return commands_.execute(command);
}

void Api::enqueue_command(Command command) {
    commands_.enqueue(std::move(command));
}

void Api::flush_commands() {
    commands_.flush_queue();
}

std::optional<std::unordered_map<std::string, Variant>> Api::query_data(const DataQuery& query) const {
    return data_.query(query);
}

} // namespace rivet::mod
