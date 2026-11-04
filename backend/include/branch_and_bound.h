#pragma once
// branch_and_bound.h — Best-bound Branch and Bound using priority queue
#include "algorithm_interface.h"

class BranchAndBound : public IOptimizationAlgorithm {
public:
    AlgorithmResult solve(
        const Dataset&            dataset,
        const OptimizationConfig& config,
        OptimizationMode          mode,
        const ExecutionLimits&    limits = ExecutionLimits{}
    ) override;

    std::string name() const override { return "Branch and Bound"; }
};

