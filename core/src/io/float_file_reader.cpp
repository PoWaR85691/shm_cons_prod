#include "io/float_file_reader.hpp"

#include "io/float_record.hpp"

#include <cstring>
#include <stdexcept>

namespace fast_channel {

FloatFileReader::FloatFileReader(const std::string& path, const std::size_t block_values)
    : input_(path, std::ios::binary), block_values_(block_values) {
    if (!input_) {
        throw std::runtime_error("cannot open input file: " + path);
    }
    if (block_values_ == 0) {
        throw std::invalid_argument("block values must be positive");
    }

    input_.seekg(0, std::ios::end);
    const auto file_size = input_.tellg();
    if (file_size < 0 || static_cast<std::uintmax_t>(file_size) % kFloatRecordSize != 0) {
        throw std::runtime_error("input file does not contain complete float records");
    }
    input_.seekg(0, std::ios::beg);
}

bool FloatFileReader::read_next(InputBlock& block) {
    const auto byte_budget = block_values_ * kFloatRecordSize;
    buffer_.resize(byte_budget);
    input_.read(reinterpret_cast<char*>(buffer_.data()), static_cast<std::streamsize>(byte_budget));
    const auto bytes_read = input_.gcount();
    if (bytes_read == 0) {
        if (input_.bad()) {
            throw std::runtime_error("failed to read input file");
        }
        return false;
    }

    const auto record_bytes = static_cast<std::size_t>(bytes_read);
    if (record_bytes % kFloatRecordSize != 0) {
        throw std::runtime_error("input file does not contain complete float records");
    }

    const auto value_count = record_bytes / kFloatRecordSize;
    std::vector<float> values(value_count);
    for (std::size_t index = 0; index < value_count; ++index) {
        const auto offset = index * kFloatRecordSize;
        if (buffer_[offset + kFloatValueSize] != static_cast<std::byte>(kFloatSeparator)) {
            throw std::runtime_error(
                "input file is not a sequence of float32 records with newline separators");
        }
        std::memcpy(&values[index], buffer_.data() + offset, kFloatValueSize);
    }

    block = {next_block_id_++, std::move(values)};
    return true;
}

}
