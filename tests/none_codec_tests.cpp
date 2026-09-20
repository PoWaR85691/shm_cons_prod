#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "compression/none_codec.hpp"

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace {

std::vector<float> make_values(const std::size_t size) {
    std::vector<float> values(size);
    for (std::size_t index = 0; index < size; ++index) {
        values[index] = static_cast<float>(index) * 0.25F;
    }
    return values;
}

}

TEST_CASE("none codec stores values without compression") {
    const fast_channel::InputBlock input{5, make_values(1024)};
    const auto compressed = fast_channel::NoneCompressor{}.compress(input);

    CHECK(compressed.id == input.id);
    CHECK(compressed.algorithm == fast_channel::CompressionAlgorithm::None);
    CHECK(compressed.original_size == input.values.size() * sizeof(float));
    CHECK(compressed.data.size() == compressed.original_size);
    CHECK(fast_channel::NoneDecompressor{}.decompress(compressed) == input.values);
}

TEST_CASE("none codec rejects a payload of the wrong size") {
    const fast_channel::CompressedBlock block{
        1, fast_channel::CompressionAlgorithm::None, 16, std::vector<std::byte>(8)};

    CHECK_THROWS_AS(fast_channel::NoneDecompressor{}.decompress(block), std::invalid_argument);
}
