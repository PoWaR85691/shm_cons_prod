#include "compression/options.hpp"

#include <stdexcept>
#include <string>

namespace fast_channel {

namespace {

constexpr const char* kSyntax = "compression options must look like <none|zstd|zfp>[,<parameter>]";

std::invalid_argument bad_parameter(const char* name, const std::string& parameter) {
    return std::invalid_argument(std::string("invalid ") + name + ": " + parameter);
}

int parse_level(const std::string& parameter) {
    std::size_t consumed = 0;
    int level = 0;
    try {
        level = std::stoi(parameter, &consumed);
    } catch (const std::exception&) {
        throw bad_parameter("zstd level", parameter);
    }
    if (consumed != parameter.size() || level < 1) {
        throw bad_parameter("zstd level", parameter);
    }
    return level;
}

double parse_accuracy(const std::string& parameter) {
    std::size_t consumed = 0;
    double accuracy = 0.0;
    try {
        accuracy = std::stod(parameter, &consumed);
    } catch (const std::exception&) {
        throw bad_parameter("zfp accuracy", parameter);
    }
    if (consumed != parameter.size() || !(accuracy > 0.0)) {
        throw bad_parameter("zfp accuracy", parameter);
    }
    return accuracy;
}

}

CompressionOptions parse_compression_options(const std::string& text) {
    const auto separator = text.find(',');
    const auto name = text.substr(0, separator);
    const auto has_parameter = separator != std::string::npos;
    const auto parameter = has_parameter ? text.substr(separator + 1) : std::string{};

    if (has_parameter && (parameter.empty() || parameter.find(',') != std::string::npos)) {
        throw std::invalid_argument(kSyntax);
    }

    CompressionOptions options;
    if (name == "none") {
        if (has_parameter) {
            throw std::invalid_argument("none takes no parameter");
        }
        options.algorithm = CompressionAlgorithm::None;
        return options;
    }
    if (name == "zstd") {
        options.algorithm = CompressionAlgorithm::Zstd;
        options.level = has_parameter ? parse_level(parameter) : options.level;
        return options;
    }
    if (name == "zfp") {
        options.algorithm = CompressionAlgorithm::Zfp;
        options.accuracy = has_parameter ? parse_accuracy(parameter) : options.accuracy;
        return options;
    }

    throw std::invalid_argument(std::string(kSyntax) + ", got: " + text);
}

}
