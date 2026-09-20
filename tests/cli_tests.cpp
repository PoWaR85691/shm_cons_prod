#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "cli.hpp"

#include <stdexcept>
#include <stdlib.h>

namespace {

// Leaves the variable unset before and after the case, so cases do not leak
// into each other or into the rest of the suite.
class ScopedUnsetEnv {
public:
    explicit ScopedUnsetEnv(const char* name) : name_(name) { ::unsetenv(name_); }
    ~ScopedUnsetEnv() { ::unsetenv(name_); }

    ScopedUnsetEnv(const ScopedUnsetEnv&) = delete;
    ScopedUnsetEnv& operator=(const ScopedUnsetEnv&) = delete;

private:
    const char* name_;
};

}

TEST_CASE("worker count defaults to the processor count when the variable is unset") {
    const ScopedUnsetEnv guard{fast_channel::cli::kWorkersVariable};
    CHECK(fast_channel::cli::worker_count_from_env() == fast_channel::cli::default_worker_count());
}

TEST_CASE("worker count is read from the environment") {
    const ScopedUnsetEnv guard{fast_channel::cli::kWorkersVariable};
    ::setenv(fast_channel::cli::kWorkersVariable, "3", 1);
    CHECK(fast_channel::cli::worker_count_from_env() == 3);
}

TEST_CASE("worker count must be a positive decimal integer") {
    CHECK(fast_channel::cli::parse_worker_count("1") == 1);
    CHECK(fast_channel::cli::parse_worker_count("128") == 128);

    CHECK_THROWS_AS(fast_channel::cli::parse_worker_count(""), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::cli::parse_worker_count("0"), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::cli::parse_worker_count("-1"), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::cli::parse_worker_count("+2"), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::cli::parse_worker_count("2.5"), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::cli::parse_worker_count(" 3"), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::cli::parse_worker_count("3 "), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::cli::parse_worker_count("abc"), std::invalid_argument);
    CHECK_THROWS_AS(
        fast_channel::cli::parse_worker_count("99999999999999999999999999"), std::invalid_argument);
}
