#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace fast_channel {

enum class CompressionAlgorithm : std::uint8_t {
    None = 0,
    Zstd = 1,
    Zfp = 2
};

struct InputBlock {
    std::uint64_t id;
    std::vector<float> values;
};

struct CompressedBlock {
    std::uint64_t id;
    CompressionAlgorithm algorithm;
    std::uint32_t original_size;
    std::vector<std::byte> data;
};

}
