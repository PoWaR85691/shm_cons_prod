#include "pipeline/producer_pipeline.hpp"

#include "channel/block_protocol.hpp"
#include "compression/codec_factory.hpp"
#include "concurrency/bounded_queue.hpp"
#include "concurrency/exception_store.hpp"
#include "io/float_file_reader.hpp"
#include "io/float_record.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>

namespace fast_channel {

namespace {

struct CompressionTask {
    InputBlock block;
};

struct CompressionResult {
    CompressedBlock block;
};

void try_send_end_slot(SharedMemoryChannel& channel) noexcept {
    try {
        channel.await_writable();
        channel.publish(make_end_slot());
    } catch (...) {
        // The channel is unusable; the original failure is reported instead.
    }
}

std::unique_ptr<FloatFileReader> open_reader(const ProducerOptions& options,
                                             SharedMemoryChannel& channel) {
    try {
        return std::make_unique<FloatFileReader>(options.input_path, options.block_values);
    } catch (...) {
        try_send_end_slot(channel);
        throw;
    }
}

std::vector<std::thread> start_compression_workers(
    const ProducerOptions& options,
    BoundedQueue<CompressionTask>& input_queue,
    BoundedQueue<CompressionResult>& output_queue,
    ExceptionStore& failure) {
    std::vector<std::thread> workers;
    workers.reserve(options.worker_count);
    for (std::size_t index = 0; index < options.worker_count; ++index) {
        workers.emplace_back([&] {
            try {
                const auto compressor = make_compressor(options.compression);
                while (auto task = input_queue.pop()) {
                    auto block = compressor->compress(std::move(task->block));
                    if (!output_queue.push(CompressionResult{std::move(block)})) {
                        break;
                    }
                }
            } catch (...) {
                failure.capture_current();
                input_queue.close();
                output_queue.close();
            }
        });
    }
    return workers;
}

std::thread start_channel_writer(BoundedQueue<CompressionResult>& output_queue,
                                 BoundedQueue<CompressionTask>& input_queue,
                                 SharedMemoryChannel& channel,
                                 ExceptionStore& failure,
                                 double& peer_wait_seconds,
                                 std::uint64_t& transmitted_bytes) {
    return std::thread([&] {
        try {
            std::uint64_t published = 0;
            // The wait that follows the first published slot is the wait for the
            // consumer to appear, not part of the transfer time.
            const auto publish_slot = [&](const Slot& slot) {
                if (published == 1) {
                    const auto wait_start = std::chrono::steady_clock::now();
                    channel.await_writable();
                    const auto blocked = std::chrono::steady_clock::now() - wait_start;
                    peer_wait_seconds = std::chrono::duration<double>(blocked).count();
                } else {
                    channel.await_writable();
                }
                channel.publish(slot);
                ++published;
            };

            while (auto result = output_queue.pop()) {
                const auto& block = result->block;
                publish_slot(make_begin_slot(block));
                for (std::size_t offset = 0; offset < block.data.size(); offset += kSlotSize) {
                    publish_slot(make_data_slot(block, offset));
                }
                transmitted_bytes += block.data.size();
            }
            if (failure.empty()) {
                publish_slot(make_end_slot());
            }
        } catch (...) {
            failure.capture_current();
            input_queue.close();
            output_queue.close();
        }
    });
}

void read_blocks(FloatFileReader& reader,
                 BoundedQueue<CompressionTask>& input_queue,
                 BoundedQueue<CompressionResult>& output_queue,
                 ExceptionStore& failure,
                 std::uint64_t& original_bytes) {
    try {
        InputBlock input{};
        while (reader.read_next(input)) {
            original_bytes += input.values.size() * kFloatValueSize;
            if (!input_queue.push(CompressionTask{std::move(input)})) {
                throw std::runtime_error("compression input queue closed unexpectedly");
            }
            input = {};
        }
        input_queue.close();
    } catch (...) {
        failure.capture_current();
        input_queue.close();
        output_queue.close();
    }
}

}

double compression_percent(const PipelineMetrics& metrics) noexcept {
    if (metrics.original_bytes == 0) {
        return 0.0;
    }
    const auto ratio = static_cast<double>(metrics.transmitted_bytes) /
                       static_cast<double>(metrics.original_bytes);
    return 100.0 * (1.0 - ratio);
}

PipelineMetrics run_producer(const ProducerOptions& options, SharedMemoryChannel& channel) {
    if (options.worker_count == 0) {
        try_send_end_slot(channel);
        throw std::invalid_argument("worker count must be positive");
    }

    const auto start = std::chrono::steady_clock::now();
    auto reader = open_reader(options, channel);
    BoundedQueue<CompressionTask> input_queue(options.worker_count * 2);
    BoundedQueue<CompressionResult> output_queue(options.worker_count * 2);
    ExceptionStore failure;
    std::uint64_t original_bytes = 0;
    std::uint64_t transmitted_bytes = 0;
    double peer_wait_seconds = 0.0;

    auto workers = start_compression_workers(options, input_queue, output_queue, failure);
    auto output_thread = start_channel_writer(
        output_queue, input_queue, channel, failure, peer_wait_seconds, transmitted_bytes);
    read_blocks(*reader, input_queue, output_queue, failure, original_bytes);

    for (auto& worker : workers) {
        worker.join();
    }
    output_queue.close();
    output_thread.join();

    try {
        failure.rethrow_if_set();
    } catch (...) {
        try_send_end_slot(channel);
        throw;
    }

    PipelineMetrics metrics;
    metrics.original_bytes = original_bytes;
    metrics.transmitted_bytes = transmitted_bytes;
    const auto elapsed =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    metrics.elapsed_seconds = std::max(0.0, elapsed - peer_wait_seconds);
    return metrics;
}

}
