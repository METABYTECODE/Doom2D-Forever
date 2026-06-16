#include <d2df/map/panel_types.hpp>

namespace d2df::map {

std::uint16_t panel_type_from_name(std::string_view name) {
    if (name == "PANEL_WALL") {
        return PANEL_WALL;
    }
    if (name == "PANEL_BACK") {
        return PANEL_BACK;
    }
    if (name == "PANEL_FORE") {
        return PANEL_FORE;
    }
    if (name == "PANEL_WATER") {
        return PANEL_WATER;
    }
    if (name == "PANEL_ACID1") {
        return PANEL_ACID1;
    }
    if (name == "PANEL_ACID2") {
        return PANEL_ACID2;
    }
    if (name == "PANEL_STEP") {
        return PANEL_STEP;
    }
    if (name == "PANEL_LIFTUP") {
        return PANEL_LIFTUP;
    }
    if (name == "PANEL_LIFTDOWN") {
        return PANEL_LIFTDOWN;
    }
    if (name == "PANEL_OPENDOOR") {
        return PANEL_OPENDOOR;
    }
    if (name == "PANEL_CLOSEDOOR") {
        return PANEL_CLOSEDOOR;
    }
    if (name == "PANEL_BLOCKMON") {
        return PANEL_BLOCKMON;
    }
    if (name == "PANEL_LIFTLEFT") {
        return PANEL_LIFTLEFT;
    }
    if (name == "PANEL_LIFTRIGHT") {
        return PANEL_LIFTRIGHT;
    }
    return 0;
}

} // namespace d2df::map
