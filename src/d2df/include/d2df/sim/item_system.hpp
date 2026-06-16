#pragma once

#include <d2df/map/item_types.hpp>
#include <d2df/map/map_document.hpp>
#include <d2df/sim/player_state.hpp>

#include <vector>

namespace d2df {

class EventBus;

} // namespace d2df

namespace d2df::sim {

struct WorldItem {
    map::ItemType type = map::ItemType::None;
    float x = 0.0f;
    float y = 0.0f;
    float width = 16.0f;
    float height = 16.0f;
    bool active = true;
};

class ItemSystem {
public:
    void reset(const map::MapDocument& map, bool single_player = true);

    void tick(PlayerState& player, EventBus* events);

    [[nodiscard]] const std::vector<WorldItem>& items() const { return items_; }

private:
    std::vector<WorldItem> items_;
};

} // namespace d2df::sim
