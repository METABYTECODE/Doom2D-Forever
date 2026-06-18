#include <catch2/catch_test_macros.hpp>

#include <rivet/core/application.hpp>
#include <rivet/core/game_context.hpp>
#include <rivet/ecs/components/collider.hpp>
#include <rivet/ecs/components/transform.hpp>
#include <rivet/mod/api.hpp>
#include <rivet/mod/command.hpp>
#include <rivet/mod/event.hpp>
#include <rivet/physics/aabb.hpp>
#include <rivet/physics/physics_world.hpp>
#include <rivet/render/camera2d.hpp>
#include <rivet/render/renderer_backend.hpp>
#include <rivet/render/renderer_factory.hpp>
#include <rivet/scene/scene.hpp>

namespace {

class CounterScene final : public rivet::scene::Scene {
public:
    CounterScene()
        : Scene("counter") {}

    [[nodiscard]] int ticks() const { return ticks_; }

protected:
    void on_fixed_update(float fixed_delta_time) override {
        (void)fixed_delta_time;
        ++ticks_;
    }

private:
    int ticks_ = 0;
};

} // namespace

TEST_CASE("Application runs fixed updates through active scene", "[engine]") {
    class TestApp final : public rivet::core::Application {
    public:
        void bootstrap(std::unique_ptr<CounterScene> scene) {
            scenes().push(std::move(scene));
        }
        [[nodiscard]] CounterScene* counter_scene() {
            return static_cast<CounterScene*>(scenes().active());
        }
    };

    TestApp app;
    app.bootstrap(std::make_unique<CounterScene>());

    for (int i = 0; i < 5; ++i) {
        app.run_one_frame(1.0f / 60.0f);
    }

    REQUIRE(app.counter_scene() != nullptr);
    CHECK(app.counter_scene()->ticks() == 5);
}

TEST_CASE("SceneManager push pop maintains single active scene", "[engine]") {
    rivet::scene::SceneManager manager;
    manager.push(std::make_unique<rivet::scene::Scene>("a"));
    CHECK(manager.count() == 1);
    CHECK(manager.active()->name() == "a");
    CHECK(manager.active()->is_active());

    manager.push(std::make_unique<rivet::scene::Scene>("b"));
    CHECK(manager.count() == 2);
    CHECK(manager.active()->name() == "b");

    manager.pop();
    CHECK(manager.count() == 1);
    CHECK(manager.active()->name() == "a");
}

TEST_CASE("ECS world creates entities with components", "[engine]") {
    rivet::ecs::World world;
    const auto entity = world.create();
    world.registry().emplace<rivet::ecs::components::Transform>(
        entity, rivet::ecs::components::Transform{.x = 10.0f, .y = 20.0f});

    CHECK(world.registry().all_of<rivet::ecs::components::Transform>(entity));
    CHECK(world.registry().get<rivet::ecs::components::Transform>(entity).x == 10.0f);
}

TEST_CASE("Mod API emits events and routes commands", "[engine][mod]") {
    class ModApp final : public rivet::core::Application {
    public:
        void bootstrap() {
            scenes().push(std::make_unique<rivet::scene::Scene>("level"));
        }
    };

    ModApp app;
    app.bootstrap();

    int tick_count = 0;
    app.mod_api().events().subscribe("OnTick", [&](const rivet::mod::Event& event) {
        ++tick_count;
        CHECK(event.name == "OnTick");
        CHECK(rivet::mod::variant_as_number(event.data.at("dt")).has_value());
    });

    bool command_ran = false;
    app.mod_api().commands().register_handler("play_sound", [&](const rivet::mod::Command& cmd) {
        command_ran = true;
        CHECK(rivet::mod::variant_as_string(cmd.args.at("id")) == "door_open");
        return rivet::mod::CommandStatus::Ok;
    });

    app.mod_api().enqueue_command(rivet::mod::Command{
        .name = "play_sound",
        .args = {{"id", std::string("door_open")}},
    });

    app.run_one_frame(1.0f / 60.0f);

    CHECK(tick_count == 1);
    CHECK(command_ran);
}

TEST_CASE("Mod data hook forwards opaque queries to registered handlers", "[engine][mod]") {
    rivet::mod::Api api;
    api.data().register_handler("runtime", [](const rivet::mod::DataQuery& query) {
        if (query.path != "runtime") {
            return std::optional<std::unordered_map<std::string, rivet::mod::Variant>>{};
        }
        return std::optional(std::unordered_map<std::string, rivet::mod::Variant>{
            {{"frame", std::int64_t{42}}},
        });
    });

    const auto result = api.query_data({.path = "runtime"});
    REQUIRE(result.has_value());
    CHECK(rivet::mod::variant_as_int(result->at("frame")) == 42);
    CHECK_FALSE(api.query_data({.path = "unknown"}).has_value());
}

