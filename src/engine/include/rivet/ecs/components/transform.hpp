#pragma once

namespace rivet::ecs::components {

struct Transform {
    float x = 0.0f;
    float y = 0.0f;
    float rotation = 0.0f;
    float scale_x = 1.0f;
    float scale_y = 1.0f;
};

struct Velocity {
    float x = 0.0f;
    float y = 0.0f;
};

struct Name {
    const char* value = "entity";
};

} // namespace rivet::ecs::components
