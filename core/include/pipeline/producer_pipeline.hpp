#pragma once

#include "compression/options.hpp"
#include "ipc/shared_memory_channel.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace fast_channel {

struct ProducerOptions {
    std::string input_path;
    std::size_t block_values{16 * 1024};
    std::size_t worker_count{1};
    CompressionOptions compression{};
};

struct PipelineMetrics {
    std::uint64_t original_bytes{0};
    std::uint64_t transmitted_bytes{0};
    double elapsed_seconds{0.0};
};

double compression_percent(const PipelineMetrics& metrics) noexcept;

PipelineMetrics run_producer(const ProducerOptions& options, SharedMemoryChannel& channel);

}
