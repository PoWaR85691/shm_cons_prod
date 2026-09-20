#pragma once

#include <cstddef>
#include <string>

namespace fast_channel {

class SharedMemorySegment {
public:
    static SharedMemorySegment create(const std::string& name, std::size_t size);
    static SharedMemorySegment open(const std::string& name);

    SharedMemorySegment(const SharedMemorySegment&) = delete;
    SharedMemorySegment& operator=(const SharedMemorySegment&) = delete;
    SharedMemorySegment(SharedMemorySegment&& other) noexcept;
    SharedMemorySegment& operator=(SharedMemorySegment&& other) noexcept;
    ~SharedMemorySegment();

    void* data() noexcept;
    const void* data() const noexcept;
    std::size_t size() const noexcept;
    const std::string& name() const noexcept;
    bool is_owner() const noexcept;
    bool is_linked() const;

    void lock() const;
    void unlock() const;
    void refresh();
    void resize(std::size_t size);
    void unlink() noexcept;

private:
    SharedMemorySegment(std::string name, int fd, void* data, std::size_t size, bool owner);
    void release() noexcept;

    std::string name_;
    int fd_{-1};
    void* data_{nullptr};
    std::size_t size_{0};
    bool owner_{false};
};

}
