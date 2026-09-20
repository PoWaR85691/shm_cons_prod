#pragma once

#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

namespace fast_channel {

class FloatFileWriter {
public:
    explicit FloatFileWriter(const std::string& path);

    void write(const std::vector<float>& values);

private:
    std::ofstream output_;
    std::vector<std::byte> buffer_;
};

}
