// backtracking.cpp — Recursive backtracking with safe pruning

#include "backtracking.h"
#include "plan_evaluator.h"
#include <chrono>
#include <algorithm>
#include <limits>

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

// ─── Internal search state ────────────────────────────────────────────────

struct BtState {
    int slot_index = 0;
    int n_days;
    int n_slots_per_day;
    int total_slots;

    const std::vector<std::string>* slot_types;
    const std::unordered_map<std::string, std::vector<const CalculatedRecipe*>>* candidates;

    std::vector<MealSlotAssignment> assignments;
    std::unordered_map<std::string, int>    repetitions;
    std::unordered_map<std::string, double> ingredient_consumed;

    // Per-day accumulated nutrition (7 days)
    std::vector<double> day_calories;
    std::vector<double> day_protein;
    std::vector<double> day_carbs;
    std::vector<double> day_fat;

    double accumulated_cost = 0.0;

    // Best solution found
    MealPlan         best_plan;
    EvaluationResult best_eval;
    bool             found_any = false;

    // Metrics
    long long& nodes_explored;
    long long& branches_pruned;

    // Limits
    const ExecutionLimits& limits;
    TimePoint start_time;
    bool& interrupted;

    // Context
    const Dataset&            dataset;
    const OptimizationConfig& config;
    OptimizationMode          mode;

    // Max per-slot calories (for pruning lower bound check)
    double max_cal_per_slot = 0.0;
    double max_prot_per_slot = 0.0;

    BtState(int nd, int nspd, long long& ne, long long& bp,
            const ExecutionLimits& lim, TimePoint st, bool& intr,
            const Dataset& ds, const OptimizationConfig& cfg, OptimizationMode m)
        : n_days(nd), n_slots_per_day(nspd), total_slots(nd * nspd),
          nodes_explored(ne), branches_pruned(bp),
          limits(lim), start_time(st), interrupted(intr),
          dataset(ds), config(cfg), mode(m)
    {
        day_calories.assign(nd, 0.0);
        day_protein.assign(nd, 0.0);
        day_carbs.assign(nd, 0.0);
        day_fat.assign(nd, 0.0);
    }
};

// ─── Recursive backtrack ──────────────────────────────────────────────────

static void backtrack(BtState& state) {
    if (state.interrupted) return;

    // Check limits
    auto elapsed = Clock::now() - state.start_time;
    if (std::chrono::duration<double>(elapsed).count() >= state.limits.max_time_seconds ||
        state.nodes_explored >= state.limits.max_nodes)
    {
        state.interrupted = true;
        return;
    }

    // Base case: all slots filled
    if (state.slot_index == state.total_slots) {
        ++state.nodes_explored;
        // Build plan and evaluate
        MealPlan plan;
        plan.assignments = state.assignments;
        auto eval = PlanEvaluator::evaluate(plan, state.dataset, state.config, state.mode);
        if (eval.is_feasible) {
            if (!state.found_any ||
                PlanEvaluator::is_better(eval, state.best_eval, state.mode))
            {
                state.best_plan  = plan;
                state.best_eval  = eval;
                state.found_any  = true;
            }
        }
        return;
    }

    ++state.nodes_explored;

    int slot_idx = state.slot_index;
    int day      = slot_idx / state.n_slots_per_day;
    const std::string& meal_type = (*state.slot_types)[slot_idx];
    auto it = state.candidates->find(meal_type);
    if (it == state.candidates->end()) return;

    const auto& candidates = it->second;

    // Remaining slots after this one (for lower bound checks)
    int remaining_after = state.total_slots - slot_idx - 1;
    (void)remaining_after;

    for (const auto* cr : candidates) {
        if (state.interrupted) break;

        // ── Pruning: repetition exceeded ──────────────────────────────────
        if (PlanEvaluator::repetition_exceeded(
                state.repetitions, cr->recipe_id, state.config.max_recipe_repetitions)) {
            ++state.branches_pruned;
            continue;
        }

        // ── Pruning: per-day fat upper bound ─────────────────────────────
        double new_fat = state.day_fat[day] + cr->fat_per_serving;
        if (new_fat > state.config.constraints.max_fat + 1e-9) {
            ++state.branches_pruned;
            continue;
        }

        // ── Pruning: per-day calorie upper bound ──────────────────────────
        double new_cal = state.day_calories[day] + cr->calories_per_serving;
        // Only prune if this is the last slot of the day
        int slot_in_day = slot_idx % state.n_slots_per_day;
        bool last_slot_of_day = (slot_in_day == state.n_slots_per_day - 1);
        if (last_slot_of_day && new_cal > state.config.constraints.max_calories + 1e-9) {
            ++state.branches_pruned;
            continue;
        }

        // ── Pruning: early budget exceeded ────────────────────────────────
        // Use gross cost as conservative proxy (actual purchase cost <= gross cost)
        // Do not prune here — only prune based on purchase cost at evaluation

        // Apply choice
        MealSlotAssignment a;
        a.day       = day;
        a.meal_type = meal_type;
        a.recipe_id = cr->recipe_id;
        a.servings  = 1;

        state.assignments.push_back(a);
        state.repetitions[cr->recipe_id]++;
        state.day_calories[day] += cr->calories_per_serving;
        state.day_protein[day]  += cr->protein_per_serving;
        state.day_carbs[day]    += cr->carbs_per_serving;
        state.day_fat[day]      += cr->fat_per_serving;
        state.accumulated_cost  += cr->gross_cost_per_serving;

        for (const auto& ri : cr->ingredients)
            state.ingredient_consumed[ri.ingredient_id] += ri.quantity_g_per_serving;

        ++state.slot_index;

        // Recurse
        backtrack(state);

        // Undo
        --state.slot_index;
        state.assignments.pop_back();
        --state.repetitions[cr->recipe_id];
        if (state.repetitions[cr->recipe_id] == 0)
            state.repetitions.erase(cr->recipe_id);
        state.day_calories[day] -= cr->calories_per_serving;
        state.day_protein[day]  -= cr->protein_per_serving;
        state.day_carbs[day]    -= cr->carbs_per_serving;
        state.day_fat[day]      -= cr->fat_per_serving;
        state.accumulated_cost  -= cr->gross_cost_per_serving;

        for (const auto& ri : cr->ingredients) {
            state.ingredient_consumed[ri.ingredient_id] -= ri.quantity_g_per_serving;
            if (state.ingredient_consumed[ri.ingredient_id] < 1e-12)
                state.ingredient_consumed.erase(ri.ingredient_id);
        }
    }
}

