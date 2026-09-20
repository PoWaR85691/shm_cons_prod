#pragma once

#include "compression/block.hpp"

#include <string>

namespace fast_channel {

struct CompressionOptions {
    CompressionAlgorithm algorithm{CompressionAlgorithm::Zstd};
    int level{1};
    double accuracy{0.001};
};

CompressionOptions parse_compression_options(const std::string& text);

}
