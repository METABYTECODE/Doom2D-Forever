#include <catch2/catch_test_macros.hpp>

#include <d2df/core/event_bus.hpp>

namespace {

struct TestEvent {
    int value = 0;
};

} // namespace

TEST_CASE("EventBus delivers subscribed events", "[core]") {
    d2df::EventBus bus;
    int received = 0;

    bus.subscribe<TestEvent>([&](const TestEvent& e) { received = e.value; });
    bus.publish(TestEvent{42});

    REQUIRE(received == 42);
}

TEST_CASE("EventBus isolates event types", "[core]") {
    d2df::EventBus bus;
    int count_a = 0;
    int count_b = 0;

    bus.subscribe<TestEvent>([&](const TestEvent&) { ++count_a; });

    struct OtherEvent {};
    bus.subscribe<OtherEvent>([&](const OtherEvent&) { ++count_b; });

    bus.publish(TestEvent{1});
    bus.publish(OtherEvent{});

    REQUIRE(count_a == 1);
    REQUIRE(count_b == 1);
}
