#include "ipc/spin_lock.hpp"

namespace fast_channel {

void SpinLock::initialize(const bool held) {
    held_.store(held, std::memory_order_relaxed);
}

void SpinLock::lock() {
    while (true) {
        while (held_.load(std::memory_order_relaxed)) {
        }
        if (!held_.exchange(true, std::memory_order_acquire)) {
            return;
        }
    }
}

void SpinLock::unlock() {
    held_.store(false, std::memory_order_release);
}

bool SpinLock::held() const {
    return held_.load(std::memory_order_relaxed);
}

}
