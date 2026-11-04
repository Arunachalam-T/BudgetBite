// plan_evaluator.cpp — Shared evaluator for all algorithms

#include "plan_evaluator.h"
#include <cmath>
#include <algorithm>
#include <unordered_set>

// ─── Diet compatibility ───────────────────────────────────────────────────

bool PlanEvaluator::ingredient_compatible(const std::string& cat, DietType diet) {
    switch (diet) {
        case DietType::VEGETARIAN:
            return cat == "Plant-based" || cat == "Dairy" || cat == "Eggs";
        case DietType::VEGAN:
            return cat == "Plant-based";
        case DietType::NON_VEGETARIAN:
        case DietType::UNRESTRICTED:
            return true;
        default:
            return true;
    }
}

bool PlanEvaluator::recipe_compatible(
    const CalculatedRecipe& recipe,
    const Dataset& dataset,
    const OptimizationConfig& config)
{
    // Build excluded set
    std::unordered_set<std::string> excl(config.excluded_ingredient_ids.begin(),
                                         config.excluded_ingredient_ids.end());

    for (const auto& ri : recipe.ingredients) {
        // Excluded ingredient check
        if (excl.count(ri.ingredient_id)) return false;

        // Diet check
        auto it = dataset.ingredient_map.find(ri.ingredient_id);
        if (it == dataset.ingredient_map.end()) continue;
        if (!ingredient_compatible(it->second.diet_category, config.diet_type)) return false;
    }
    return true;
}

// ─── Nutrition deviation ──────────────────────────────────────────────────

double PlanEvaluator::daily_nutrition_deviation(
    double cal, double prot, double carbs, double fat,
    const NutritionTargets& t)
{
    double d = 0.0;
    d += std::abs(cal   - t.target_calories) / std::max(t.target_calories, EPSILON);
    d += std::abs(prot  - t.target_protein)  / std::max(t.target_protein,  EPSILON);
    d += std::abs(carbs - t.target_carbs)    / std::max(t.target_carbs,    EPSILON);
    d += std::abs(fat   - t.target_fat)      / std::max(t.target_fat,      EPSILON);
    return d / 4.0;  // average over 4 nutrients
}

double PlanEvaluator::total_nutrition_deviation(
    const std::vector<double>& daily_cal,
    const std::vector<double>& daily_prot,
    const std::vector<double>& daily_carbs,
    const std::vector<double>& daily_fat,
    const NutritionTargets& t)
{
    double total = 0.0;
    int n = static_cast<int>(daily_cal.size());
    for (int d = 0; d < n; ++d)
        total += daily_nutrition_deviation(
            daily_cal[d], daily_prot[d], daily_carbs[d], daily_fat[d], t);
    return n > 0 ? total / n : 0.0;
}

// ─── Partial-state helpers ────────────────────────────────────────────────

bool PlanEvaluator::budget_exceeded(double cost, double budget) {
    return cost > budget + EPSILON;
}

bool PlanEvaluator::calories_impossible(
    double accum, double min_cal, int remaining_slots, double best_per_slot)
{
    double max_achievable = accum + remaining_slots * best_per_slot;
    return max_achievable < min_cal - EPSILON;
}

bool PlanEvaluator::fat_exceeded(double fat, double max_fat) {
    return fat > max_fat + EPSILON;
}

bool PlanEvaluator::repetition_exceeded(
    const std::unordered_map<std::string, int>& counts,
    const std::string& recipe_id, int max_reps)
{
    auto it = counts.find(recipe_id);
    return it != counts.end() && it->second >= max_reps;
}

// ─── Full plan evaluation ─────────────────────────────────────────────────

