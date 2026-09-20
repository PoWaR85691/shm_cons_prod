#include "compression/none_codec.hpp"

#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace fast_channel {

namespace {

void check_uint32_size(const std::size_t size, const char* field) {
    if (size > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument(std::string(field) + " exceeds uint32 range");
    }
}

}

CompressedBlock NoneCompressor::compress(InputBlock block) const {
    const auto input_size = block.values.size() * sizeof(float);
    check_uint32_size(input_size, "input block");

    std::vector<std::byte> data(input_size);
    std::memcpy(data.data(), block.values.data(), input_size);
    return {block.id,
            CompressionAlgorithm::None,
            static_cast<std::uint32_t>(input_size),
            std::move(data)};
}

std::vector<float> NoneDecompressor::decompress(const CompressedBlock& block) const {
    if (block.data.size() != block.original_size) {
        throw std::invalid_argument("stored block size does not match original size");
    }

    std::vector<float> values(block.original_size / sizeof(float));
    std::memcpy(values.data(), block.data.data(), block.data.size());
    return values;
}

}
