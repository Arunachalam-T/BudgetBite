// brute_force.cpp — Exhaustive enumeration

#include "brute_force.h"
#include "plan_evaluator.h"
#include "greedy.h"
#include <chrono>
#include <vector>
#include <functional>
#include <numeric>

AlgorithmResult BruteForce::solve(
    const Dataset&            dataset,
    const OptimizationConfig& config,
    OptimizationMode          mode,
    const ExecutionLimits&    limits)
{
    AlgorithmResult result;
    result.algorithm_name = name();

    auto start = std::chrono::steady_clock::now();
    auto time_limit = std::chrono::duration<double>(limits.max_time_seconds);

    const auto& slots_per_day = config.meal_slots();
    int n_days = 7;
    int n_slots = static_cast<int>(slots_per_day.size());

    // Pre-filter compatible recipes per slot type
    std::unordered_map<std::string, std::vector<std::string>> candidates; // meal_type -> [recipe_id]
    for (const auto& cr : dataset.calculated_recipes) {
        if (PlanEvaluator::recipe_compatible(cr, dataset, config))
            candidates[cr.meal_type].push_back(cr.recipe_id);
    }

    // Check each slot has candidates
    for (const auto& slot : slots_per_day) {
        if (candidates[slot].empty()) {
            result.status = ResultStatus::INFEASIBLE;
            result.termination_reason = "No compatible recipes for meal slot: " + slot;
            result.execution_time_ms = 0.0;
            return result;
        }
    }

    // Total slots = n_days * n_slots
    int total_slots = n_days * n_slots;
    std::vector<std::string> slot_types;
    slot_types.reserve(total_slots);
    for (int d = 0; d < n_days; ++d)
        for (const auto& s : slots_per_day)
            slot_types.push_back(s);

    // Candidate list for each position
    std::vector<std::vector<std::string>> slot_candidates(total_slots);
    for (int i = 0; i < total_slots; ++i)
        slot_candidates[i] = candidates[slot_types[i]];

    // Current assignment indices
    std::vector<int> indices(total_slots, 0);

    bool exhausted = false;
    bool interrupted = false;
    bool found_any = false;

    EvaluationResult best_eval;
    MealPlan         best_plan;

    // Primal Heuristic: Seed with Greedy heuristic so if search times out
    // on large instances, we return a valid feasible solution rather than failing.
    Greedy greedy;
    auto g_res = greedy.solve(dataset, config, mode, limits);
    if (g_res.is_feasible) {
        best_plan = g_res.best_plan;
        best_eval = g_res.evaluation;
        found_any = true;
    }

    // Iterate all combinations
    while (true) {
        // Check time limit
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= time_limit) {
            interrupted = true;
            result.termination_reason = "Time limit reached (" +
                std::to_string(limits.max_time_seconds) + "s)";
            break;
        }
        if (result.nodes_explored >= limits.max_nodes) {
            interrupted = true;
            result.termination_reason = "Node limit reached (" +
                std::to_string(limits.max_nodes) + " nodes)";
            break;
        }

        // Build plan from current indices
        MealPlan plan;
        for (int i = 0; i < total_slots; ++i) {
            MealSlotAssignment a;
            a.day        = i / n_slots;
            a.meal_type  = slot_types[i];
            a.recipe_id  = slot_candidates[i][indices[i]];
            a.servings   = 1;
            plan.assignments.push_back(a);
        }

        ++result.nodes_explored;

        // Evaluate
        auto eval = PlanEvaluator::evaluate(plan, dataset, config, mode);
        if (eval.is_feasible) {
            if (!found_any || PlanEvaluator::is_better(eval, best_eval, mode)) {
                best_eval = eval;
                best_plan = plan;
                found_any = true;
            }
        }

        // Advance indices (odometer)
        int pos = total_slots - 1;
        while (pos >= 0) {
            ++indices[pos];
            if (indices[pos] < static_cast<int>(slot_candidates[pos].size())) break;
            indices[pos] = 0;
            --pos;
        }
        if (pos < 0) { exhausted = true; break; }
    }

    auto end = std::chrono::steady_clock::now();
    result.execution_time_ms =
        std::chrono::duration<double, std::milli>(end - start).count();

    if (found_any) {
        result.best_plan    = best_plan;
        result.evaluation   = best_eval;
        result.is_feasible  = true;
        result.primary_objective   = best_eval.primary_objective_score;
        result.weekly_purchase_cost = best_eval.weekly_purchase_cost;
        result.nutrition_deviation = best_eval.nutrition_deviation_score;
        result.distinct_recipe_count = best_eval.distinct_recipe_count;

        if (exhausted && !interrupted) {
            result.status = ResultStatus::OPTIMAL;
            result.optimality_proven = true;
            result.termination_reason = "Exhaustive search completed.";
        } else {
            result.status = ResultStatus::FEASIBLE_NOT_PROVEN;
            result.optimality_proven = false;
        }
    } else {
        if (exhausted) {
            result.status = ResultStatus::INFEASIBLE;
            result.termination_reason = "No feasible plan exists (exhaustive search).";
        } else {
            result.status = ResultStatus::NO_FEASIBLE_FOUND;
        }
    }

    return result;
}
