#pragma once

namespace d2df::ecs {

struct Transform {
    float x = 0.0f;
    float y = 0.0f;
    float prev_x = 0.0f;
    float prev_y = 0.0f;
};

struct Velocity {
    int x = 0;
    int y = 0;
};

struct PlayerTag {};

} // namespace d2df::ecs
