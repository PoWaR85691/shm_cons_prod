#include "compression/zstd_codec.hpp"

#include <zstd.h>

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace fast_channel {

namespace {

void check_zstd_result(const std::size_t result, const char* operation) {
    if (ZSTD_isError(result) != 0) {
        throw std::runtime_error(std::string(operation) + ": " + ZSTD_getErrorName(result));
    }
}

void check_uint32_size(const std::size_t size, const char* field) {
    if (size > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument(std::string(field) + " exceeds uint32 range");
    }
}

}

ZstdCompressor::ZstdCompressor(const int compression_level)
    : compression_level_(compression_level) {
    if (compression_level_ < 1) {
        throw std::invalid_argument("compression level must be positive");
    }
}

CompressedBlock ZstdCompressor::compress(InputBlock block) const {
    const auto input_size = block.values.size() * sizeof(float);
    check_uint32_size(input_size, "input block");

    const auto bound = ZSTD_compressBound(input_size);
    std::vector<std::byte> compressed(bound);
    const auto compressed_size = ZSTD_compress(compressed.data(),
                                               compressed.size(),
                                               block.values.data(),
                                               input_size,
                                               compression_level_);
    check_zstd_result(compressed_size, "compression");

    compressed.resize(compressed_size);
    return {block.id,
            CompressionAlgorithm::Zstd,
            static_cast<std::uint32_t>(input_size),
            std::move(compressed)};
}

std::vector<float> ZstdDecompressor::decompress(const CompressedBlock& block) const {
    std::vector<float> values(block.original_size / sizeof(float));
    const auto restored_size = ZSTD_decompress(values.data(),
                                               block.original_size,
                                               block.data.data(),
                                               block.data.size());
    check_zstd_result(restored_size, "decompression");
    if (restored_size != block.original_size) {
        throw std::runtime_error("decompressed size does not match original size");
    }
    return values;
}

}
