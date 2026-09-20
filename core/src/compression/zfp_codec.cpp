#include "compression/zfp_codec.hpp"

#include <zfp.h>
#include <zfp/bitstream.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace fast_channel {

namespace {

using BitstreamHandle = std::unique_ptr<bitstream, decltype(&stream_close)>;
using ZfpStreamHandle = std::unique_ptr<zfp_stream, decltype(&zfp_stream_close)>;
using ZfpFieldHandle = std::unique_ptr<zfp_field, decltype(&zfp_field_free)>;

void check_uint32_size(const std::size_t size, const char* field) {
    if (size > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument(std::string(field) + " exceeds uint32 range");
    }
}

}

ZfpCompressor::ZfpCompressor(const double accuracy) : accuracy_(accuracy) {
    if (!(accuracy_ > 0.0)) {
        throw std::invalid_argument("zfp accuracy must be positive");
    }
}

CompressedBlock ZfpCompressor::compress(InputBlock block) const {
    const auto input_size = block.values.size() * sizeof(float);
    check_uint32_size(input_size, "input block");

    ZfpFieldHandle field{
        zfp_field_1d(block.values.data(), zfp_type_float, block.values.size()), &zfp_field_free};
    ZfpStreamHandle stream{zfp_stream_open(nullptr), &zfp_stream_close};
    if (field == nullptr || stream == nullptr) {
        throw std::runtime_error("zfp: failed to allocate stream state");
    }
    zfp_stream_set_accuracy(stream.get(), accuracy_);

    std::vector<std::byte> compressed(zfp_stream_maximum_size(stream.get(), field.get()));
    BitstreamHandle bitstream{stream_open(compressed.data(), compressed.size()), &stream_close};
    if (bitstream == nullptr) {
        throw std::runtime_error("zfp: failed to allocate bit stream");
    }
    zfp_stream_set_bit_stream(stream.get(), bitstream.get());
    zfp_stream_rewind(stream.get());
    zfp_write_header(stream.get(), field.get(), ZFP_HEADER_FULL);

    const auto compressed_size = zfp_compress(stream.get(), field.get());
    if (compressed_size == 0) {
        throw std::runtime_error("zfp: compression failed");
    }

    compressed.resize(compressed_size);
    return {block.id,
            CompressionAlgorithm::Zfp,
            static_cast<std::uint32_t>(input_size),
            std::move(compressed)};
}

std::vector<float> ZfpDecompressor::decompress(const CompressedBlock& block) const {
    std::vector<float> values(block.original_size / sizeof(float));

    ZfpFieldHandle field{
        zfp_field_1d(values.data(), zfp_type_float, values.size()), &zfp_field_free};
    auto* input = const_cast<std::byte*>(block.data.data());
    BitstreamHandle bitstream{stream_open(input, block.data.size()), &stream_close};
    ZfpStreamHandle stream{zfp_stream_open(bitstream.get()), &zfp_stream_close};
    if (field == nullptr || bitstream == nullptr || stream == nullptr) {
        throw std::runtime_error("zfp: failed to allocate stream state");
    }

    if (zfp_read_header(stream.get(), field.get(), ZFP_HEADER_FULL) == 0) {
        throw std::runtime_error("zfp: failed to read stream header");
    }
    if (zfp_decompress(stream.get(), field.get()) == 0) {
        throw std::runtime_error("zfp: decompression failed");
    }
    return values;
}

}
