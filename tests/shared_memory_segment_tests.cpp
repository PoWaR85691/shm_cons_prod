#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "ipc/shared_memory_segment.hpp"

#include "ipc/ipc_error.hpp"

#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

std::string unique_name(const std::string& suffix) {
    return "/fast_channel_shm_" + std::to_string(getpid()) + "_" + suffix;
}

}

TEST_CASE("created segment exposes the requested size") {
    auto segment = fast_channel::SharedMemorySegment::create(unique_name("size"), 256);
    CHECK(segment.size() == 256);
    CHECK(segment.data() != nullptr);
    CHECK(segment.name() == unique_name("size"));
}

TEST_CASE("segment validates name and size") {
    CHECK_THROWS_AS(fast_channel::SharedMemorySegment::create("invalid", 256),
                    std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::SharedMemorySegment::create(unique_name("zero"), 0),
                    std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::SharedMemorySegment::open(unique_name("missing")),
                    std::runtime_error);
}

TEST_CASE("opening a missing segment reports a missing object") {
    try {
        fast_channel::SharedMemorySegment::open(unique_name("absent"));
        FAIL("expected an IpcError");
    } catch (const fast_channel::IpcError& error) {
        CHECK(error.error_code() == ENOENT);
    }
}

TEST_CASE("creating a duplicate segment fails") {
    const auto name = unique_name("duplicate");
    auto first = fast_channel::SharedMemorySegment::create(name, 256);
    CHECK_THROWS_AS(fast_channel::SharedMemorySegment::create(name, 256), std::runtime_error);
}

TEST_CASE("opened segment sees the same storage") {
    const auto name = unique_name("shared");
    auto owner = fast_channel::SharedMemorySegment::create(name, 256);
    auto viewer = fast_channel::SharedMemorySegment::open(name);

    CHECK(viewer.size() == owner.size());
    auto* owner_bytes = static_cast<std::byte*>(owner.data());
    owner_bytes[0] = std::byte{0x7F};
    const auto* viewer_bytes = static_cast<const std::byte*>(viewer.data());
    CHECK(viewer_bytes[0] == std::byte{0x7F});
}

TEST_CASE("segment is shared between processes") {
    const auto name = unique_name("process");
    auto segment = fast_channel::SharedMemorySegment::create(name, 256);

    const auto child = fork();
    REQUIRE(child >= 0);
    if (child == 0) {
        try {
            auto child_segment = fast_channel::SharedMemorySegment::open(name);
            static_cast<std::byte*>(child_segment.data())[1] = std::byte{0x2A};
            _exit(0);
        } catch (...) {
            _exit(1);
        }
    }

    int status = 0;
    REQUIRE(waitpid(child, &status, 0) == child);
    CHECK(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == 0);
    CHECK(static_cast<const std::byte*>(segment.data())[1] == std::byte{0x2A});
}

TEST_CASE("non-owner does not unlink the segment") {
    const auto name = unique_name("owner");
    auto owner = fast_channel::SharedMemorySegment::create(name, 256);
    {
        auto viewer = fast_channel::SharedMemorySegment::open(name);
        CHECK(viewer.size() == 256);
    }
    CHECK_NOTHROW(fast_channel::SharedMemorySegment::open(name));
}

TEST_CASE("owner unlinks the segment on destruction") {
    const auto name = unique_name("unlink");
    {
        auto segment = fast_channel::SharedMemorySegment::create(name, 256);
        CHECK_NOTHROW(fast_channel::SharedMemorySegment::open(name));
    }
    CHECK_THROWS_AS(fast_channel::SharedMemorySegment::open(name), std::runtime_error);
}

TEST_CASE("moving transfers ownership") {
    const auto name = unique_name("move");
    auto original = fast_channel::SharedMemorySegment::create(name, 256);

    auto moved = std::move(original);
    CHECK(moved.size() == 256);
    CHECK(moved.name() == name);

    auto target = fast_channel::SharedMemorySegment::create(unique_name("target"), 128);
    target = std::move(moved);
    CHECK(target.size() == 256);
    CHECK(target.name() == name);

    auto replacement = fast_channel::SharedMemorySegment::open(name);
    CHECK(replacement.size() == 256);
}

TEST_CASE("segment can be resized and keeps its contents") {
    const auto name = unique_name("resize");
    auto segment = fast_channel::SharedMemorySegment::create(name, 64);
    CHECK(segment.size() == 64);
    static_cast<std::byte*>(segment.data())[0] = std::byte{0x11};

    segment.resize(256);
    CHECK(segment.size() == 256);
    CHECK(static_cast<const std::byte*>(segment.data())[0] == std::byte{0x11});

    auto viewer = fast_channel::SharedMemorySegment::open(name);
    CHECK(viewer.size() == 256);
    CHECK(static_cast<const std::byte*>(viewer.data())[0] == std::byte{0x11});
}

TEST_CASE("explicit unlink removes the name and keeps the mapping") {
    const auto name = unique_name("explicit-unlink");
    auto segment = fast_channel::SharedMemorySegment::create(name, 128);
    CHECK(segment.is_owner());
    CHECK(segment.is_linked());

    segment.unlink();
    CHECK_FALSE(segment.is_owner());
    CHECK_FALSE(segment.is_linked());
    CHECK(segment.size() == 128);
    CHECK(segment.data() != nullptr);
    CHECK_THROWS_AS(fast_channel::SharedMemorySegment::open(name), std::runtime_error);
}

TEST_CASE("refresh picks up a size change made by another process") {
    const auto name = unique_name("refresh");
    auto creator = fast_channel::SharedMemorySegment::create(name, 64);
    auto viewer = fast_channel::SharedMemorySegment::open(name);
    CHECK(viewer.size() == 64);

    creator.resize(256);
    viewer.refresh();

    CHECK(viewer.size() == 256);
    CHECK(viewer.data() != nullptr);
    static_cast<std::byte*>(creator.data())[200] = std::byte{0x5A};
    CHECK(static_cast<const std::byte*>(viewer.data())[200] == std::byte{0x5A});
}

TEST_CASE("segment lock excludes another process") {
    const auto name = unique_name("lock");
    auto segment = fast_channel::SharedMemorySegment::create(name, 64);
    segment.lock();

    const auto child = fork();
    REQUIRE(child >= 0);
    if (child == 0) {
        alarm(5);
        try {
            auto child_segment = fast_channel::SharedMemorySegment::open(name);
            child_segment.lock();
            child_segment.unlock();
            _exit(0);
        } catch (...) {
            _exit(1);
        }
    }

    usleep(50000);
    segment.unlock();

    int status = 0;
    REQUIRE(waitpid(child, &status, 0) == child);
    CHECK(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == 0);
}
