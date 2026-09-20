#pragma once

#include "compression/block.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace fast_channel {

class FloatFileReader {
public:
    FloatFileReader(const std::string& path, std::size_t block_values);

    bool read_next(InputBlock& block);

private:
    std::ifstream input_;
    const std::size_t block_values_;
    std::uint64_t next_block_id_{0};
    std::vector<std::byte> buffer_;
};

}
