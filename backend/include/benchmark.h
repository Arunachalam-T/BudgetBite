#pragma once
// benchmark.h — Runs all five algorithms under identical conditions

#include "algorithm_interface.h"
#include <vector>
#include <memory>

struct BenchmarkResult {
    std::vector<AlgorithmResult> results; // one per algorithm, in order
    std::string optimization_mode;
    std::string dataset_name;
    double timestamp_unix = 0.0;
};

class Benchmark {
public:
    // Run all five algorithms with identical config, mode, and limits.
    static BenchmarkResult run_all(
        const Dataset&            dataset,
        const OptimizationConfig& config,
        OptimizationMode          mode,
        const ExecutionLimits&    limits = ExecutionLimits{}
    );
};

