#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "compression/options.hpp"

#include <stdexcept>
#include <string>

TEST_CASE("algorithm is parsed without a parameter") {
    CHECK(fast_channel::parse_compression_options("none").algorithm ==
          fast_channel::CompressionAlgorithm::None);

    const auto zstd = fast_channel::parse_compression_options("zstd");
    CHECK(zstd.algorithm == fast_channel::CompressionAlgorithm::Zstd);
    CHECK(zstd.level == 1);

    const auto zfp = fast_channel::parse_compression_options("zfp");
    CHECK(zfp.algorithm == fast_channel::CompressionAlgorithm::Zfp);
    CHECK(zfp.accuracy == doctest::Approx(0.001));
}

TEST_CASE("parameter is parsed for the algorithms that take one") {
    const auto zstd = fast_channel::parse_compression_options("zstd,7");
    CHECK(zstd.algorithm == fast_channel::CompressionAlgorithm::Zstd);
    CHECK(zstd.level == 7);

    const auto zfp = fast_channel::parse_compression_options("zfp,0.001");
    CHECK(zfp.algorithm == fast_channel::CompressionAlgorithm::Zfp);
    CHECK(zfp.accuracy == doctest::Approx(0.001));
}

TEST_CASE("invalid options are rejected") {
    CHECK_THROWS_AS(fast_channel::parse_compression_options(""), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::parse_compression_options("lzma"), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::parse_compression_options("none,1"), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::parse_compression_options("zstd,"), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::parse_compression_options("zstd,1,2"), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::parse_compression_options("zstd,0"), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::parse_compression_options("zstd,three"), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::parse_compression_options("zstd,3 "), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::parse_compression_options("zfp,0"), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::parse_compression_options("zfp,-0.5"), std::invalid_argument);
    CHECK_THROWS_AS(fast_channel::parse_compression_options("zfp,nan"), std::invalid_argument);
}
