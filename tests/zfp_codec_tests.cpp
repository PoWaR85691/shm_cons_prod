#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "compression/zfp_codec.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace {

std::vector<float> make_values(const std::size_t size) {
    std::vector<float> values(size);
    for (std::size_t index = 0; index < size; ++index) {
        values[index] = 0.5F + 0.25F * std::sin(static_cast<float>(index) * 0.01F);
    }
    return values;
}

float max_absolute_error(const std::vector<float>& left, const std::vector<float>& right) {
    float worst = 0.0F;
    for (std::size_t index = 0; index < left.size(); ++index) {
        worst = std::max(worst, std::abs(left[index] - right[index]));
    }
    return worst;
}

}

TEST_CASE("zfp preserves values within the requested accuracy") {
    const fast_channel::InputBlock input{3, make_values(4096)};
    const fast_channel::ZfpCompressor compressor{0.001};
    const auto compressed = compressor.compress(input);

    CHECK(compressed.id == input.id);
    CHECK(compressed.algorithm == fast_channel::CompressionAlgorithm::Zfp);
    CHECK(compressed.original_size == input.values.size() * sizeof(float));
    CHECK(compressed.data.size() < compressed.original_size);

    const auto restored = fast_channel::ZfpDecompressor{}.decompress(compressed);
    REQUIRE(restored.size() == input.values.size());
    CHECK(max_absolute_error(input.values, restored) <= 0.001F);
}

TEST_CASE("zfp accuracy controls the payload size") {
    const fast_channel::InputBlock input{4, make_values(4096)};

    const auto coarse = fast_channel::ZfpCompressor{0.1}.compress(input);
    const auto fine = fast_channel::ZfpCompressor{0.0001}.compress(input);

    CHECK(coarse.data.size() < fine.data.size());
}

TEST_CASE("zfp rejects a non-positive accuracy") {
    CHECK_THROWS_AS(fast_channel::ZfpCompressor{0.0}, std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::ZfpCompressor{-1.0}, std::invalid_argument);
}
