#pragma once

#include "ipc/shared_memory_channel.hpp"
#include "pipeline/producer_pipeline.hpp"

#include <cstddef>
#include <string>

namespace fast_channel {

struct ConsumerOptions {
    std::string output_path;
    std::size_t worker_count{1};
};

PipelineMetrics run_consumer(const ConsumerOptions& options, SharedMemoryChannel& channel);

}
