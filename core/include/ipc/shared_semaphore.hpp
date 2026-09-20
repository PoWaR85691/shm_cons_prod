#pragma once

#include <semaphore.h>

namespace fast_channel {

class SharedSemaphore {
public:
    SharedSemaphore() = default;
    SharedSemaphore(const SharedSemaphore&) = delete;
    SharedSemaphore& operator=(const SharedSemaphore&) = delete;
    SharedSemaphore(SharedSemaphore&&) = delete;
    SharedSemaphore& operator=(SharedSemaphore&&) = delete;

    void initialize(bool held);
    void lock();
    void unlock();
    bool held() const;

private:
    sem_t handle_;
};

}
