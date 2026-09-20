#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "concurrency/exception_store.hpp"

#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

TEST_CASE("empty store does not rethrow") {
    const fast_channel::ExceptionStore store;
    CHECK(store.empty());
    CHECK_NOTHROW(store.rethrow_if_set());
}

TEST_CASE("store rethrows the captured exception") {
    fast_channel::ExceptionStore store;
    try {
        throw std::runtime_error("failure");
    } catch (...) {
        store.capture_current();
    }

    CHECK_FALSE(store.empty());
    CHECK_THROWS_AS(store.rethrow_if_set(), std::runtime_error);
    try {
        store.rethrow_if_set();
    } catch (const std::runtime_error& error) {
        CHECK(std::string(error.what()) == "failure");
    }
}

TEST_CASE("store keeps the first captured exception") {
    fast_channel::ExceptionStore store;
    try {
        throw std::runtime_error("first");
    } catch (...) {
        store.capture_current();
    }
    try {
        throw std::logic_error("second");
    } catch (...) {
        store.capture_current();
    }

    try {
        store.rethrow_if_set();
    } catch (const std::runtime_error& error) {
        CHECK(std::string(error.what()) == "first");
    }
}

TEST_CASE("store collects an exception from another thread") {
    fast_channel::ExceptionStore store;
    std::thread worker([&store] {
        try {
            throw std::runtime_error("worker failure");
        } catch (...) {
            store.capture_current();
        }
    });
    worker.join();

    CHECK_THROWS_AS(store.rethrow_if_set(), std::runtime_error);
}

TEST_CASE("store handles concurrent captures") {
    fast_channel::ExceptionStore store;
    std::vector<std::thread> workers;
    for (int index = 0; index < 8; ++index) {
        workers.emplace_back([&store] {
            try {
                throw std::runtime_error("concurrent");
            } catch (...) {
                store.capture_current();
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }

    CHECK_THROWS_AS(store.rethrow_if_set(), std::runtime_error);
}
