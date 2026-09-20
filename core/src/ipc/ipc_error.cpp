#include "ipc/ipc_error.hpp"

#include <cstring>

namespace fast_channel {

IpcError::IpcError(const std::string& operation, const int error_code)
    : std::runtime_error(operation + ": " + std::strerror(error_code)), error_code_(error_code) {}

int IpcError::error_code() const noexcept {
    return error_code_;
}

}
