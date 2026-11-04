// greedy.cpp — Deterministic greedy heuristic

#include "greedy.h"
#include "plan_evaluator.h"
#include <chrono>
#include <algorithm>
#include <limits>
#include <unordered_set>

// ─── Local scoring for a candidate recipe ────────────────────────────────

double Greedy::score_candidate(
    const CalculatedRecipe&   recipe,
    const OptimizationConfig& config,
    OptimizationMode          mode,
    double accumulated_cost,
    double day_calories,
    double day_protein,
    double day_carbs,
    double day_fat,
    const std::unordered_map<std::string, int>& repetitions,
    const std::unordered_map<std::string, double>& ingredient_consumed,
    const Dataset& dataset) const
{
    (void)accumulated_cost;
    switch (mode) {
        case OptimizationMode::NUTRITION: {
            // Marginal deviation improvement: how much closer to daily target
            // does adding this recipe get us?
            double new_cal  = day_calories + recipe.calories_per_serving;
            double new_prot = day_protein  + recipe.protein_per_serving;
            double new_carb = day_carbs    + recipe.carbs_per_serving;
            double new_fat  = day_fat      + recipe.fat_per_serving;

            const auto& t = config.targets;
            const double eps = 1e-9;
            double dev =
                std::abs(new_cal  - t.target_calories) / std::max(t.target_calories, eps) +
                std::abs(new_prot - t.target_protein)  / std::max(t.target_protein,  eps) +
                std::abs(new_carb - t.target_carbs)    / std::max(t.target_carbs,    eps) +
                std::abs(new_fat  - t.target_fat)      / std::max(t.target_fat,      eps);
            return -dev; // higher is better (less deviation)
        }
        case OptimizationMode::BUDGET: {
            // Calculate incremental purchase cost with current inventory
            double marginal_cost = 0.0;
            for (const auto& ri : recipe.ingredients) {
                double consumed_so_far = 0.0;
                auto it = ingredient_consumed.find(ri.ingredient_id);
                if (it != ingredient_consumed.end())
                    consumed_so_far = it->second;

                double inv = 0.0;
                auto inv_it = dataset.inventory_map.find(ri.ingredient_id);
                if (inv_it != dataset.inventory_map.end())
                    inv = inv_it->second;

                double already_purchased = std::max(0.0, consumed_so_far - inv);
                double new_required      = consumed_so_far + ri.quantity_g_per_serving;
                double new_purchased     = std::max(0.0, new_required - inv);
                double marginal_g        = new_purchased - already_purchased;

                auto ing_it = dataset.ingredient_map.find(ri.ingredient_id);
                if (ing_it != dataset.ingredient_map.end())
                    marginal_cost += (marginal_g / 1000.0) * ing_it->second.price_per_kg_inr;
            }
            return -marginal_cost; // higher is better (lower cost)
        }
        case OptimizationMode::VARIETY: {
            auto rep_it = repetitions.find(recipe.recipe_id);
            int reps = (rep_it != repetitions.end()) ? rep_it->second : 0;
            // Prefer unselected (reps=0) recipes strongly, then lower reps
            return -static_cast<double>(reps);
        }
    }
    return 0.0;
}

// ─── Main solve ───────────────────────────────────────────────────────────

