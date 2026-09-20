#pragma once

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

namespace fast_channel {

template <typename T>
class OrderedQueue {
public:
    OrderedQueue() = default;

    OrderedQueue(const OrderedQueue&) = delete;
    OrderedQueue& operator=(const OrderedQueue&) = delete;

    bool push(const std::uint64_t key, T value) {
        std::lock_guard lock(mutex_);
        if (closed_) {
            return false;
        }
        if (key < next_key_ || items_.count(key) != 0) {
            throw std::logic_error("ordered queue keys must be unique and increasing");
        }
        items_.emplace(key, std::move(value));
        ready_.notify_one();
        return true;
    }

    std::optional<T> pop() {
        std::unique_lock lock(mutex_);
        ready_.wait(lock, [this] { return closed_ || items_.count(next_key_) != 0; });
        const auto iterator = items_.find(next_key_);
        if (iterator == items_.end()) {
            return std::nullopt;
        }
        T value = std::move(iterator->second);
        items_.erase(iterator);
        ++next_key_;
        return value;
    }

    void close() {
        {
            std::lock_guard lock(mutex_);
            closed_ = true;
        }
        ready_.notify_all();
    }

    bool is_closed() const {
        std::lock_guard lock(mutex_);
        return closed_;
    }

    std::size_t size() const {
        std::lock_guard lock(mutex_);
        return items_.size();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable ready_;
    std::map<std::uint64_t, T> items_;
    std::uint64_t next_key_{0};
    bool closed_{false};
};

}
