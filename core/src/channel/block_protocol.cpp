#include "channel/block_protocol.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace fast_channel {

namespace {

Slot slot_with_prefix(const void* data, const std::size_t size) {
    Slot slot{};
    std::memcpy(slot.data(), data, size);
    return slot;
}

}

Slot make_begin_slot(const CompressedBlock& block) {
    if (block.data.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument("compressed block exceeds protocol limits");
    }
    const BlockBegin begin{block.id,
                           block.algorithm,
                           static_cast<std::uint32_t>(block.data.size()),
                           block.original_size};
    return slot_with_prefix(&begin, sizeof(begin));
}

Slot make_data_slot(const CompressedBlock& block, const std::size_t offset) {
    if (offset >= block.data.size()) {
        throw std::invalid_argument("data slot offset is outside block");
    }
    const auto size = std::min(kSlotSize, block.data.size() - offset);
    return slot_with_prefix(block.data.data() + offset, size);
}

Slot make_end_slot() {
    return Slot{};
}

BlockBegin read_block_begin(const Slot& slot) {
    BlockBegin begin{};
    std::memcpy(&begin, slot.data(), sizeof(begin));
    return begin;
}

bool is_stream_end(const BlockBegin& begin) noexcept {
    return begin.compressed_size == 0;
}

std::size_t data_slot_size(const BlockBegin& begin, const std::size_t received) noexcept {
    return std::min<std::size_t>(kSlotSize, begin.compressed_size - received);
}

}
