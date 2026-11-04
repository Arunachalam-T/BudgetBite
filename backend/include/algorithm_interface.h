#pragma once
// algorithm_interface.h — Common interface and result structure for all algorithms

#include "models.h"
#include "optimization_config.h"
#include <string>
#include <chrono>

// ─── Algorithm result status ──────────────────────────────────────────────
enum class ResultStatus {
    OPTIMAL,              // proven optimal solution found
    FEASIBLE_NOT_PROVEN,  // feasible solution found, not proven optimal
    NO_FEASIBLE_FOUND,    // no feasible solution found (infeasibility not proven)
    INFEASIBLE,           // exhaustive proof that no feasible solution exists
    INTERRUPTED           // cut short by time/node limit
};

inline std::string result_status_to_string(ResultStatus s) {
    switch (s) {
        case ResultStatus::OPTIMAL:             return "Optimal";
        case ResultStatus::FEASIBLE_NOT_PROVEN: return "Feasible (not proven optimal)";
        case ResultStatus::NO_FEASIBLE_FOUND:   return "No feasible solution found";
        case ResultStatus::INFEASIBLE:          return "Infeasible";
        case ResultStatus::INTERRUPTED:         return "Interrupted";
        default:                                return "Unknown";
    }
}

// ─── Common algorithm result ──────────────────────────────────────────────
struct AlgorithmResult {
    std::string      algorithm_name;
    MealPlan         best_plan;
    EvaluationResult evaluation;

    ResultStatus status          = ResultStatus::NO_FEASIBLE_FOUND;
    bool         is_feasible     = false;
    bool         optimality_proven = false;

    // Metrics
    double execution_time_ms     = 0.0;
    long long nodes_explored     = 0;
    long long branches_pruned    = 0;
    long long peak_memory_bytes  = 0;   // algorithm-owned state memory estimate

    // Objective details
    double primary_objective     = 0.0;
    double weekly_purchase_cost  = 0.0;
    double nutrition_deviation   = 0.0;
    int    distinct_recipe_count = 0;

    // Bounds & gap (for exact algorithms)
    double lower_bound           = 0.0;  // for minimization problems
    double upper_bound           = 0.0;  // for maximization problems
    double optimality_gap_pct    = -1.0; // -1 means "not available"

    std::string termination_reason;
};

// ─── Execution limits (used by all algorithms) ───────────────────────────
struct ExecutionLimits {
    double    max_time_seconds  = 60.0;
    long long max_nodes         = 10'000'000LL;
};

// ─── Algorithm interface ──────────────────────────────────────────────────
class IOptimizationAlgorithm {
public:
    virtual AlgorithmResult solve(
        const Dataset&            dataset,
        const OptimizationConfig& config,
        OptimizationMode          mode,
        const ExecutionLimits&    limits = ExecutionLimits{}
    ) = 0;

    virtual std::string name() const = 0;
    virtual ~IOptimizationAlgorithm() = default;
};

