#pragma once

#include <optional>
#include <string_view>

namespace d2df::engine::map {

struct EntityDimensions {
    float width = 0.0f;
    float height = 0.0f;
};

[[nodiscard]] std::optional<EntityDimensions> entity_dimensions(std::string_view type_id);

} // namespace d2df::engine::map
