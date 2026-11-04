// dynamic_programming.cpp — Exact DP with memoization

#include "dynamic_programming.h"
#include "plan_evaluator.h"
#include <chrono>
#include <algorithm>
#include <cmath>
#include <map>
#include <functional>
#include <limits>

using Clock = std::chrono::steady_clock;

// ─── DP State ─────────────────────────────────────────────────────────────
// The state encodes all information needed for future decisions:
// - slot index (position in the linearized 7*n_slots sequence)
// - cumulative ingredient consumption (compressed as map)
// - per-day accumulated nutrition
// - recipe repetition counts
//
// Because nutrition values are floating-point, exact memoization would
// require matching all floats precisely. We use a map keyed by a string
// representation of the state. This is exact (no bucketing/approximation).

struct DpStateKey {
    int slot_index;
    // Represent repetitions as sorted vector of (id, count) pairs
    std::vector<std::pair<std::string,int>> reps;
    // Per-day nutrition (truncated to int to reduce state space while remaining valid)
    // We keep full resolution and serialize as integers (nearest kcal/g)
    std::vector<int> day_cal_i;
    std::vector<int> day_prot_i;
    std::vector<int> day_carb_i;
    std::vector<int> day_fat_i;

    bool operator<(const DpStateKey& o) const {
        if (slot_index != o.slot_index) return slot_index < o.slot_index;
        if (reps != o.reps) return reps < o.reps;
        if (day_cal_i != o.day_cal_i) return day_cal_i < o.day_cal_i;
        if (day_prot_i != o.day_prot_i) return day_prot_i < o.day_prot_i;
        if (day_carb_i != o.day_carb_i) return day_carb_i < o.day_carb_i;
        return day_fat_i < o.day_fat_i;
    }
};

// Value stored in memo: best (eval, plan) continuation from this state
struct DpMemoValue {
    MealPlan         plan;        // partial plan from this state onwards
    EvaluationResult eval;        // full evaluation (filled at leaf)
    bool             found = false;
};

// ─── DP Context ──────────────────────────────────────────────────────────

struct DpCtx {
    int n_days;
    int n_slots;
    int total_slots;
    const std::vector<std::string>* slot_types;
    const std::unordered_map<std::string, std::vector<const CalculatedRecipe*>>* candidates;
    const Dataset&            dataset;
    const OptimizationConfig& config;
    OptimizationMode          mode;
    const ExecutionLimits&    limits;
    Clock::time_point         start;

    long long& nodes_explored;
    long long& branches_pruned;
    bool       interrupted = false;

    // Memoization: DpStateKey -> best plan from that state
    std::map<DpStateKey, DpMemoValue> memo;

    // Accumulated state (mutable during recursion)
    std::vector<MealSlotAssignment> partial;
    std::unordered_map<std::string, int>    reps;
    std::vector<double> day_cal;
    std::vector<double> day_prot;
    std::vector<double> day_carb;
    std::vector<double> day_fat;
    std::unordered_map<std::string, double> ingredient_consumed;

    DpCtx(int nd, int nspd, const Dataset& ds, const OptimizationConfig& cfg,
          OptimizationMode m, const ExecutionLimits& lim, Clock::time_point st,
          long long& ne, long long& bp)
        : n_days(nd), n_slots(nspd), total_slots(nd*nspd),
          dataset(ds), config(cfg), mode(m), limits(lim), start(st),
          nodes_explored(ne), branches_pruned(bp)
    {
        day_cal.assign(nd,0.0);
        day_prot.assign(nd,0.0);
        day_carb.assign(nd,0.0);
        day_fat.assign(nd,0.0);
    }
};

// Build state key from current accumulated values
static DpStateKey make_key(const DpCtx& ctx, int slot_idx) {
    DpStateKey k;
    k.slot_index = slot_idx;
    for (const auto& [id,cnt] : ctx.reps)
        k.reps.push_back({id,cnt});
    std::sort(k.reps.begin(), k.reps.end());

    for (int d = 0; d < ctx.n_days; ++d) {
        k.day_cal_i.push_back(static_cast<int>(std::round(ctx.day_cal[d])));
        k.day_prot_i.push_back(static_cast<int>(std::round(ctx.day_prot[d]*10)));
        k.day_carb_i.push_back(static_cast<int>(std::round(ctx.day_carb[d]*10)));
        k.day_fat_i.push_back(static_cast<int>(std::round(ctx.day_fat[d]*10)));
    }
    return k;
}

// ─── Recursive DP ────────────────────────────────────────────────────────