EvaluationResult PlanEvaluator::evaluate(
    const MealPlan&           plan,
    const Dataset&            dataset,
    const OptimizationConfig& config,
    OptimizationMode          mode)
{
    EvaluationResult res;
    const auto& slots = config.meal_slots();
    int n_days = 7;

    res.daily_calories.assign(n_days, 0.0);
    res.daily_protein.assign(n_days, 0.0);
    res.daily_carbs.assign(n_days, 0.0);
    res.daily_fat.assign(n_days, 0.0);
    res.daily_nutrition_deviation.assign(n_days, 0.0);

    std::unordered_map<std::string, double> ingredient_consumed; // id -> total g
    std::unordered_map<std::string, int>    recipe_reps;         // id -> count

    // Validate slot count
    int expected = n_days * static_cast<int>(slots.size());
    if (static_cast<int>(plan.assignments.size()) != expected) {
        res.is_feasible = false;
        res.violation_messages.push_back(
            "Plan has " + std::to_string(plan.assignments.size()) +
            " assignments; expected " + std::to_string(expected));
        return res;
    }

    // Accumulate daily nutrition and ingredient consumption
    for (const auto& a : plan.assignments) {
        auto it = dataset.calc_recipe_map.find(a.recipe_id);
        if (it == dataset.calc_recipe_map.end()) {
            res.violation_messages.push_back("Unknown recipe in plan: " + a.recipe_id);
            res.is_feasible = false;
            return res;
        }
        const CalculatedRecipe& cr = it->second;

        // Check meal type compatibility
        if (cr.meal_type != a.meal_type) {
            res.violation_messages.push_back(
                "Recipe " + a.recipe_id + " (" + cr.meal_type +
                ") assigned to incompatible slot " + a.meal_type);
        }

        // Diet and excluded ingredient check
        if (!recipe_compatible(cr, dataset, config)) {
            res.violation_messages.push_back(
                "Recipe " + a.recipe_id + " violates diet/exclusion constraint.");
        }

        int d = a.day;
        int servings = a.servings;
        res.daily_calories[d] += cr.calories_per_serving * servings;
        res.daily_protein[d]  += cr.protein_per_serving  * servings;
        res.daily_carbs[d]    += cr.carbs_per_serving    * servings;
        res.daily_fat[d]      += cr.fat_per_serving      * servings;
        res.gross_recipe_cost += cr.gross_cost_per_serving * servings;

        recipe_reps[a.recipe_id] += servings;

        // Ingredient consumption
        for (const auto& ri : cr.ingredients) {
            ingredient_consumed[ri.ingredient_id] += ri.quantity_g_per_serving * servings;
        }
    }

    res.recipe_repetitions = recipe_reps;

    // Count distinct recipes
    std::unordered_set<std::string> distinct;
    for (const auto& a : plan.assignments) distinct.insert(a.recipe_id);
    res.distinct_recipe_count = static_cast<int>(distinct.size());

    // Weekly totals
    for (int d = 0; d < n_days; ++d) {
        res.weekly_calories += res.daily_calories[d];
        res.weekly_protein  += res.daily_protein[d];
        res.weekly_carbs    += res.daily_carbs[d];
        res.weekly_fat      += res.daily_fat[d];
    }

    // Hard constraint checks
    bool feasible = res.violation_messages.empty();

    for (int d = 0; d < n_days; ++d) {
        if (res.daily_calories[d] < config.constraints.min_calories - EPSILON) {
            feasible = false;
            res.violation_messages.push_back(
                "Day " + std::to_string(d+1) + ": calories " +
                std::to_string(res.daily_calories[d]) + " < min " +
                std::to_string(config.constraints.min_calories));
        }
        if (res.daily_calories[d] > config.constraints.max_calories + EPSILON) {
            feasible = false;
            res.violation_messages.push_back(
                "Day " + std::to_string(d+1) + ": calories " +
                std::to_string(res.daily_calories[d]) + " > max " +
                std::to_string(config.constraints.max_calories));
        }
        if (res.daily_protein[d] < config.constraints.min_protein - EPSILON) {
            feasible = false;
            res.violation_messages.push_back(
                "Day " + std::to_string(d+1) + ": protein " +
                std::to_string(res.daily_protein[d]) + " < min " +
                std::to_string(config.constraints.min_protein));
        }
        if (res.daily_carbs[d] < config.constraints.min_carbs - EPSILON) {
            feasible = false;
            res.violation_messages.push_back(
                "Day " + std::to_string(d+1) + ": carbs " +
                std::to_string(res.daily_carbs[d]) + " < min " +
                std::to_string(config.constraints.min_carbs));
        }
        if (res.daily_fat[d] > config.constraints.max_fat + EPSILON) {
            feasible = false;
            res.violation_messages.push_back(
                "Day " + std::to_string(d+1) + ": fat " +
                std::to_string(res.daily_fat[d]) + " > max " +
                std::to_string(config.constraints.max_fat));
        }
    }

    // Recipe repetition check
    for (const auto& [rid, cnt] : recipe_reps) {
        if (cnt > config.max_recipe_repetitions) {
            feasible = false;
            res.violation_messages.push_back(
                "Recipe " + rid + " repeated " + std::to_string(cnt) +
                " times; max is " + std::to_string(config.max_recipe_repetitions));
        }
    }

    // Grocery calculation
    res.ingredient_consumption = ingredient_consumed;
    double weekly_purchase_cost = 0.0;
    for (const auto& [ing_id, total_g] : ingredient_consumed) {
        double available_g = 0.0;
        auto inv_it = dataset.inventory_map.find(ing_id);
        if (inv_it != dataset.inventory_map.end())
            available_g = inv_it->second;

        double purchase_g = std::max(0.0, total_g - available_g);
        res.ingredient_purchased[ing_id] = purchase_g;

        auto ing_it = dataset.ingredient_map.find(ing_id);
        if (ing_it != dataset.ingredient_map.end()) {
            double cost = (purchase_g / 1000.0) * ing_it->second.price_per_kg_inr;
            weekly_purchase_cost += cost;

            if (purchase_g > 0.0 || total_g > 0.0) {
                GroceryItem gi;
                gi.ingredient_id    = ing_id;
                gi.ingredient_name  = ing_it->second.name;
                gi.total_required_g = total_g;
                gi.available_g      = available_g;
                gi.purchase_g       = purchase_g;
                gi.price_per_kg_inr = ing_it->second.price_per_kg_inr;
                gi.purchase_cost_inr= cost;
                res.grocery_list.push_back(gi);
            }
        }
    }
    res.weekly_purchase_cost = weekly_purchase_cost;

    // Budget check
    if (weekly_purchase_cost > config.weekly_budget_inr + EPSILON) {
        feasible = false;
        res.violation_messages.push_back(
            "Weekly purchase cost " + std::to_string(weekly_purchase_cost) +
            " exceeds budget " + std::to_string(config.weekly_budget_inr));
    }

    res.is_feasible = feasible;

    // Objective scores
    res.nutrition_deviation_score = total_nutrition_deviation(
        res.daily_calories, res.daily_protein, res.daily_carbs, res.daily_fat,
        config.targets);

    int total_slots = n_days * static_cast<int>(slots.size());
    res.variety_score = total_slots > 0
        ? 100.0 * res.distinct_recipe_count / total_slots
        : 0.0;

    for (int d = 0; d < n_days; ++d)
        res.daily_nutrition_deviation[d] = daily_nutrition_deviation(
            res.daily_calories[d], res.daily_protein[d],
            res.daily_carbs[d], res.daily_fat[d],
            config.targets);

    switch (mode) {
        case OptimizationMode::NUTRITION:
            res.primary_objective_score = res.nutrition_deviation_score;
            break;
        case OptimizationMode::BUDGET:
            res.primary_objective_score = res.weekly_purchase_cost;
            break;
        case OptimizationMode::VARIETY:
            res.primary_objective_score = static_cast<double>(res.distinct_recipe_count);
            break;
    }

    return res;
}

// ─── Objective comparison ─────────────────────────────────────────────────

bool PlanEvaluator::is_better(
    const EvaluationResult& a,
    const EvaluationResult& b,
    OptimizationMode mode)
{
    // Only compare feasible plans
    if (!a.is_feasible) return false;
    if (!b.is_feasible) return true;

    const double TOL = 1e-9;

    double pa = a.primary_objective_score;
    double pb = b.primary_objective_score;

    bool minimize = (mode != OptimizationMode::VARIETY);

    // Primary objective
    if (minimize) {
        if (pa < pb - TOL) return true;
        if (pa > pb + TOL) return false;
    } else {
        if (pa > pb + TOL) return true;
        if (pa < pb - TOL) return false;
    }

    // Tie-break 1: prefer lower weekly purchase cost
    if (a.weekly_purchase_cost < b.weekly_purchase_cost - TOL) return true;
    if (a.weekly_purchase_cost > b.weekly_purchase_cost + TOL) return false;

    // Tie-break 2: prefer lower nutrition deviation
    if (a.nutrition_deviation_score < b.nutrition_deviation_score - TOL) return true;
    if (a.nutrition_deviation_score > b.nutrition_deviation_score + TOL) return false;

    return false; // equal
}

