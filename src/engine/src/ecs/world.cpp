#include <rivet/ecs/world.hpp>

namespace rivet::ecs {

Entity World::create() {
    return registry_.create();
}

void World::destroy(Entity entity) {
    registry_.destroy(entity);
}

void World::clear() {
    registry_.clear();
}

} // namespace rivet::ecs
