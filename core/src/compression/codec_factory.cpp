#include "compression/codec_factory.hpp"

#include "compression/none_codec.hpp"
#include "compression/zfp_codec.hpp"
#include "compression/zstd_codec.hpp"

#include <stdexcept>
#include <string>

namespace fast_channel {

namespace {

[[noreturn]] void throw_unknown_algorithm(const CompressionAlgorithm algorithm) {
    throw std::invalid_argument("unknown compression algorithm: " +
                                std::to_string(static_cast<unsigned>(algorithm)));
}

}

std::unique_ptr<Compressor> make_compressor(const CompressionOptions& options) {
    switch (options.algorithm) {
    case CompressionAlgorithm::None:
        return std::make_unique<NoneCompressor>();
    case CompressionAlgorithm::Zstd:
        return std::make_unique<ZstdCompressor>(options.level);
    case CompressionAlgorithm::Zfp:
        return std::make_unique<ZfpCompressor>(options.accuracy);
    }
    throw_unknown_algorithm(options.algorithm);
}

std::unique_ptr<Decompressor> make_decompressor(const CompressionAlgorithm algorithm) {
    switch (algorithm) {
    case CompressionAlgorithm::None:
        return std::make_unique<NoneDecompressor>();
    case CompressionAlgorithm::Zstd:
        return std::make_unique<ZstdDecompressor>();
    case CompressionAlgorithm::Zfp:
        return std::make_unique<ZfpDecompressor>();
    }
    throw_unknown_algorithm(algorithm);
}

}
