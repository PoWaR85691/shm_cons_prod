#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "ipc/ipc_error.hpp"

#include <cerrno>
#include <stdexcept>
#include <string>

TEST_CASE("IpcError stores the error code") {
    const fast_channel::IpcError error("shm_open", ENOENT);
    CHECK(error.error_code() == ENOENT);
}

TEST_CASE("IpcError appends the system message to the operation") {
    const fast_channel::IpcError error("sem_open", EACCES);
    const std::string message = error.what();
    CHECK(message.rfind("sem_open", 0) == 0);
    CHECK(message.size() > std::string("sem_open").size());
}

TEST_CASE("IpcError is a runtime_error") {
    const fast_channel::IpcError error("mmap", EINVAL);
    CHECK_THROWS_AS(throw error, std::runtime_error);
}
