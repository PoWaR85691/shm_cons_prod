#pragma once

#include <stdexcept>
#include <string>

namespace fast_channel {

class IpcError : public std::runtime_error {
public:
    IpcError(const std::string& operation, int error_code);

    int error_code() const noexcept;

private:
    int error_code_;
};

}
