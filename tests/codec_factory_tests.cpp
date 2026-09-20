#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "compression/codec_factory.hpp"

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace {

std::vector<float> make_values(const std::size_t size) {
    std::vector<float> values(size);
    for (std::size_t index = 0; index < size; ++index) {
        values[index] = static_cast<float>(index % 7) * 1.5F;
    }
    return values;
}

}

TEST_CASE("factory builds a working codec for every algorithm") {
    for (const auto options :
         {fast_channel::CompressionOptions{fast_channel::CompressionAlgorithm::None},
          fast_channel::CompressionOptions{fast_channel::CompressionAlgorithm::Zstd},
          fast_channel::CompressionOptions{fast_channel::CompressionAlgorithm::Zfp}}) {
        const fast_channel::InputBlock input{9, make_values(4096)};
        const auto compressor = fast_channel::make_compressor(options);
        const auto compressed = compressor->compress(input);

        CHECK(compressed.algorithm == options.algorithm);
        CHECK(compressed.original_size == input.values.size() * sizeof(float));

        const auto decompressor = fast_channel::make_decompressor(options.algorithm);
        CHECK(decompressor->decompress(compressed) == input.values);
    }
}

TEST_CASE("factory rejects unknown algorithms") {
    const auto unknown = static_cast<fast_channel::CompressionAlgorithm>(99);

    fast_channel::CompressionOptions options;
    options.algorithm = unknown;
    CHECK_THROWS_AS(fast_channel::make_compressor(options), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::make_decompressor(unknown), std::invalid_argument);
}
