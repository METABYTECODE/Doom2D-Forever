#pragma once

namespace d2df::engine::ecs {

struct Collider {
    float width = 0.0f;
    float height = 0.0f;
    bool solid = true;
};

struct MonsterTag {};

} // namespace d2df::engine::ecs
