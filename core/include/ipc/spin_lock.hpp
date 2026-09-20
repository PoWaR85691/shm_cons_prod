#pragma once

#include <atomic>

namespace fast_channel {

class SpinLock {
public:
    SpinLock() = default;
    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;
    SpinLock(SpinLock&&) = delete;
    SpinLock& operator=(SpinLock&&) = delete;

    void initialize(bool held);
    void lock();
    void unlock();
    bool held() const;

private:
    std::atomic<bool> held_{false};
};

static_assert(std::atomic<bool>::is_always_lock_free,
              "a shared memory lock requires an always lock-free atomic<bool>");

}