// ─── Algorithm entry ──────────────────────────────────────────────────────

AlgorithmResult Backtracking::solve(
    const Dataset&            dataset,
    const OptimizationConfig& config,
    OptimizationMode          mode,
    const ExecutionLimits&    limits)
{
    AlgorithmResult result;
    result.algorithm_name = name();

    auto start_time = Clock::now();

    const auto& slots_per_day = config.meal_slots();
    int n_days  = 7;
    int n_slots = static_cast<int>(slots_per_day.size());
    int total   = n_days * n_slots;

    // Build slot_types array
    std::vector<std::string> slot_types;
    slot_types.reserve(total);
    for (int d = 0; d < n_days; ++d)
        for (const auto& s : slots_per_day)
            slot_types.push_back(s);

    // Compatible candidates
    std::unordered_map<std::string, std::vector<const CalculatedRecipe*>> candidates;
    for (const auto& cr : dataset.calculated_recipes)
        if (PlanEvaluator::recipe_compatible(cr, dataset, config))
            candidates[cr.meal_type].push_back(&cr);

    // Sort candidates deterministically (by recipe_id) for reproducibility
    for (auto& [mt, vec] : candidates)
        std::sort(vec.begin(), vec.end(),
                  [](const CalculatedRecipe* a, const CalculatedRecipe* b){
                      return a->recipe_id < b->recipe_id; });

    bool interrupted = false;
    BtState state(n_days, n_slots, result.nodes_explored, result.branches_pruned,
                  limits, start_time, interrupted, dataset, config, mode);
    state.slot_types = &slot_types;
    state.candidates = &candidates;

    backtrack(state);

    auto end_time = Clock::now();
    result.execution_time_ms =
        std::chrono::duration<double, std::milli>(end_time - start_time).count();

    if (state.found_any) {
        result.best_plan   = state.best_plan;
        result.evaluation  = state.best_eval;
        result.is_feasible = true;
        result.primary_objective     = state.best_eval.primary_objective_score;
        result.weekly_purchase_cost  = state.best_eval.weekly_purchase_cost;
        result.nutrition_deviation   = state.best_eval.nutrition_deviation_score;
        result.distinct_recipe_count = state.best_eval.distinct_recipe_count;

        if (!interrupted) {
            result.status             = ResultStatus::OPTIMAL;
            result.optimality_proven  = true;
            result.termination_reason = "Exhaustive backtracking completed.";
        } else {
            result.status             = ResultStatus::FEASIBLE_NOT_PROVEN;
            result.optimality_proven  = false;
            result.termination_reason = "Interrupted by time/node limit.";
        }
    } else {
        if (!interrupted) {
            result.status             = ResultStatus::INFEASIBLE;
            result.termination_reason = "Exhaustive search: no feasible plan exists.";
        } else {
            result.status             = ResultStatus::NO_FEASIBLE_FOUND;
            result.termination_reason = "Interrupted before feasible plan was found.";
        }
    }

    return result;
}

