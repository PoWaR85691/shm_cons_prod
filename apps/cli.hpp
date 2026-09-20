#pragma once

#include "compression/options.hpp"

#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>

namespace fast_channel::cli {

constexpr int kUsageExitCode = 2;
constexpr int kFailureExitCode = 1;

inline constexpr const char* kDefaultChannelName = "/fast_channel";
inline constexpr const char* kAlgorithmVariable = "COMPRESSION_ALGORITHM";
inline constexpr const char* kWorkersVariable = "WORKERS_COUNT";
inline constexpr const char* kDefaultAlgorithm = "zstd";

inline std::size_t default_worker_count() {
    const auto count = std::thread::hardware_concurrency();
    return count == 0 ? 1 : count;
}

inline std::size_t parse_worker_count(const std::string& text) {
    std::size_t count = 0;
    const char* const begin = text.data();
    const char* const end = begin + text.size();
    const auto parsed = std::from_chars(begin, end, count);
    if (parsed.ec != std::errc{} || parsed.ptr != end || count == 0) {
        throw std::invalid_argument(std::string("invalid ") + kWorkersVariable + ": " + text);
    }
    return count;
}

inline std::size_t worker_count_from_env() {
    const char* const value = std::getenv(kWorkersVariable);
    return value == nullptr ? default_worker_count() : parse_worker_count(value);
}

inline CompressionOptions compression_options_from_env() {
    const char* const value = std::getenv(kAlgorithmVariable);
    return parse_compression_options(value == nullptr ? kDefaultAlgorithm : value);
}

}
