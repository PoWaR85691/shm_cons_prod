#pragma once

#include "compression/codec.hpp"
#include "compression/options.hpp"

#include <memory>

namespace fast_channel {

std::unique_ptr<Compressor> make_compressor(const CompressionOptions& options);
std::unique_ptr<Decompressor> make_decompressor(CompressionAlgorithm algorithm);

}