TEST_CASE("Camera2D maps world space into screen space", "[engine][render]") {
    rivet::render::Camera2D camera;
    camera.set_viewport(800, 600);
    camera.center_on(100.0f, 50.0f);

    const auto screen = camera.world_to_screen({.x = 100.0f, .y = 50.0f, .width = 32.0f, .height = 32.0f});
    CHECK(screen.x == 400.0f);
    CHECK(screen.y == 300.0f);
    CHECK(screen.width == 32.0f);
}

TEST_CASE("AABB overlap helpers detect intersections", "[engine][physics]") {
    const rivet::physics::Aabb a{.x = 0.0f, .y = 0.0f, .width = 32.0f, .height = 32.0f};
    const rivet::physics::Aabb b{.x = 16.0f, .y = 16.0f, .width = 32.0f, .height = 32.0f};
    const rivet::physics::Aabb c{.x = 64.0f, .y = 64.0f, .width = 16.0f, .height = 16.0f};

    CHECK(a.intersects(b));
    CHECK_FALSE(a.intersects(c));
}

TEST_CASE("PhysicsWorld resolves static collisions", "[engine][physics]") {
    using namespace rivet::ecs::components;

    rivet::ecs::World world;
    rivet::physics::PhysicsWorld physics;

    const auto dynamic = world.create();
    world.registry().emplace<Transform>(dynamic, Transform{.x = 10.0f, .y = 0.0f});
    world.registry().emplace<Velocity>(dynamic, Velocity{.x = 100.0f, .y = 0.0f});
    world.registry().emplace<Collider>(dynamic, Collider{.width = 16.0f, .height = 16.0f});

    const auto wall = world.create();
    world.registry().emplace<Transform>(wall, Transform{.x = 20.0f, .y = 0.0f});
    world.registry().emplace<Collider>(wall, Collider{.width = 16.0f, .height = 16.0f, .is_static = true});

    physics.step(world, 0.2f);

    const auto& transform = world.registry().get<Transform>(dynamic);
    const auto& wall_transform = world.registry().get<Transform>(wall);
    const auto dynamic_box = rivet::physics::make_aabb(transform, world.registry().get<Collider>(dynamic));
    const auto wall_box =
        rivet::physics::make_aabb(wall_transform, world.registry().get<Collider>(wall));
    CHECK_FALSE(dynamic_box.intersects(wall_box));
}

TEST_CASE("Application delegates lifecycle hooks to attached game", "[engine][game]") {
    class ProbeGame final : public rivet::core::Game {
    public:
        void on_attach(rivet::core::GameContext& context) override {
            context.scenes().push(std::make_unique<CounterScene>());
            attached = true;
        }

        void on_update(rivet::core::GameContext& context, float delta_time) override {
            (void)context;
            (void)delta_time;
            ++updates;
        }

        void on_fixed_update(rivet::core::GameContext& context, float fixed_delta_time) override {
            (void)context;
            (void)fixed_delta_time;
            ++fixed_updates;
        }

        void on_render(rivet::core::GameContext& context, float interpolation_alpha) override {
            (void)context;
            (void)interpolation_alpha;
            ++renders;
        }

        bool attached = false;
        int updates = 0;
        int fixed_updates = 0;
        int renders = 0;
    };

    rivet::core::Application app;
    ProbeGame game;
    app.attach_game(game);

    app.run_one_frame(1.0f / 60.0f);

    CHECK(game.attached);
    CHECK(game.updates == 1);
    CHECK(game.fixed_updates == 1);
    CHECK(game.renders == 1);
    REQUIRE(app.scenes().active() != nullptr);
    CHECK(app.scenes().active()->name() == "counter");
}

TEST_CASE("GameContext exposes stable engine services", "[engine][api]") {
    rivet::core::Application app;
    rivet::core::GameContext context(app);

    CHECK(context.is_running());
    context.request_quit();
    CHECK_FALSE(context.is_running());
}

TEST_CASE("Renderer factory exposes backend availability", "[engine][render]") {
    using rivet::render::RendererBackend;

    CHECK(rivet::render::is_renderer_backend_available(RendererBackend::Sdl));
    CHECK_FALSE(rivet::render::is_renderer_backend_available(RendererBackend::OpenGl));
    CHECK(rivet::render::parse_renderer_backend("sdl") == RendererBackend::Sdl);
    CHECK(rivet::render::parse_renderer_backend("opengl") == RendererBackend::OpenGl);
    CHECK(rivet::render::to_string(RendererBackend::OpenGl) == "opengl");
}

TEST_CASE("Renderer factory rejects unavailable OpenGL backend", "[engine][render]") {
    CHECK_THROWS_AS(
        rivet::render::create_renderer(rivet::render::RendererBackend::OpenGl, {}),
        std::runtime_error);
}
