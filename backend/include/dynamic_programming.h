#pragma once
// dynamic_programming.h — Exact state-based DP with memoization
#include "algorithm_interface.h"
#include <unordered_map>

class DynamicProgramming : public IOptimizationAlgorithm {
public:
    AlgorithmResult solve(
        const Dataset&            dataset,
        const OptimizationConfig& config,
        OptimizationMode          mode,
        const ExecutionLimits&    limits = ExecutionLimits{}
    ) override;

    std::string name() const override { return "Dynamic Programming"; }
};

