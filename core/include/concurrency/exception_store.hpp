#pragma once

#include <exception>
#include <mutex>

namespace fast_channel {

class ExceptionStore {
public:
    void capture_current() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!exception_) {
            exception_ = std::current_exception();
        }
    }

    void rethrow_if_set() const {
        std::exception_ptr exception;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            exception = exception_;
        }
        if (exception) {
            std::rethrow_exception(exception);
        }
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return !exception_;
    }

private:
    mutable std::mutex mutex_;
    std::exception_ptr exception_;
};

}
