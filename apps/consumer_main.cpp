#include "cli.hpp"

#include "ipc/shared_memory_channel.hpp"
#include "pipeline/consumer_pipeline.hpp"

#include <exception>
#include <iomanip>
#include <iostream>

namespace {

const char* const kUsage = "usage: consumer <output>\n";

fast_channel::ConsumerOptions parse_arguments(char** argv) {
    fast_channel::ConsumerOptions options;
    options.output_path = argv[1];
    options.worker_count = fast_channel::cli::worker_count_from_env();
    return options;
}

void print_metrics(const fast_channel::PipelineMetrics& metrics) {
    std::cout << "consumer: original_bytes=" << metrics.original_bytes
              << " transmitted_bytes=" << metrics.transmitted_bytes
              << " elapsed_seconds=" << metrics.elapsed_seconds
              << " compression_percent=" << std::fixed << std::setprecision(2)
              << fast_channel::compression_percent(metrics) << '\n';
}

int run(const fast_channel::ConsumerOptions& options) {
    auto channel =
        fast_channel::SharedMemoryChannel::create_or_open(fast_channel::cli::kDefaultChannelName);
    print_metrics(fast_channel::run_consumer(options, channel));
    return 0;
}

}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << kUsage;
        return fast_channel::cli::kUsageExitCode;
    }

    try {
        return run(parse_arguments(argv));
    } catch (const std::exception& error) {
        std::cerr << "consumer error: " << error.what() << '\n';
        return fast_channel::cli::kFailureExitCode;
    }
}
