#include "ipc/shared_memory_channel.hpp"

#include "ipc/ipc_error.hpp"

#include <cerrno>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <utility>

namespace fast_channel {

namespace {

SharedMemoryControl* control_from(SharedMemorySegment& segment) {
    return static_cast<SharedMemoryControl*>(segment.data());
}

bool is_committed(const SharedMemorySegment& segment) {
    return segment.size() == kSharedMemorySize;
}

std::optional<SharedMemorySegment> open_existing(const std::string& name) {
    try {
        return SharedMemorySegment::open(name);
    } catch (const IpcError& error) {
        if (error.error_code() != ENOENT) {
            throw;
        }
        return std::nullopt;
    }
}

}

SharedMemoryChannel SharedMemoryChannel::make(SharedMemorySegment segment) {
    auto* control = static_cast<SharedMemoryControl*>(segment.data());
    auto* bytes = static_cast<std::byte*>(segment.data());
    return SharedMemoryChannel(std::move(segment), control, bytes + kControlSize);
}

SharedMemoryChannel SharedMemoryChannel::create_or_open(const std::string& name) {
    while (true) {
        auto existing = open_existing(name);
        if (existing) {
            existing->lock();
            existing->refresh();
            if (is_committed(*existing)) {
                existing->unlock();
                return make(std::move(*existing));
            }
            existing->unlink();
            continue;
        }

        std::optional<SharedMemorySegment> created;
        try {
            created = SharedMemorySegment::create(name, kControlSize);
        } catch (const IpcError& error) {
            if (error.error_code() != EEXIST) {
                throw;
            }
            continue;
        }

        created->lock();
        if (!created->is_linked()) {
            continue;
        }
        initialize_control(*control_from(*created));
        created->resize(kSharedMemorySize);
        created->unlock();
        return make(std::move(*created));
    }
}

SharedMemoryChannel::SharedMemoryChannel(SharedMemorySegment segment,
                                         SharedMemoryControl* control,
                                         std::byte* slot)
    : segment_(std::move(segment)), control_(control), slot_(slot) {}

SharedMemoryChannel::SharedMemoryChannel(SharedMemoryChannel&& other) noexcept
    : segment_(std::move(other.segment_)),
      control_(other.control_),
      slot_(other.slot_),
      writable_reserved_(other.writable_reserved_),
      readable_reserved_(other.readable_reserved_) {
    other.control_ = nullptr;
    other.slot_ = nullptr;
    other.writable_reserved_ = false;
    other.readable_reserved_ = false;
}

SharedMemoryChannel& SharedMemoryChannel::operator=(SharedMemoryChannel&& other) noexcept {
    if (this != &other) {
        segment_ = std::move(other.segment_);
        control_ = other.control_;
        slot_ = other.slot_;
        writable_reserved_ = other.writable_reserved_;
        readable_reserved_ = other.readable_reserved_;
        other.control_ = nullptr;
        other.slot_ = nullptr;
        other.writable_reserved_ = false;
        other.readable_reserved_ = false;
    }
    return *this;
}

void SharedMemoryChannel::await_writable() {
    if (writable_reserved_) {
        throw std::logic_error("slot is already reserved for writing");
    }
    control_->empty.lock();
    writable_reserved_ = true;
}

void SharedMemoryChannel::publish(const Slot& slot) {
    if (!writable_reserved_) {
        throw std::logic_error("slot must be reserved before publishing");
    }
    std::memcpy(slot_, slot.data(), kSlotSize);
    control_->full.unlock();
    writable_reserved_ = false;
}

void SharedMemoryChannel::await_readable() {
    if (readable_reserved_) {
        throw std::logic_error("slot is already reserved for reading");
    }
    control_->full.lock();
    readable_reserved_ = true;
}

Slot SharedMemoryChannel::fetch() {
    if (!readable_reserved_) {
        throw std::logic_error("slot must be reserved before fetching");
    }
    Slot slot{};
    std::memcpy(slot.data(), slot_, kSlotSize);
    control_->empty.unlock();
    readable_reserved_ = false;
    return slot;
}

const std::string& SharedMemoryChannel::name() const noexcept {
    return segment_.name();
}

}
