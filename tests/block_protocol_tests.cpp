#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "channel/block_protocol.hpp"
#include "compression/zstd_codec.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {

std::vector<std::byte> make_data(const std::size_t size) {
    std::vector<std::byte> data(size);
    for (std::size_t index = 0; index < size; ++index) {
        data[index] = static_cast<std::byte>(index % 13);
    }
    return data;
}

std::vector<std::byte> assemble(const fast_channel::CompressedBlock& block) {
    const auto begin = fast_channel::read_block_begin(fast_channel::make_begin_slot(block));
    std::vector<std::byte> data;
    while (data.size() < begin.compressed_size) {
        const auto slot = fast_channel::make_data_slot(block, data.size());
        const auto size = fast_channel::data_slot_size(begin, data.size());
        data.insert(data.end(), slot.begin(), slot.begin() + size);
    }
    return data;
}

}

TEST_CASE("begin slot carries block metadata") {
    const fast_channel::CompressedBlock block{
        7, fast_channel::CompressionAlgorithm::Zstd, 900, make_data(300)};
    const auto begin = fast_channel::read_block_begin(fast_channel::make_begin_slot(block));

    CHECK(begin.block_id == 7);
    CHECK(begin.algorithm == fast_channel::CompressionAlgorithm::Zstd);
    CHECK(begin.compressed_size == 300);
    CHECK(begin.original_size == 900);
    CHECK_FALSE(fast_channel::is_stream_end(begin));
}

TEST_CASE("end slot terminates the stream") {
    CHECK(fast_channel::is_stream_end(
        fast_channel::read_block_begin(fast_channel::make_end_slot())));
}

TEST_CASE("block survives the slot protocol") {
    for (const auto size : {std::size_t{1},
                            fast_channel::kSlotSize,
                            fast_channel::kSlotSize + 1,
                            3 * fast_channel::kSlotSize + 5}) {
        const fast_channel::CompressedBlock block{
            11, fast_channel::CompressionAlgorithm::None, 1000, make_data(size)};
        CHECK(assemble(block) == block.data);
    }
}

TEST_CASE("data slot size is derived from the begin metadata") {
    const fast_channel::CompressedBlock block{
        12, fast_channel::CompressionAlgorithm::None, 1000, make_data(fast_channel::kSlotSize + 7)};
    const auto begin = fast_channel::read_block_begin(fast_channel::make_begin_slot(block));

    CHECK(fast_channel::data_slot_size(begin, 0) == fast_channel::kSlotSize);
    CHECK(fast_channel::data_slot_size(begin, fast_channel::kSlotSize) == 7);
}

TEST_CASE("data slot offset must be inside the block") {
    const fast_channel::CompressedBlock block{
        13, fast_channel::CompressionAlgorithm::None, 1000, make_data(10)};
    CHECK_THROWS_AS(fast_channel::make_data_slot(block, 10), std::invalid_argument);
}

TEST_CASE("compressed block survives the slot protocol") {
    std::vector<float> values(16 * 1024);
    for (std::size_t index = 0; index < values.size(); ++index) {
        values[index] = static_cast<float>(index % 13) * 0.5F;
    }
    const fast_channel::InputBlock input{14, std::move(values)};
    const auto compressed = fast_channel::ZstdCompressor{}.compress(input);
    const fast_channel::CompressedBlock received{
        compressed.id, compressed.algorithm, compressed.original_size, assemble(compressed)};

    CHECK(fast_channel::read_block_begin(fast_channel::make_begin_slot(compressed)).algorithm ==
          fast_channel::CompressionAlgorithm::Zstd);
    CHECK(fast_channel::ZstdDecompressor{}.decompress(received) == input.values);
}
