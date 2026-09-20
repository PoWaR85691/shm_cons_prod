#include "io/float_file_writer.hpp"

#include "io/float_record.hpp"

#include <cstring>
#include <stdexcept>

namespace fast_channel {

FloatFileWriter::FloatFileWriter(const std::string& path) : output_(path, std::ios::binary) {
    if (!output_) {
        throw std::runtime_error("cannot open output file: " + path);
    }
}

void FloatFileWriter::write(const std::vector<float>& values) {
    buffer_.resize(values.size() * kFloatRecordSize);
    for (std::size_t index = 0; index < values.size(); ++index) {
        std::memcpy(buffer_.data() + index * kFloatRecordSize, &values[index], kFloatValueSize);
        buffer_[index * kFloatRecordSize + kFloatValueSize] =
            static_cast<std::byte>(kFloatSeparator);
    }

    output_.write(reinterpret_cast<const char*>(buffer_.data()),
                  static_cast<std::streamsize>(buffer_.size()));
    if (!output_) {
        throw std::runtime_error("failed to write output file");
    }
}

}
