#pragma once

#include "compression/codec.hpp"

namespace fast_channel {

class ZfpCompressor : public Compressor {
public:
    explicit ZfpCompressor(double accuracy);

    CompressedBlock compress(InputBlock block) const override;

private:
    double accuracy_;
};

class ZfpDecompressor : public Decompressor {
public:
    std::vector<float> decompress(const CompressedBlock& block) const override;
};

}
