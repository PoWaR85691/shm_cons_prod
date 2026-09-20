#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "concurrency/bounded_queue.hpp"

#include <atomic>
#include <chrono>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

TEST_CASE("queue rejects zero capacity") {
    CHECK_THROWS_AS(fast_channel::BoundedQueue<int>(0), std::invalid_argument);
}

TEST_CASE("queue preserves FIFO order") {
    fast_channel::BoundedQueue<int> queue(2);
    CHECK(queue.push(10));
    CHECK(queue.push(20));
    CHECK(queue.size() == 2);
    CHECK(queue.pop().value() == 10);
    CHECK(queue.pop().value() == 20);
    queue.close();
    CHECK_FALSE(queue.pop().has_value());
}

TEST_CASE("closing queue drains pending values and rejects new values") {
    fast_channel::BoundedQueue<int> queue(2);
    CHECK(queue.push(7));
    queue.close();

    CHECK(queue.is_closed());
    CHECK(queue.pop().value() == 7);
    CHECK_FALSE(queue.pop().has_value());
    CHECK_FALSE(queue.push(8));
}

TEST_CASE("producer waits until consumer pops from a full queue") {
    fast_channel::BoundedQueue<int> queue(1);
    CHECK(queue.push(1));

    bool pushed = false;
    std::thread producer([&] {
        pushed = queue.push(2);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    CHECK_FALSE(pushed);
    CHECK(queue.pop().value() == 1);
    producer.join();
    CHECK(pushed);
    CHECK(queue.pop().value() == 2);
}

TEST_CASE("queue supports multiple producers and consumers") {
    constexpr int producer_count = 4;
    constexpr int values_per_producer = 1000;
    fast_channel::BoundedQueue<int> queue(16);
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::vector<int> values;
    std::mutex values_mutex;
    std::atomic<bool> producers_succeeded{true};

    for (int producer_id = 0; producer_id < producer_count; ++producer_id) {
        producers.emplace_back([&, producer_id] {
            for (int value = 0; value < values_per_producer; ++value) {
                if (!queue.push(producer_id * values_per_producer + value)) {
                    producers_succeeded = false;
                }
            }
        });
    }
    for (int consumer_id = 0; consumer_id < 4; ++consumer_id) {
        consumers.emplace_back([&] {
            while (auto value = queue.pop()) {
                std::lock_guard<std::mutex> lock(values_mutex);
                values.push_back(*value);
            }
        });
    }

    for (auto& producer : producers) {
        producer.join();
    }
    queue.close();
    for (auto& consumer : consumers) {
        consumer.join();
    }

    CHECK(producers_succeeded.load());
    CHECK(values.size() == static_cast<std::size_t>(producer_count * values_per_producer));
}
