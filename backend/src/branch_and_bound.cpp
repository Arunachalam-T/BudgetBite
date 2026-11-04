// branch_and_bound.cpp — Best-first Branch and Bound with priority queue

#include "branch_and_bound.h"
#include "plan_evaluator.h"
#include <chrono>
#include <queue>
#include <algorithm>
#include <limits>
#include <cmath>

using Clock = std::chrono::steady_clock;

// ─── Search node ──────────────────────────────────────────────────────────

struct BnBNode {
    int slot_index = 0;

    std::vector<MealSlotAssignment> partial;
    std::unordered_map<std::string, int>    reps;
    std::unordered_map<std::string, double> ingredient_consumed;

    std::vector<double> day_cal;
    std::vector<double> day_prot;
    std::vector<double> day_carb;
    std::vector<double> day_fat;

    double accumulated_gross_cost = 0.0;

    // Optimistic bound for priority queue ordering
    double bound = 0.0; // for min: lower is better; for max: higher is better
};

// ─── Bound computation ────────────────────────────────────────────────────
// Compute an optimistic bound (admissible, i.e., never worse than actual best).

static double compute_bound(
    const BnBNode& node,
    int total_slots,
    const std::vector<std::string>& slot_types,
    const std::unordered_map<std::string, std::vector<const CalculatedRecipe*>>& candidates,
    const Dataset& dataset,
    const OptimizationConfig& config,
    OptimizationMode mode)
{
    (void)total_slots;
    (void)slot_types;
    (void)candidates;
    (void)dataset;
    (void)config;
    switch (mode) {
        case OptimizationMode::BUDGET: {
            // Lower bound: current gross cost (purchase cost <= gross cost)
            // This is a valid lower bound because inventory only reduces cost.
            return node.accumulated_gross_cost;
        }
        case OptimizationMode::NUTRITION: {
            // Lower bound on deviation: 0 (optimistically, remaining slots could
            // perfectly match targets). This is admissible but weak.
            // Better: compute current partial deviation for filled slots.
            return 0.0; // admissible lower bound
        }
        case OptimizationMode::VARIETY: {
            // Upper bound on distinct recipes:
            // current distinct + remaining slots (assuming all new recipes)
            std::unordered_set<std::string> distinct;
            for (const auto& a : node.partial) distinct.insert(a.recipe_id);
            int remaining = total_slots - node.slot_index;
            return static_cast<double>(distinct.size() + remaining);
        }
    }
    return 0.0;
}

// Node comparison for priority queue
// For minimization: lower bound = higher priority
// For maximization: higher bound = higher priority
struct NodeCmp {
    OptimizationMode mode;
    bool operator()(const BnBNode& a, const BnBNode& b) const {
        if (mode == OptimizationMode::VARIETY)
            return a.bound < b.bound; // max-heap on bound
        return a.bound > b.bound;     // min-heap on bound
    }
};

// ─── Algorithm entry ──────────────────────────────────────────────────────

