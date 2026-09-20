#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "ipc/spin_lock.hpp"

#include "ipc/ipc_error.hpp"

#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <new>
#include <type_traits>

namespace {

using fast_channel::SpinLock;

class AlarmGuard {
public:
    AlarmGuard() { alarm(10); }
    ~AlarmGuard() { alarm(0); }
};

class MappedLock {
public:
    explicit MappedLock(const bool held) {
        storage_ = mmap(nullptr, sizeof(SpinLock), PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (storage_ == MAP_FAILED) {
            throw fast_channel::IpcError("mmap", errno);
        }
        lock_ = new (storage_) SpinLock{};
        lock_->initialize(held);
    }

    ~MappedLock() { munmap(storage_, sizeof(SpinLock)); }

    MappedLock(const MappedLock&) = delete;
    MappedLock& operator=(const MappedLock&) = delete;

    SpinLock& get() { return *lock_; }

private:
    void* storage_;
    SpinLock* lock_;
};

// On assertion failure or exception, reap the child before the mapping is destroyed.
class ChildGuard {
public:
    explicit ChildGuard(const pid_t child) : child_(child) {}
    ~ChildGuard() {
        if (child_ > 0) {
            kill(child_, SIGKILL);
            int status = 0;
            while (waitpid(child_, &status, 0) == -1 && errno == EINTR) {
            }
        }
    }

    ChildGuard(const ChildGuard&) = delete;
    ChildGuard& operator=(const ChildGuard&) = delete;

    int join() {
        int status = 0;
        pid_t result = -1;
        do {
            result = waitpid(child_, &status, 0);
        } while (result == -1 && errno == EINTR);
        if (result == -1) {
            throw fast_channel::IpcError("waitpid", errno);
        }
        child_ = -1;
        return status;
    }

private:
    pid_t child_;
};

}

TEST_CASE("spin lock is exactly one atomic flag and is not copyable") {
    CHECK(sizeof(SpinLock) == sizeof(std::atomic<bool>));
    CHECK(alignof(SpinLock) == alignof(std::atomic<bool>));
    CHECK(std::is_standard_layout_v<SpinLock>);
    CHECK_FALSE(std::is_copy_constructible_v<SpinLock>);
    CHECK_FALSE(std::is_copy_assignable_v<SpinLock>);
    CHECK_FALSE(std::is_move_constructible_v<SpinLock>);
    CHECK_FALSE(std::is_move_assignable_v<SpinLock>);
}

TEST_CASE("spin lock reports the state it was initialized with") {
    MappedLock unlocked(false);
    MappedLock locked(true);
    CHECK_FALSE(unlocked.get().held());
    CHECK(locked.get().held());
}

TEST_CASE("lock takes a free lock and unlock releases it") {
    MappedLock mapped(false);
    auto& lock = mapped.get();

    lock.lock();
    CHECK(lock.held());
    lock.unlock();
    CHECK_FALSE(lock.held());
}

TEST_CASE("lock on a free lock returns without waiting") {
    AlarmGuard alarm_guard;
    MappedLock mapped(false);

    const auto start = std::chrono::steady_clock::now();
    mapped.get().lock();
    const auto elapsed = std::chrono::steady_clock::now() - start;

    CHECK(mapped.get().held());
    CHECK(elapsed < std::chrono::seconds(1));
    mapped.get().unlock();
}

TEST_CASE("lock waits for the peer process to release") {
    AlarmGuard alarm_guard;
    MappedLock mapped(true);

    const auto child = fork();
    REQUIRE(child >= 0);
    if (child == 0) {
        // Alarm timers are not inherited across fork.
        alarm(5);
        try {
            usleep(200000);
            mapped.get().unlock();
            _exit(0);
        } catch (...) {
            _exit(1);
        }
    }

    ChildGuard child_guard(child);
    const auto start = std::chrono::steady_clock::now();
    mapped.get().lock();
    const auto waited = std::chrono::steady_clock::now() - start;

    CHECK(waited >= std::chrono::milliseconds(150));
    CHECK(mapped.get().held());

    const int status = child_guard.join();
    REQUIRE(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == 0);
}

TEST_CASE("spin lock hands the baton between processes in both directions") {
    AlarmGuard alarm_guard;
    MappedLock request(true);
    MappedLock response(true);

    const auto child = fork();
    REQUIRE(child >= 0);
    if (child == 0) {
        alarm(5);
        try {
            request.get().lock();
            request.get().unlock();
            response.get().unlock();
            _exit(0);
        } catch (...) {
            _exit(1);
        }
    }

    ChildGuard child_guard(child);
    request.get().unlock();
    response.get().lock();
    response.get().unlock();

    const int status = child_guard.join();
    REQUIRE(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == 0);
}
