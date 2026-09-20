#pragma once

#include "compression/codec.hpp"

namespace fast_channel {

class NoneCompressor : public Compressor {
public:
    CompressedBlock compress(InputBlock block) const override;
};

class NoneDecompressor : public Decompressor {
public:
    std::vector<float> decompress(const CompressedBlock& block) const override;
};

}
