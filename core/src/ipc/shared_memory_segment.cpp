#include "ipc/shared_memory_segment.hpp"

#include "ipc/ipc_error.hpp"

#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <stdexcept>
#include <utility>

namespace fast_channel {

namespace {

IpcError system_error(const char* operation) {
    return IpcError(operation, errno);
}

void validate_name(const std::string& name) {
    if (name.empty() || name.front() != '/') {
        throw std::invalid_argument("shared memory name must start with '/'");
    }
}

void* map_segment(const int fd, const std::size_t size) {
    if (size == 0) {
        return nullptr;
    }
    void* data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        throw system_error("mmap");
    }
    return data;
}

struct Status {
    std::size_t size;
    nlink_t links;
};

Status status_of(const int fd) {
    struct stat info {};
    if (fstat(fd, &info) == -1) {
        throw system_error("fstat");
    }
    return {static_cast<std::size_t>(info.st_size), info.st_nlink};
}

int lock_flag(const bool exclusive) {
    return exclusive ? LOCK_EX : LOCK_SH;
}

}

SharedMemorySegment SharedMemorySegment::create(const std::string& name, const std::size_t size) {
    validate_name(name);
    if (size == 0) {
        throw std::invalid_argument("shared memory size must be positive");
    }

    const int fd = shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1) {
        throw system_error("shm_open");
    }

    try {
        if (ftruncate(fd, static_cast<off_t>(size)) == -1) {
            throw system_error("ftruncate");
        }
        return SharedMemorySegment(name, fd, map_segment(fd, size), size, true);
    } catch (...) {
        ::close(fd);
        shm_unlink(name.c_str());
        throw;
    }
}

SharedMemorySegment SharedMemorySegment::open(const std::string& name) {
    validate_name(name);
    const int fd = shm_open(name.c_str(), O_RDWR, 0600);
    if (fd == -1) {
        throw system_error("shm_open");
    }

    try {
        const auto status = status_of(fd);
        return SharedMemorySegment(name, fd, map_segment(fd, status.size), status.size, false);
    } catch (...) {
        ::close(fd);
        throw;
    }
}

SharedMemorySegment::SharedMemorySegment(std::string name,
                                         const int fd,
                                         void* const data,
                                         const std::size_t size,
                                         const bool owner)
    : name_(std::move(name)), fd_(fd), data_(data), size_(size), owner_(owner) {}

SharedMemorySegment::SharedMemorySegment(SharedMemorySegment&& other) noexcept
    : name_(std::move(other.name_)),
      fd_(other.fd_),
      data_(other.data_),
      size_(other.size_),
      owner_(other.owner_) {
    other.fd_ = -1;
    other.data_ = nullptr;
    other.size_ = 0;
    other.owner_ = false;
}

SharedMemorySegment& SharedMemorySegment::operator=(SharedMemorySegment&& other) noexcept {
    if (this != &other) {
        release();
        name_ = std::move(other.name_);
        fd_ = other.fd_;
        data_ = other.data_;
        size_ = other.size_;
        owner_ = other.owner_;
        other.fd_ = -1;
        other.data_ = nullptr;
        other.size_ = 0;
        other.owner_ = false;
    }
    return *this;
}

SharedMemorySegment::~SharedMemorySegment() {
    release();
}

void SharedMemorySegment::release() noexcept {
    if (fd_ != -1 && owner_ && is_linked()) {
        shm_unlink(name_.c_str());
    }
    if (data_ != nullptr) {
        munmap(data_, size_);
    }
    if (fd_ != -1) {
        ::close(fd_);
    }
    data_ = nullptr;
    fd_ = -1;
    size_ = 0;
    owner_ = false;
}

void* SharedMemorySegment::data() noexcept {
    return data_;
}

const void* SharedMemorySegment::data() const noexcept {
    return data_;
}

std::size_t SharedMemorySegment::size() const noexcept {
    return size_;
}

const std::string& SharedMemorySegment::name() const noexcept {
    return name_;
}

bool SharedMemorySegment::is_owner() const noexcept {
    return owner_;
}

bool SharedMemorySegment::is_linked() const {
    return fd_ != -1 && status_of(fd_).links != 0;
}

void SharedMemorySegment::lock() const {
    if (flock(fd_, lock_flag(true)) == -1) {
        throw system_error("flock");
    }
}

void SharedMemorySegment::unlock() const {
    if (flock(fd_, LOCK_UN) == -1) {
        throw system_error("flock");
    }
}

void SharedMemorySegment::refresh() {
    const auto status = status_of(fd_);
    if (status.size == size_) {
        return;
    }
    void* fresh = map_segment(fd_, status.size);
    if (data_ != nullptr) {
        munmap(data_, size_);
    }
    data_ = fresh;
    size_ = status.size;
}

void SharedMemorySegment::resize(const std::size_t size) {
    if (ftruncate(fd_, static_cast<off_t>(size)) == -1) {
        throw system_error("ftruncate");
    }

    void* fresh = map_segment(fd_, size);
    if (data_ != nullptr) {
        munmap(data_, size_);
    }
    data_ = fresh;
    size_ = size;
}

void SharedMemorySegment::unlink() noexcept {
    shm_unlink(name_.c_str());
    owner_ = false;
}

}