AlgorithmResult BranchAndBound::solve(
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
    int total   = n_days * n_slots;

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

    // Incumbent (best known feasible)
    MealPlan         incumbent_plan;
    EvaluationResult incumbent_eval;
    bool             found_any = false;

    // Priority queue (best-first)
    NodeCmp cmp{mode};
    std::priority_queue<BnBNode, std::vector<BnBNode>, NodeCmp> pq(cmp);

    // Root node
    BnBNode root;
    root.slot_index = 0;
    root.day_cal.assign(n_days, 0.0);
    root.day_prot.assign(n_days, 0.0);
    root.day_carb.assign(n_days, 0.0);
    root.day_fat.assign(n_days, 0.0);
    root.bound = compute_bound(root, total, slot_types, candidates, dataset, config, mode);
    pq.push(root);

    bool interrupted = false;

    while (!pq.empty() && !interrupted) {
        // Time/node check
        auto elapsed = std::chrono::duration<double>(Clock::now() - start).count();
        if (elapsed >= limits.max_time_seconds || result.nodes_explored >= limits.max_nodes) {
            interrupted = true;
            break;
        }

        BnBNode node = pq.top();
        pq.pop();
        ++result.nodes_explored;

        // Prune: can this node beat the incumbent?
        if (found_any) {
            bool prune = false;
            if (mode == OptimizationMode::VARIETY) {
                // Prune if upper bound <= incumbent distinct count
                if (node.bound <= incumbent_eval.distinct_recipe_count) prune = true;
            } else {
                // Prune if lower bound >= incumbent objective
                if (node.bound >= incumbent_eval.primary_objective_score - 1e-9) prune = true;
            }
            if (prune) { ++result.branches_pruned; continue; }
        }

        // Leaf node
        if (node.slot_index == total) {
            MealPlan plan;
            plan.assignments = node.partial;
            auto eval = PlanEvaluator::evaluate(plan, dataset, config, mode);
            if (eval.is_feasible) {
                if (!found_any || PlanEvaluator::is_better(eval, incumbent_eval, mode)) {
                    incumbent_plan = plan;
                    incumbent_eval = eval;
                    found_any = true;
                }
            }
            continue;
        }

        int day = node.slot_index / n_slots;
        const std::string& meal_type = slot_types[node.slot_index];
        auto cit = candidates.find(meal_type);
        if (cit == candidates.end()) continue;

        for (const auto* cr : cit->second) {
            if (interrupted) break;

            // Repetition prune
            if (PlanEvaluator::repetition_exceeded(node.reps, cr->recipe_id,
                                                   config.max_recipe_repetitions)) {
                ++result.branches_pruned;
                continue;
            }
            // Fat prune
            if (node.day_fat[day] + cr->fat_per_serving > config.constraints.max_fat + 1e-9) {
                ++result.branches_pruned;
                continue;
            }

            // Build child
            BnBNode child = node;
            MealSlotAssignment a;
            a.day = day; a.meal_type = meal_type;
            a.recipe_id = cr->recipe_id; a.servings = 1;
            child.partial.push_back(a);
            child.reps[cr->recipe_id]++;
            child.day_cal[day]  += cr->calories_per_serving;
            child.day_prot[day] += cr->protein_per_serving;
            child.day_carb[day] += cr->carbs_per_serving;
            child.day_fat[day]  += cr->fat_per_serving;
            child.accumulated_gross_cost += cr->gross_cost_per_serving;
            for (const auto& ri : cr->ingredients)
                child.ingredient_consumed[ri.ingredient_id] += ri.quantity_g_per_serving;
            child.slot_index = node.slot_index + 1;

            child.bound = compute_bound(child, total, slot_types, candidates,
                                        dataset, config, mode);

            // Prune child by bound
            if (found_any) {
                bool prune = false;
                if (mode == OptimizationMode::VARIETY)
                    prune = child.bound <= incumbent_eval.distinct_recipe_count;
                else
                    prune = child.bound >= incumbent_eval.primary_objective_score - 1e-9;
                if (prune) { ++result.branches_pruned; continue; }
            }

            pq.push(child);
        }
    }

    auto end = Clock::now();
    result.execution_time_ms =
        std::chrono::duration<double, std::milli>(end - start).count();

    if (found_any) {
        result.best_plan   = incumbent_plan;
        result.evaluation  = incumbent_eval;
        result.is_feasible = true;
        result.primary_objective     = incumbent_eval.primary_objective_score;
        result.weekly_purchase_cost  = incumbent_eval.weekly_purchase_cost;
        result.nutrition_deviation   = incumbent_eval.nutrition_deviation_score;
        result.distinct_recipe_count = incumbent_eval.distinct_recipe_count;

        if (!interrupted && pq.empty()) {
            result.status            = ResultStatus::OPTIMAL;
            result.optimality_proven = true;
            result.termination_reason = "Branch and Bound: queue exhausted, optimality proven.";
            result.optimality_gap_pct = 0.0;
        } else {
            result.status            = ResultStatus::FEASIBLE_NOT_PROVEN;
            result.optimality_proven = false;
            result.termination_reason = "Branch and Bound: interrupted; best plan returned.";
            // Report best known bound vs incumbent
            if (!pq.empty()) {
                double bound = pq.top().bound;
                double inc   = incumbent_eval.primary_objective_score;
                if (mode == OptimizationMode::VARIETY) {
                    // gap = (bound - inc) / max(inc, eps) * 100
                    result.optimality_gap_pct = (bound - inc) / std::max(inc, 1e-9) * 100.0;
                } else {
                    result.optimality_gap_pct = (inc - bound) / std::max(std::abs(inc), 1e-9) * 100.0;
                }
            }
        }
    } else {
        result.status = interrupted ? ResultStatus::NO_FEASIBLE_FOUND
                                    : ResultStatus::INFEASIBLE;
        result.termination_reason = interrupted
            ? "Interrupted; no feasible plan found yet."
            : "No feasible plan exists (B&B exhaustive).";
    }

    return result;
}

