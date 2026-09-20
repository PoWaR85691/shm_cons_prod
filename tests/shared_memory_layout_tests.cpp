#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "ipc/shared_memory_layout.hpp"

#include "ipc/shared_semaphore.hpp"
#include "ipc/spin_lock.hpp"

#include <cstddef>
#include <type_traits>

#if defined(FAST_CHANNEL_SYNC_SEMAPHORE)
static_assert(std::is_same_v<fast_channel::ChannelLock, fast_channel::SharedSemaphore>,
              "FAST_CHANNEL_SYNC_SEMAPHORE must select the semaphore backend");
#else
static_assert(std::is_same_v<fast_channel::ChannelLock, fast_channel::SpinLock>,
              "the default backend is the spin lock");
#endif

TEST_CASE("channel layout fits the fixed segment size") {
    CHECK(fast_channel::kControlSize + fast_channel::kSlotSize ==
          fast_channel::kSharedMemorySize);
    CHECK(sizeof(fast_channel::Slot) == fast_channel::kSlotSize);
}

TEST_CASE("control block is exactly the two selected locks") {
    CHECK(fast_channel::kControlSize == 2 * sizeof(fast_channel::ChannelLock));
}
