#include <d2df/map/trigger_types.hpp>

namespace d2df::map {

TriggerType trigger_type_from_name(std::string_view name) {
    if (name == "TRIGGER_EXIT") {
        return TriggerType::Exit;
    }
    if (name == "TRIGGER_TELEPORT") {
        return TriggerType::Teleport;
    }
    if (name == "TRIGGER_OPENDOOR") {
        return TriggerType::OpenDoor;
    }
    if (name == "TRIGGER_CLOSEDOOR") {
        return TriggerType::CloseDoor;
    }
    if (name == "TRIGGER_DOOR") {
        return TriggerType::Door;
    }
    if (name == "TRIGGER_DOOR5") {
        return TriggerType::Door5;
    }
    if (name == "TRIGGER_CLOSETRAP") {
        return TriggerType::CloseTrap;
    }
    if (name == "TRIGGER_TRAP") {
        return TriggerType::Trap;
    }
    if (name == "TRIGGER_PRESS") {
        return TriggerType::Press;
    }
    if (name == "TRIGGER_SECRET") {
        return TriggerType::Secret;
    }
    if (name == "TRIGGER_LIFTUP") {
        return TriggerType::LiftUp;
    }
    if (name == "TRIGGER_LIFTDOWN") {
        return TriggerType::LiftDown;
    }
    if (name == "TRIGGER_LIFT") {
        return TriggerType::Lift;
    }
    if (name == "TRIGGER_ON") {
        return TriggerType::On;
    }
    if (name == "TRIGGER_OFF") {
        return TriggerType::Off;
    }
    if (name == "TRIGGER_ONOFF") {
        return TriggerType::OnOff;
    }
    return TriggerType::None;
}

ActivateType activate_type_from_json(std::string_view snippet) {
    ActivateType flags = ActivateType::None;
    if (snippet.find("ACTIVATE_PLAYERCOLLIDE") != std::string_view::npos) {
        flags = static_cast<ActivateType>(static_cast<std::uint8_t>(flags) |
                                          static_cast<std::uint8_t>(ActivateType::PlayerCollide));
    }
    if (snippet.find("ACTIVATE_MONSTERCOLLIDE") != std::string_view::npos) {
        flags = static_cast<ActivateType>(static_cast<std::uint8_t>(flags) |
                                          static_cast<std::uint8_t>(ActivateType::MonsterCollide));
    }
    if (snippet.find("ACTIVATE_PLAYERPRESS") != std::string_view::npos) {
        flags = static_cast<ActivateType>(static_cast<std::uint8_t>(flags) |
                                          static_cast<std::uint8_t>(ActivateType::PlayerPress));
    }
    if (snippet.find("ACTIVATE_MONSTERPRESS") != std::string_view::npos) {
        flags = static_cast<ActivateType>(static_cast<std::uint8_t>(flags) |
                                          static_cast<std::uint8_t>(ActivateType::MonsterPress));
    }
    if (snippet.find("ACTIVATE_SHOT") != std::string_view::npos) {
        flags = static_cast<ActivateType>(static_cast<std::uint8_t>(flags) |
                                          static_cast<std::uint8_t>(ActivateType::Shot));
    }
    if (snippet.find("ACTIVATE_NOMONSTER") != std::string_view::npos) {
        flags = static_cast<ActivateType>(static_cast<std::uint8_t>(flags) |
                                          static_cast<std::uint8_t>(ActivateType::NoMonster));
    }
    if (snippet.find("ACTIVATE_NONE") != std::string_view::npos) {
        flags = ActivateType::None;
    }
    return flags;
}

bool has_activate_on_load(ActivateType flags) {
    return flags == ActivateType::None;
}

} // namespace d2df::map
