#pragma once

namespace rivet::ecs::components {

/// Contact sides from the last PhysicsWorld step.
struct PhysicsContacts {
    bool floor = false;
    bool ceiling = false;
    bool wall_left = false;
    bool wall_right = false;
};

} // namespace rivet::ecs::components
