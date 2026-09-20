#pragma once

#include "ipc/shared_semaphore.hpp"
#include "ipc/spin_lock.hpp"

namespace fast_channel {

#if defined(FAST_CHANNEL_SYNC_SEMAPHORE)
using ChannelLock = SharedSemaphore;
#elif defined(FAST_CHANNEL_SYNC_SPINLOCK)
using ChannelLock = SpinLock;
#else
#error "configure with -DFAST_CHANNEL_SYNC=spinlock or -DFAST_CHANNEL_SYNC=semaphore"
#endif

}
