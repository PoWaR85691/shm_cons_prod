#include "ipc/shared_semaphore.hpp"

#include "ipc/ipc_error.hpp"

#include <cerrno>

namespace fast_channel {

namespace {

IpcError system_error(const char* operation) {
    return IpcError(operation, errno);
}

}

void SharedSemaphore::initialize(const bool held) {
    const unsigned int initial_value = held ? 0U : 1U;
    if (sem_init(&handle_, 1, initial_value) == -1) {
        throw system_error("sem_init");
    }
}

void SharedSemaphore::lock() {
    int result = -1;
    do {
        result = sem_wait(&handle_);
    } while (result == -1 && errno == EINTR);
    if (result == -1) {
        throw system_error("sem_wait");
    }
}

void SharedSemaphore::unlock() {
    if (sem_post(&handle_) == -1) {
        throw system_error("sem_post");
    }
}

bool SharedSemaphore::held() const {
    int value = -1;
    if (sem_getvalue(const_cast<sem_t*>(&handle_), &value) == -1) {
        throw system_error("sem_getvalue");
    }
    return value == 0;
}

}
