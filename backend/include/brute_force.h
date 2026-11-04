#pragma once
// brute_force.h — Exhaustive enumeration algorithm
#include "algorithm_interface.h"

class BruteForce : public IOptimizationAlgorithm {
public:
    AlgorithmResult solve(
        const Dataset&            dataset,
        const OptimizationConfig& config,
        OptimizationMode          mode,
        const ExecutionLimits&    limits = ExecutionLimits{}
    ) override;

    std::string name() const override { return "Brute Force"; }
};

