#pragma once
// greedy.h — Deterministic greedy heuristic
#include "algorithm_interface.h"

class Greedy : public IOptimizationAlgorithm {
public:
    AlgorithmResult solve(
        const Dataset&            dataset,
        const OptimizationConfig& config,
        OptimizationMode          mode,
        const ExecutionLimits&    limits = ExecutionLimits{}
    ) override;

    std::string name() const override { return "Greedy"; }

private:
    // Score a candidate recipe for the current slot under the given mode.
    double score_candidate(
        const CalculatedRecipe&   recipe,
        const OptimizationConfig& config,
        OptimizationMode          mode,
        // accumulated state
        double accumulated_cost,
        double day_calories,
        double day_protein,
        double day_carbs,
        double day_fat,
        const std::unordered_map<std::string, int>& repetitions,
        const std::unordered_map<std::string, double>& ingredient_consumed,
        const Dataset& dataset
    ) const;
};

