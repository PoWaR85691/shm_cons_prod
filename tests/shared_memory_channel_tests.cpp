#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "ipc/shared_memory_channel.hpp"

#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

fast_channel::Slot slot_with(const std::initializer_list<std::byte> bytes) {
    fast_channel::Slot slot{};
    std::copy(bytes.begin(), bytes.end(), slot.begin());
    return slot;
}

fast_channel::Slot empty_slot() {
    return fast_channel::Slot{};
}

}

TEST_CASE("shared memory channel exchanges a slot between processes") {
    const std::string name = "/fast_channel_test_" + std::to_string(getpid());
    auto producer = fast_channel::SharedMemoryChannel::create_or_open(name);
    const auto expected = slot_with({std::byte{0x10}, std::byte{0x20}, std::byte{0x30}});

    const auto child = fork();
    REQUIRE(child >= 0);
    if (child == 0) {
        try {
            auto consumer = fast_channel::SharedMemoryChannel::create_or_open(name);
            consumer.await_readable();
            const auto actual = consumer.fetch();
            _exit(actual[0] == expected[0] && actual[1] == expected[1] &&
                          actual[2] == expected[2] &&
                          actual[fast_channel::kSlotSize - 1] == std::byte{0}
                      ? 0
                      : 1);
        } catch (...) {
            _exit(2);
        }
    }

    producer.await_writable();
    producer.publish(expected);

    int status = 0;
    REQUIRE(waitpid(child, &status, 0) == child);
    CHECK(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == 0);
}

TEST_CASE("concurrent create_or_open converges on a single channel") {
    const std::string name = "/fast_channel_race_" + std::to_string(getpid());

    const auto child = fork();
    REQUIRE(child >= 0);
    if (child == 0) {
        alarm(10);
        try {
            auto channel = fast_channel::SharedMemoryChannel::create_or_open(name);
            channel.await_readable();
            const auto slot = channel.fetch();
            _exit(slot[0] == std::byte{0x11} ? 0 : 1);
        } catch (...) {
            _exit(2);
        }
    }

    auto channel = fast_channel::SharedMemoryChannel::create_or_open(name);
    channel.await_writable();
    channel.publish(slot_with({std::byte{0x11}, std::byte{0x22}}));

    int status = 0;
    REQUIRE(waitpid(child, &status, 0) == child);
    REQUIRE(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == 0);
}

TEST_CASE("shared memory channel rejects invalid names") {
    CHECK_THROWS_AS(fast_channel::SharedMemoryChannel::create_or_open("invalid"),
                    std::invalid_argument);
}

TEST_CASE("repeated create_or_open attaches to the same channel") {
    const std::string name = "/fast_channel_reuse_" + std::to_string(getpid());
    auto first = fast_channel::SharedMemoryChannel::create_or_open(name);
    auto second = fast_channel::SharedMemoryChannel::create_or_open(name);

    const auto expected = slot_with({std::byte{0x41}, std::byte{0x42}});
    first.await_writable();
    first.publish(expected);

    second.await_readable();
    const auto actual = second.fetch();
    CHECK(actual[0] == expected[0]);
    CHECK(actual[1] == expected[1]);
    CHECK(actual[2] == std::byte{0});
}

TEST_CASE("await_readable blocks until a slot is published") {
    const std::string name = "/fast_channel_late_producer_" + std::to_string(getpid());
    auto consumer = fast_channel::SharedMemoryChannel::create_or_open(name);

    const auto child = fork();
    REQUIRE(child >= 0);
    if (child == 0) {
        alarm(10);
        try {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            auto producer = fast_channel::SharedMemoryChannel::create_or_open(name);
            producer.await_writable();
            producer.publish(empty_slot());
            _exit(0);
        } catch (...) {
            _exit(2);
        }
    }

    const auto wait_start = std::chrono::steady_clock::now();
    consumer.await_readable();
    const auto blocked = std::chrono::steady_clock::now() - wait_start;
    consumer.fetch();
    CHECK(blocked >= std::chrono::milliseconds(150));

    int status = 0;
    REQUIRE(waitpid(child, &status, 0) == child);
    REQUIRE(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == 0);
}

TEST_CASE("await_writable blocks until the peer consumes the slot") {
    const std::string name = "/fast_channel_late_consumer_" + std::to_string(getpid());
    auto producer = fast_channel::SharedMemoryChannel::create_or_open(name);
    const auto slot = empty_slot();

    const auto child = fork();
    REQUIRE(child >= 0);
    if (child == 0) {
        alarm(10);
        try {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            auto consumer = fast_channel::SharedMemoryChannel::create_or_open(name);
            consumer.await_readable();
            consumer.fetch();
            _exit(0);
        } catch (...) {
            _exit(2);
        }
    }

    producer.await_writable();
    producer.publish(slot);

    const auto wait_start = std::chrono::steady_clock::now();
    producer.await_writable();
    const auto blocked = std::chrono::steady_clock::now() - wait_start;
    producer.publish(slot);
    CHECK(blocked >= std::chrono::milliseconds(150));

    int status = 0;
    REQUIRE(waitpid(child, &status, 0) == child);
    REQUIRE(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == 0);
}

TEST_CASE("slot reservation order is enforced") {
    const std::string name = "/fast_channel_reservation_" + std::to_string(getpid());
    auto channel = fast_channel::SharedMemoryChannel::create_or_open(name);
    const auto slot = slot_with({std::byte{0x51}});

    CHECK_THROWS_AS(channel.publish(slot), std::logic_error);
    CHECK_THROWS_AS(channel.fetch(), std::logic_error);

    channel.await_writable();
    CHECK_THROWS_AS(channel.await_writable(), std::logic_error);
    channel.publish(slot);
    CHECK_THROWS_AS(channel.publish(slot), std::logic_error);

    channel.await_readable();
    CHECK_THROWS_AS(channel.await_readable(), std::logic_error);
    const auto received = channel.fetch();
    CHECK(received[0] == slot[0]);
    CHECK_THROWS_AS(channel.fetch(), std::logic_error);
}
