#pragma once

#include "compression/block.hpp"
#include "ipc/shared_memory_layout.hpp"

#include <cstddef>
#include <cstdint>

namespace fast_channel {

struct BlockBegin {
    std::uint64_t block_id;
    CompressionAlgorithm algorithm;
    std::uint32_t compressed_size;
    std::uint32_t original_size;
};

Slot make_begin_slot(const CompressedBlock& block);
Slot make_data_slot(const CompressedBlock& block, std::size_t offset);
Slot make_end_slot();
BlockBegin read_block_begin(const Slot& slot);
bool is_stream_end(const BlockBegin& begin) noexcept;
std::size_t data_slot_size(const BlockBegin& begin, std::size_t received) noexcept;

}
