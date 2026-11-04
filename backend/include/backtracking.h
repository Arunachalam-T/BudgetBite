#pragma once
// backtracking.h — Recursive backtracking with constraint pruning
#include "algorithm_interface.h"

class Backtracking : public IOptimizationAlgorithm {
public:
    AlgorithmResult solve(
        const Dataset&            dataset,
        const OptimizationConfig& config,
        OptimizationMode          mode,
        const ExecutionLimits&    limits = ExecutionLimits{}
    ) override;

    std::string name() const override { return "Backtracking"; }
};