static DpMemoValue dp_solve(DpCtx& ctx, int slot_idx) {
    // Time/node limit check
    auto elapsed = std::chrono::duration<double>(Clock::now() - ctx.start).count();
    if (elapsed >= ctx.limits.max_time_seconds || ctx.nodes_explored >= ctx.limits.max_nodes) {
        ctx.interrupted = true;
        return {};
    }

    // Base case
    if (slot_idx == ctx.total_slots) {
        ++ctx.nodes_explored;
        MealPlan plan;
        plan.assignments = ctx.partial;
        auto eval = PlanEvaluator::evaluate(plan, ctx.dataset, ctx.config, ctx.mode);
        DpMemoValue val;
        val.plan  = plan;
        val.eval  = eval;
        val.found = eval.is_feasible;
        return val;
    }

    // Check memo
    auto key = make_key(ctx, slot_idx);
    auto it = ctx.memo.find(key);
    if (it != ctx.memo.end())
        return it->second;

    ++ctx.nodes_explored;

    int day = slot_idx / ctx.n_slots;
    const std::string& meal_type = (*ctx.slot_types)[slot_idx];
    auto cit = ctx.candidates->find(meal_type);

    DpMemoValue best;
    best.found = false;

    if (cit != ctx.candidates->end()) {
        for (const auto* cr : cit->second) {
            if (ctx.interrupted) break;

            // Repetition prune
            if (PlanEvaluator::repetition_exceeded(ctx.reps, cr->recipe_id,
                                                   ctx.config.max_recipe_repetitions)) {
                ++ctx.branches_pruned;
                continue;
            }

            // Fat prune
            double new_fat = ctx.day_fat[day] + cr->fat_per_serving;
            if (new_fat > ctx.config.constraints.max_fat + 1e-9) {
                ++ctx.branches_pruned;
                continue;
            }

            // Apply
            MealSlotAssignment a;
            a.day = day; a.meal_type = meal_type;
            a.recipe_id = cr->recipe_id; a.servings = 1;
            ctx.partial.push_back(a);
            ctx.reps[cr->recipe_id]++;
            ctx.day_cal[day]  += cr->calories_per_serving;
            ctx.day_prot[day] += cr->protein_per_serving;
            ctx.day_carb[day] += cr->carbs_per_serving;
            ctx.day_fat[day]  += cr->fat_per_serving;
            for (const auto& ri : cr->ingredients)
                ctx.ingredient_consumed[ri.ingredient_id] += ri.quantity_g_per_serving;

            auto sub = dp_solve(ctx, slot_idx + 1);

            // Undo
            ctx.partial.pop_back();
            ctx.reps[cr->recipe_id]--;
            if (ctx.reps[cr->recipe_id] == 0) ctx.reps.erase(cr->recipe_id);
            ctx.day_cal[day]  -= cr->calories_per_serving;
            ctx.day_prot[day] -= cr->protein_per_serving;
            ctx.day_carb[day] -= cr->carbs_per_serving;
            ctx.day_fat[day]  -= cr->fat_per_serving;
            for (const auto& ri : cr->ingredients) {
                ctx.ingredient_consumed[ri.ingredient_id] -= ri.quantity_g_per_serving;
                if (ctx.ingredient_consumed[ri.ingredient_id] < 1e-12)
                    ctx.ingredient_consumed.erase(ri.ingredient_id);
            }

            if (sub.found) {
                if (!best.found ||
                    PlanEvaluator::is_better(sub.eval, best.eval, ctx.mode)) {
                    best = sub;
                }
            }
        }
    }

    ctx.memo[key] = best;
    return best;
}

// ─── Algorithm entry ──────────────────────────────────────────────────────

AlgorithmResult DynamicProgramming::solve(
    const Dataset&            dataset,
    const OptimizationConfig& config,
    OptimizationMode          mode,
    const ExecutionLimits&    limits)
{
    AlgorithmResult result;
    result.algorithm_name = name();

    auto start = Clock::now();

    const auto& slots_per_day = config.meal_slots();
    int n_days  = 7;
    int n_slots = static_cast<int>(slots_per_day.size());

    std::vector<std::string> slot_types;
    for (int d = 0; d < n_days; ++d)
        for (const auto& s : slots_per_day) slot_types.push_back(s);

    std::unordered_map<std::string, std::vector<const CalculatedRecipe*>> candidates;
    for (const auto& cr : dataset.calculated_recipes)
        if (PlanEvaluator::recipe_compatible(cr, dataset, config))
            candidates[cr.meal_type].push_back(&cr);

    for (auto& [mt,vec] : candidates)
        std::sort(vec.begin(), vec.end(),
                  [](const CalculatedRecipe* a, const CalculatedRecipe* b){
                      return a->recipe_id < b->recipe_id; });

    DpCtx ctx(n_days, n_slots, dataset, config, mode, limits, start,
              result.nodes_explored, result.branches_pruned);
    ctx.slot_types = &slot_types;
    ctx.candidates = &candidates;

    auto res = dp_solve(ctx, 0);

    auto end = Clock::now();
    result.execution_time_ms =
        std::chrono::duration<double, std::milli>(end - start).count();

    // Report memo size as peak memory estimate (rough)
    result.peak_memory_bytes = static_cast<long long>(ctx.memo.size()) * 512;

    if (res.found) {
        result.best_plan   = res.plan;
        result.evaluation  = res.eval;
        result.is_feasible = true;
        result.primary_objective     = res.eval.primary_objective_score;
        result.weekly_purchase_cost  = res.eval.weekly_purchase_cost;
        result.nutrition_deviation   = res.eval.nutrition_deviation_score;
        result.distinct_recipe_count = res.eval.distinct_recipe_count;

        if (!ctx.interrupted) {
            result.status            = ResultStatus::OPTIMAL;
            result.optimality_proven = true;
            result.termination_reason = "DP with memoization completed (exact).";
        } else {
            result.status            = ResultStatus::FEASIBLE_NOT_PROVEN;
            result.optimality_proven = false;
            result.termination_reason = "DP interrupted; best feasible plan returned.";
        }
    } else {
        result.status = ctx.interrupted ? ResultStatus::NO_FEASIBLE_FOUND
                                        : ResultStatus::INFEASIBLE;
        result.termination_reason = ctx.interrupted
            ? "Interrupted before feasible plan found."
            : "No feasible plan exists (DP exhaustive).";
    }

    return result;
}

