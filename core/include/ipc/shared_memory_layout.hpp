#pragma once

#include "ipc/sync_lock.hpp"

#include <array>
#include <cstddef>
#include <type_traits>

namespace fast_channel {

constexpr std::size_t kSharedMemorySize = 256;

struct SharedMemoryControl {
    ChannelLock empty;
    ChannelLock full;
};

static_assert(std::is_standard_layout_v<SharedMemoryControl>);

constexpr std::size_t kControlSize = sizeof(SharedMemoryControl);
constexpr std::size_t kSlotSize = kSharedMemorySize - kControlSize;

using Slot = std::array<std::byte, kSlotSize>;

void initialize_control(SharedMemoryControl& control);

}
