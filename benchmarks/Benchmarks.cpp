#include "catch2/benchmark/catch_benchmark_all.hpp"
#include "catch2/catch_test_macros.hpp"

TEST_CASE ("Boot performance")
{
    BENCHMARK_ADVANCED ("Mock test")
    (Catch::Benchmark::Chronometer meter)
    {
        meter.measure ([&] (int /* i */) {
            // do nothing
        });
    };
}
