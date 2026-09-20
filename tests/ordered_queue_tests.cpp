#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "concurrency/ordered_queue.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <vector>

TEST_CASE("queue delivers values in key order") {
    fast_channel::OrderedQueue<int> queue;
    CHECK(queue.push(0, 10));
    CHECK(queue.push(1, 20));
    CHECK(queue.pop().value() == 10);
    CHECK(queue.pop().value() == 20);
}

TEST_CASE("queue holds values that arrive before their turn") {
    fast_channel::OrderedQueue<int> queue;
    CHECK(queue.push(2, 30));
    CHECK(queue.push(1, 20));
    CHECK(queue.size() == 2);

    std::vector<int> values;
    std::atomic<bool> finished{false};
    std::thread consumer([&] {
        while (auto value = queue.pop()) {
            values.push_back(*value);
        }
        finished = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    CHECK_FALSE(finished.load());
    CHECK(queue.size() == 2);

    CHECK(queue.push(0, 10));
    queue.close();
    consumer.join();

    CHECK(values == std::vector<int>{10, 20, 30});
    CHECK(finished.load());
}

TEST_CASE("closing the queue drains the in-order prefix") {
    fast_channel::OrderedQueue<int> queue;
    CHECK(queue.push(0, 10));
    CHECK(queue.push(1, 20));
    queue.close();

    CHECK(queue.is_closed());
    CHECK(queue.pop().value() == 10);
    CHECK(queue.pop().value() == 20);
    CHECK_FALSE(queue.pop().has_value());
    CHECK_FALSE(queue.push(2, 30));
}

TEST_CASE("closing the queue drops values behind a gap") {
    fast_channel::OrderedQueue<int> queue;
    CHECK(queue.push(1, 20));
    queue.close();

    CHECK_FALSE(queue.pop().has_value());
    CHECK(queue.size() == 1);
}

TEST_CASE("queue rejects duplicate and stale keys") {
    fast_channel::OrderedQueue<int> queue;
    CHECK(queue.push(0, 10));
    CHECK(queue.pop().value() == 10);

    CHECK_THROWS_AS(queue.push(0, 11), std::logic_error);
    CHECK(queue.push(1, 20));
    CHECK_THROWS_AS(queue.push(1, 21), std::logic_error);
}

TEST_CASE("queue supports multiple producers") {
    constexpr std::uint64_t producer_count = 4;
    constexpr std::uint64_t values_per_producer = 500;
    fast_channel::OrderedQueue<std::uint64_t> queue;

    std::vector<std::thread> producers;
    for (std::uint64_t producer_id = 0; producer_id < producer_count; ++producer_id) {
        producers.emplace_back([&, producer_id] {
            for (std::uint64_t index = 0; index < values_per_producer; ++index) {
                const auto key = producer_id * values_per_producer + index;
                queue.push(key, key);
            }
        });
    }

    std::vector<std::uint64_t> values;
    std::thread consumer([&] {
        while (auto value = queue.pop()) {
            values.push_back(*value);
        }
    });

    for (auto& producer : producers) {
        producer.join();
    }
    queue.close();
    consumer.join();

    std::vector<std::uint64_t> expected(values.size());
    std::iota(expected.begin(), expected.end(), std::uint64_t{0});
    CHECK(values.size() == producer_count * values_per_producer);
    CHECK(values == expected);
}
