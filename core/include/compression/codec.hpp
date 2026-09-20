#pragma once

#include "compression/block.hpp"

namespace fast_channel {

class Compressor {
public:
    Compressor(const Compressor&) = delete;
    Compressor& operator=(const Compressor&) = delete;
    virtual ~Compressor() = default;

    virtual CompressedBlock compress(InputBlock block) const = 0;

protected:
    Compressor() = default;
};

class Decompressor {
public:
    Decompressor(const Decompressor&) = delete;
    Decompressor& operator=(const Decompressor&) = delete;
    virtual ~Decompressor() = default;

    virtual std::vector<float> decompress(const CompressedBlock& block) const = 0;

protected:
    Decompressor() = default;
};

}
