#pragma once

#include "ipc/shared_memory_layout.hpp"
#include "ipc/shared_memory_segment.hpp"

#include <string>

namespace fast_channel {

class SharedMemoryChannel {
public:
    static SharedMemoryChannel create_or_open(const std::string& name);

    SharedMemoryChannel(const SharedMemoryChannel&) = delete;
    SharedMemoryChannel& operator=(const SharedMemoryChannel&) = delete;
    SharedMemoryChannel(SharedMemoryChannel&& other) noexcept;
    SharedMemoryChannel& operator=(SharedMemoryChannel&& other) noexcept;
    ~SharedMemoryChannel() = default;

    void await_writable();
    void publish(const Slot& slot);
    void await_readable();
    Slot fetch();

    const std::string& name() const noexcept;

private:
    static SharedMemoryChannel make(SharedMemorySegment segment);
    SharedMemoryChannel(SharedMemorySegment segment, SharedMemoryControl* control, std::byte* slot);

    SharedMemorySegment segment_;
    SharedMemoryControl* control_{nullptr};
    std::byte* slot_{nullptr};
    bool writable_reserved_{false};
    bool readable_reserved_{false};
};

}
