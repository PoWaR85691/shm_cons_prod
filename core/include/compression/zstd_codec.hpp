#pragma once

#include "compression/codec.hpp"

namespace fast_channel {

class ZstdCompressor : public Compressor {
public:
    explicit ZstdCompressor(int compression_level = 1);

    CompressedBlock compress(InputBlock block) const override;

private:
    int compression_level_;
};

class ZstdDecompressor : public Decompressor {
public:
    std::vector<float> decompress(const CompressedBlock& block) const override;
};

}
