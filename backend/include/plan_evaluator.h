#pragma once
// plan_evaluator.h — Shared evaluator used by ALL algorithms

#include "models.h"
#include "optimization_config.h"

class PlanEvaluator {
public:
    // Evaluate a complete meal plan.
    // All five algorithms call this same function.
    static EvaluationResult evaluate(
        const MealPlan&           plan,
        const Dataset&            dataset,
        const OptimizationConfig& config,
        OptimizationMode          mode
    );

    // ─── Partial-state feasibility helpers (for search pruning) ──────────
    // These must never reject a partial state that could still become feasible.

    // Returns true if accumulated cost already exceeds budget.
    static bool budget_exceeded(double accumulated_cost, double weekly_budget);

    // Returns true if adding any more slots cannot possibly satisfy min_calories
    // given remaining slots and best-case per-slot nutrition.
    static bool calories_impossible(
        double accumulated_calories,
        double min_calories,
        int    remaining_slots,
        double best_possible_per_slot
    );

    // Returns true if accumulated fat already exceeds max.
    static bool fat_exceeded(double accumulated_fat, double max_fat);

    // Check recipe repetition limit
    static bool repetition_exceeded(
        const std::unordered_map<std::string, int>& counts,
        const std::string& recipe_id,
        int max_reps
    );

    // ─── Objective comparison ─────────────────────────────────────────────

    // Returns true if result_a is strictly better than result_b
    // under the given mode, using lexicographic tie-breaking.
    static bool is_better(
        const EvaluationResult& a,
        const EvaluationResult& b,
        OptimizationMode mode
    );

    // ─── Diet compatibility ───────────────────────────────────────────────

    // Returns true if ingredient's diet_category is allowed under diet_type.
    static bool ingredient_compatible(const std::string& diet_category, DietType diet_type);

    // Returns true if ALL ingredients in a recipe are compatible.
    static bool recipe_compatible(
        const CalculatedRecipe& recipe,
        const Dataset& dataset,
        const OptimizationConfig& config
    );

    // ─── Nutrition deviation ──────────────────────────────────────────────

    // Compute normalized deviation for one day.
    static double daily_nutrition_deviation(
        double cal, double prot, double carbs, double fat,
        const NutritionTargets& targets
    );

    // Compute aggregate deviation score (0..∞), lower is better.
    static double total_nutrition_deviation(
        const std::vector<double>& daily_cal,
        const std::vector<double>& daily_prot,
        const std::vector<double>& daily_carbs,
        const std::vector<double>& daily_fat,
        const NutritionTargets& targets
    );

private:
    static constexpr double EPSILON = 1e-9;
};