AlgorithmResult Greedy::solve(
    const Dataset&            dataset,
    const OptimizationConfig& config,
    OptimizationMode          mode,
    const ExecutionLimits&    limits)
{
    (void)limits;
    AlgorithmResult result;
    result.algorithm_name = name();

    auto start = std::chrono::steady_clock::now();

    const auto& slots_per_day = config.meal_slots();
    int n_days = 7;
    int n_slots = static_cast<int>(slots_per_day.size());

    // Pre-filter compatible recipes per meal type
    std::unordered_map<std::string, std::vector<const CalculatedRecipe*>> candidates;
    for (const auto& cr : dataset.calculated_recipes) {
        if (PlanEvaluator::recipe_compatible(cr, dataset, config))
            candidates[cr.meal_type].push_back(&cr);
    }

    MealPlan plan;
    std::unordered_map<std::string, int>    repetitions;
    std::unordered_map<std::string, double> ingredient_consumed;
    double accumulated_cost = 0.0;
    bool failed = false;

    for (int d = 0; d < n_days && !failed; ++d) {
        double day_cal = 0, day_prot = 0, day_carb = 0, day_fat = 0;

        for (int s = 0; s < n_slots && !failed; ++s) {
            const std::string& slot = slots_per_day[s];
            auto& slot_candidates = candidates[slot];

            if (slot_candidates.empty()) {
                failed = true;
                result.termination_reason = "No compatible recipes for slot: " + slot;
                break;
            }

            // Filter: exclude if max_reps exceeded
            const CalculatedRecipe* best = nullptr;
            double best_score = -std::numeric_limits<double>::infinity();

            for (const auto* cr : slot_candidates) {
                if (PlanEvaluator::repetition_exceeded(repetitions, cr->recipe_id,
                                                       config.max_recipe_repetitions))
                    continue;

                double sc = score_candidate(
                    *cr, config, mode,
                    accumulated_cost, day_cal, day_prot, day_carb, day_fat,
                    repetitions, ingredient_consumed, dataset);

                // Deterministic tie-break: prefer lexicographically smaller recipe_id
                if (best == nullptr || sc > best_score + 1e-12 ||
                    (std::abs(sc - best_score) < 1e-12 && cr->recipe_id < best->recipe_id)) {
                    best = cr;
                    best_score = sc;
                }
            }

            if (!best) {
                // All candidates exhausted repetition limit — relax and take lowest reps
                for (const auto* cr : slot_candidates) {
                    double sc = score_candidate(
                        *cr, config, mode,
                        accumulated_cost, day_cal, day_prot, day_carb, day_fat,
                        repetitions, ingredient_consumed, dataset);
                    if (best == nullptr || sc > best_score + 1e-12 ||
                        (std::abs(sc - best_score) < 1e-12 && cr->recipe_id < best->recipe_id)) {
                        best = cr;
                        best_score = sc;
                    }
                }
            }

            if (!best) {
                failed = true;
                result.termination_reason = "No candidate available for slot: " + slot;
                break;
            }

            // Assign
            MealSlotAssignment a;
            a.day       = d;
            a.meal_type = slot;
            a.recipe_id = best->recipe_id;
            a.servings  = 1;
            plan.assignments.push_back(a);

            // Update state
            repetitions[best->recipe_id]++;
            day_cal  += best->calories_per_serving;
            day_prot += best->protein_per_serving;
            day_carb += best->carbs_per_serving;
            day_fat  += best->fat_per_serving;

            for (const auto& ri : best->ingredients)
                ingredient_consumed[ri.ingredient_id] += ri.quantity_g_per_serving;

            accumulated_cost += best->gross_cost_per_serving;
            ++result.nodes_explored;
        }
    }

    auto end = std::chrono::steady_clock::now();
    result.execution_time_ms =
        std::chrono::duration<double, std::milli>(end - start).count();

    if (!failed) {
        auto eval = PlanEvaluator::evaluate(plan, dataset, config, mode);
        result.best_plan   = plan;
        result.evaluation  = eval;
        result.is_feasible = eval.is_feasible;
        result.primary_objective     = eval.primary_objective_score;
        result.weekly_purchase_cost  = eval.weekly_purchase_cost;
        result.nutrition_deviation   = eval.nutrition_deviation_score;
        result.distinct_recipe_count = eval.distinct_recipe_count;

        // Greedy never claims optimality
        result.status             = ResultStatus::FEASIBLE_NOT_PROVEN;
        result.optimality_proven  = false;
        result.termination_reason = "Greedy heuristic completed (optimality not guaranteed).";
    } else {
        result.status = ResultStatus::NO_FEASIBLE_FOUND;
    }

    return result;
}

