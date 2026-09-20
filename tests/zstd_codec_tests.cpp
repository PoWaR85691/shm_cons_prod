#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "compression/zstd_codec.hpp"

#include <cstddef>
#include <random>
#include <stdexcept>
#include <vector>

namespace {

std::vector<float> repeated_values(const std::size_t size) {
    return std::vector<float>(size, 1.5F);
}

std::vector<float> make_random_values(const std::size_t size) {
    std::mt19937 generator(42);
    std::uniform_real_distribution<float> distribution(-1000.0F, 1000.0F);
    std::vector<float> result(size);
    for (auto& value : result) {
        value = distribution(generator);
    }
    return result;
}

}

TEST_CASE("compressor and decompressor preserve repeated values") {
    const fast_channel::InputBlock input{12, repeated_values(16 * 1024)};
    const auto compressed = fast_channel::ZstdCompressor{}.compress(input);

    CHECK(compressed.id == input.id);
    CHECK(compressed.algorithm == fast_channel::CompressionAlgorithm::Zstd);
    CHECK(compressed.original_size == input.values.size() * sizeof(float));
    CHECK(compressed.data.size() < compressed.original_size);
    CHECK(fast_channel::ZstdDecompressor{}.decompress(compressed) == input.values);
}

TEST_CASE("compressor and decompressor preserve incompressible values") {
    const fast_channel::InputBlock input{13, make_random_values(1024)};

    CHECK(fast_channel::ZstdDecompressor{}.decompress(
              fast_channel::ZstdCompressor{}.compress(input)) == input.values);
}

TEST_CASE("corrupted compressed block is rejected") {
    const fast_channel::InputBlock input{14, repeated_values(256)};
    auto compressed = fast_channel::ZstdCompressor{}.compress(input);
    compressed.data[0] ^= std::byte{0x01};

    CHECK_THROWS_AS(fast_channel::ZstdDecompressor{}.decompress(compressed), std::runtime_error);
}

TEST_CASE("invalid compression level is rejected") {
    CHECK_THROWS_AS(fast_channel::ZstdCompressor{0}, std::invalid_argument);
}
