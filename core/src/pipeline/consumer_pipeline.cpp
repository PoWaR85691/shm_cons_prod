#include "pipeline/consumer_pipeline.hpp"

#include "channel/block_protocol.hpp"
#include "compression/codec_factory.hpp"
#include "concurrency/bounded_queue.hpp"
#include "concurrency/exception_store.hpp"
#include "concurrency/ordered_queue.hpp"
#include "io/float_file_writer.hpp"
#include "io/float_record.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace fast_channel {

namespace {

struct DecompressionTask {
    CompressedBlock block;
};

struct DecompressionResult {
    std::vector<float> values;
};

std::vector<std::thread> start_decompression_workers(
    const ConsumerOptions& options,
    BoundedQueue<DecompressionTask>& input_queue,
    OrderedQueue<DecompressionResult>& output_queue,
    ExceptionStore& failure) {
    std::vector<std::thread> workers;
    workers.reserve(options.worker_count);
    for (std::size_t index = 0; index < options.worker_count; ++index) {
        workers.emplace_back([&] {
            try {
                std::unique_ptr<Decompressor> decompressor;
                CompressionAlgorithm algorithm{};
                while (auto task = input_queue.pop()) {
                    if (decompressor == nullptr || task->block.algorithm != algorithm) {
                        algorithm = task->block.algorithm;
                        decompressor = make_decompressor(algorithm);
                    }
                    const auto block_id = task->block.id;
                    DecompressionResult result{decompressor->decompress(task->block)};
                    if (!output_queue.push(block_id, std::move(result))) {
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

std::thread start_output_writer(OrderedQueue<DecompressionResult>& output_queue,
                                BoundedQueue<DecompressionTask>& input_queue,
                                FloatFileWriter& output,
                                ExceptionStore& failure,
                                std::uint64_t& original_bytes) {
    return std::thread([&] {
        try {
            while (auto result = output_queue.pop()) {
                output.write(result->values);
                original_bytes += result->values.size() * kFloatValueSize;
            }
        } catch (...) {
            failure.capture_current();
            input_queue.close();
            output_queue.close();
        }
    });
}

void receive_blocks(SharedMemoryChannel& channel,
                    BoundedQueue<DecompressionTask>& input_queue,
                    OrderedQueue<DecompressionResult>& output_queue,
                    ExceptionStore& failure,
                    std::uint64_t& transmitted_bytes,
                    double& peer_wait_seconds) {
    try {
        bool awaiting_peer = true;
        // The wait for the very first frame is the wait for the producer to appear,
        // not part of the transfer time.
        const auto read_slot = [&] {
            if (awaiting_peer) {
                const auto wait_start = std::chrono::steady_clock::now();
                channel.await_readable();
                const auto blocked = std::chrono::steady_clock::now() - wait_start;
                peer_wait_seconds = std::chrono::duration<double>(blocked).count();
                awaiting_peer = false;
            } else {
                channel.await_readable();
            }
            return channel.fetch();
        };

        while (true) {
            const auto begin = read_block_begin(read_slot());
            if (is_stream_end(begin)) {
                break;
            }

            std::vector<std::byte> data;
            data.reserve(begin.compressed_size);
            while (data.size() < begin.compressed_size) {
                const auto slot = read_slot();
                const auto size = data_slot_size(begin, data.size());
                data.insert(data.end(), slot.begin(), slot.begin() + size);
            }

            transmitted_bytes += begin.compressed_size;
            CompressedBlock block{
                begin.block_id, begin.algorithm, begin.original_size, std::move(data)};
            if (!input_queue.push(DecompressionTask{std::move(block)})) {
                throw std::runtime_error("decompression input queue closed unexpectedly");
            }
        }
    } catch (...) {
        failure.capture_current();
        output_queue.close();
    }
    input_queue.close();
}

}

PipelineMetrics run_consumer(const ConsumerOptions& options, SharedMemoryChannel& channel) {
    if (options.worker_count == 0) {
        throw std::invalid_argument("worker count must be positive");
    }

    FloatFileWriter output(options.output_path);

    const auto start = std::chrono::steady_clock::now();
    BoundedQueue<DecompressionTask> input_queue(options.worker_count * 2);
    OrderedQueue<DecompressionResult> output_queue;
    ExceptionStore failure;
    std::uint64_t original_bytes = 0;
    std::uint64_t transmitted_bytes = 0;
    double peer_wait_seconds = 0.0;

    auto workers = start_decompression_workers(options, input_queue, output_queue, failure);
    auto output_thread =
        start_output_writer(output_queue, input_queue, output, failure, original_bytes);
    receive_blocks(
        channel, input_queue, output_queue, failure, transmitted_bytes, peer_wait_seconds);

    for (auto& worker : workers) {
        worker.join();
    }
    output_queue.close();
    output_thread.join();
    failure.rethrow_if_set();

    PipelineMetrics metrics;
    metrics.original_bytes = original_bytes;
    metrics.transmitted_bytes = transmitted_bytes;
    const auto elapsed =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    metrics.elapsed_seconds = std::max(0.0, elapsed - peer_wait_seconds);
    return metrics;
}

}
