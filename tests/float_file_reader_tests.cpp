#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "io/float_file_reader.hpp"

#include <unistd.h>

#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

namespace {

class TemporaryFile {
public:
    TemporaryFile() {
        path_ = "/tmp/fast_channel_reader_" + std::to_string(getpid()) + ".bin";
    }
    ~TemporaryFile() { unlink(path_.c_str()); }
    const std::string& path() const { return path_; }

private:
    std::string path_;
};

void write_records(const std::string& path, const std::vector<float>& values) {
    std::ofstream output(path, std::ios::binary);
    for (const auto value : values) {
        output.write(reinterpret_cast<const char*>(&value), sizeof(float));
        output.put('\n');
    }
}

}

TEST_CASE("reader returns values in sequential blocks") {
    TemporaryFile file;
    write_records(file.path(), {1.5F, -2.25F, 0.0F, 42.0F, 0.001F});

    fast_channel::FloatFileReader reader(file.path(), 3);
    fast_channel::InputBlock block{};

    CHECK(reader.read_next(block));
    CHECK(block.id == 0);
    CHECK(block.values == std::vector<float>{1.5F, -2.25F, 0.0F});

    CHECK(reader.read_next(block));
    CHECK(block.id == 1);
    CHECK(block.values == std::vector<float>{42.0F, 0.001F});

    CHECK_FALSE(reader.read_next(block));
}

TEST_CASE("reader rejects incomplete float record") {
    TemporaryFile file;
    {
        std::ofstream output(file.path(), std::ios::binary);
        output << "123456";
    }

    CHECK_THROWS_AS(fast_channel::FloatFileReader(file.path(), 10), std::runtime_error);
}

TEST_CASE("reader rejects a record without a newline separator") {
    TemporaryFile file;
    {
        const float value = 1.0F;
        std::ofstream output(file.path(), std::ios::binary);
        output.write(reinterpret_cast<const char*>(&value), sizeof(float));
        output.put('x');
    }

    fast_channel::FloatFileReader reader(file.path(), 1);
    fast_channel::InputBlock block{};
    CHECK_THROWS_AS(reader.read_next(block), std::runtime_error);
}

TEST_CASE("reader validates block values and file path") {
    TemporaryFile file;
    CHECK_THROWS_AS(fast_channel::FloatFileReader("/tmp/missing-fast-channel-file", 10),
                    std::runtime_error);

    write_records(file.path(), {1.0F});
    CHECK_THROWS_AS(fast_channel::FloatFileReader(file.path(), 0), std::invalid_argument);
}
