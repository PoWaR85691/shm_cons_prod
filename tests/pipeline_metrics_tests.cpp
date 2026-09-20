#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "pipeline/producer_pipeline.hpp"

TEST_CASE("compression percent reports the saved share") {
    fast_channel::PipelineMetrics metrics;
    metrics.original_bytes = 1000;
    metrics.transmitted_bytes = 250;

    CHECK(fast_channel::compression_percent(metrics) == doctest::Approx(75.0));
}

TEST_CASE("compression percent is negative when data grows") {
    fast_channel::PipelineMetrics metrics;
    metrics.original_bytes = 1000;
    metrics.transmitted_bytes = 1250;

    CHECK(fast_channel::compression_percent(metrics) == doctest::Approx(-25.0));
}

TEST_CASE("compression percent is zero without input") {
    const fast_channel::PipelineMetrics metrics{};

    CHECK(fast_channel::compression_percent(metrics) == 0.0);
}
