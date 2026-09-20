#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "io/float_file_reader.hpp"
#include "io/float_file_writer.hpp"
#include "io/float_record.hpp"

#include <unistd.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class TemporaryFile {
public:
    TemporaryFile() {
        path_ = "/tmp/fast_channel_writer_" + std::to_string(getpid()) + ".bin";
    }
    ~TemporaryFile() { unlink(path_.c_str()); }
    const std::string& path() const { return path_; }

private:
    std::string path_;
};

std::vector<std::byte> read_bytes(const std::string& path, const std::size_t size) {
    std::ifstream input(path, std::ios::binary);
    std::vector<std::byte> bytes(size);
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
    return bytes;
}

}

TEST_CASE("writer emits float32 followed by a newline") {
    TemporaryFile file;
    {
        fast_channel::FloatFileWriter writer(file.path());
        writer.write({1.0F, -2.5F});
    }

    const auto bytes = read_bytes(file.path(), 2 * fast_channel::kFloatRecordSize);
    std::array<float, 2> restored{};
    std::memcpy(&restored[0], bytes.data(), fast_channel::kFloatValueSize);
    std::memcpy(&restored[1],
                bytes.data() + fast_channel::kFloatRecordSize,
                fast_channel::kFloatValueSize);

    CHECK(restored[0] == 1.0F);
    CHECK(restored[1] == -2.5F);
    CHECK(bytes[fast_channel::kFloatValueSize] == std::byte{'\n'});
    CHECK(bytes[fast_channel::kFloatRecordSize + fast_channel::kFloatValueSize] == std::byte{'\n'});
}

TEST_CASE("writer appends blocks without gaps") {
    TemporaryFile file;
    {
        fast_channel::FloatFileWriter writer(file.path());
        writer.write({1.0F, 2.0F});
        writer.write({3.0F});
    }

    std::ifstream input(file.path(), std::ios::binary);
    input.seekg(0, std::ios::end);
    CHECK(input.tellg() == static_cast<std::streamoff>(3 * fast_channel::kFloatRecordSize));
}

TEST_CASE("writer output is read back by the reader") {
    TemporaryFile file;
    const std::vector<float> values{0.5F, 7.25F, -13.0F, 3.5F};
    {
        fast_channel::FloatFileWriter writer(file.path());
        writer.write(values);
    }

    fast_channel::FloatFileReader reader(file.path(), 3);
    fast_channel::InputBlock first{};
    fast_channel::InputBlock second{};

    CHECK(reader.read_next(first));
    CHECK(reader.read_next(second));
    CHECK_FALSE(reader.read_next(first));
    CHECK(first.values == std::vector<float>{0.5F, 7.25F, -13.0F});
    CHECK(second.values == std::vector<float>{3.5F});
}

TEST_CASE("writer rejects an unopenable path") {
    CHECK_THROWS_AS(
        fast_channel::FloatFileWriter("/tmp/missing-fast-channel-dir/out.bin"),
        std::runtime_error);
}
