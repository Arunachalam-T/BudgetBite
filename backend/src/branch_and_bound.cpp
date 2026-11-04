// branch_and_bound.cpp — Best-first Branch and Bound with priority queue

#include "branch_and_bound.h"
#include "plan_evaluator.h"
#include "greedy.h"
#include <chrono>
#include <queue>
#include <algorithm>
#include <limits>
#include <cmath>
#include <unordered_set>

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

    // Optimistic bound for priority queue ordering (admissible)
    double bound = 0.0;

    // Heuristic score for tie-breaking and dive guidance
    double heuristic_score = 0.0;
};

// ─── Bound computation ────────────────────────────────────────────────────
// Compute an optimistic bound (admissible, i.e., never worse than actual best).

static double compute_bound(
    const BnBNode& node,
    int total_slots,
    int n_slots_per_day,
    const Dataset& dataset,
    const OptimizationConfig& config,
    OptimizationMode mode)
{
    switch (mode) {
        case OptimizationMode::BUDGET: {
            // Actual purchase cost for already-consumed ingredients
            double purchase_cost = 0.0;
            for (const auto& [ing_id, consumed_g] : node.ingredient_consumed) {
                double inv = 0.0;
                auto inv_it = dataset.inventory_map.find(ing_id);
                if (inv_it != dataset.inventory_map.end()) inv = inv_it->second;
                double purchased_g = std::max(0.0, consumed_g - inv);
                auto ing_it = dataset.ingredient_map.find(ing_id);
                if (ing_it != dataset.ingredient_map.end()) {
                    purchase_cost += (purchased_g / 1000.0) * ing_it->second.price_per_kg_inr;
                }
            }
            return purchase_cost;
        }
        case OptimizationMode::NUTRITION: {
            // Admissible lower bound on 7-day average nutrition deviation:
            // sum of exact deviations of all completed days / 7.0
            int completed_days = node.slot_index / n_slots_per_day;
            double sum_dev = 0.0;
            for (int d = 0; d < completed_days; ++d) {
                sum_dev += PlanEvaluator::daily_nutrition_deviation(
                    node.day_cal[d], node.day_prot[d], node.day_carb[d], node.day_fat[d],
                    config.targets);
            }
            return sum_dev / 7.0;
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

// ─── Heuristic candidate score ────────────────────────────────────────────

static double calc_recipe_score(
    const CalculatedRecipe& cr,
    const BnBNode& node,
    int day,
    const OptimizationConfig& config,
    OptimizationMode mode,
    const Dataset& dataset)
{
    switch (mode) {
        case OptimizationMode::NUTRITION: {
            double new_cal  = node.day_cal[day]  + cr.calories_per_serving;
            double new_prot = node.day_prot[day] + cr.protein_per_serving;
            double new_carb = node.day_carb[day] + cr.carbs_per_serving;
            double new_fat  = node.day_fat[day]  + cr.fat_per_serving;

            const auto& t = config.targets;
            const double eps = 1e-9;
            double dev =
                std::abs(new_cal  - t.target_calories) / std::max(t.target_calories, eps) +
                std::abs(new_prot - t.target_protein)  / std::max(t.target_protein,  eps) +
                std::abs(new_carb - t.target_carbs)    / std::max(t.target_carbs,    eps) +
                std::abs(new_fat  - t.target_fat)      / std::max(t.target_fat,      eps);
            return -dev; // higher is better (less deviation from targets)
        }
        case OptimizationMode::BUDGET: {
            double marginal_cost = 0.0;
            for (const auto& ri : cr.ingredients) {
                double consumed = 0.0;
                auto it = node.ingredient_consumed.find(ri.ingredient_id);
                if (it != node.ingredient_consumed.end()) consumed = it->second;

                double inv = 0.0;
                auto inv_it = dataset.inventory_map.find(ri.ingredient_id);
                if (inv_it != dataset.inventory_map.end()) inv = inv_it->second;

                double already_purchased = std::max(0.0, consumed - inv);
                double new_purchased     = std::max(0.0, consumed + ri.quantity_g_per_serving - inv);
                double marginal_g        = new_purchased - already_purchased;

                auto ing_it = dataset.ingredient_map.find(ri.ingredient_id);
                if (ing_it != dataset.ingredient_map.end())
                    marginal_cost += (marginal_g / 1000.0) * ing_it->second.price_per_kg_inr;
            }
            return -marginal_cost; // higher is better (lower cost)
        }
        case OptimizationMode::VARIETY: {
            auto rep_it = node.reps.find(cr.recipe_id);
            int reps = (rep_it != node.reps.end()) ? rep_it->second : 0;
            return -static_cast<double>(reps);
        }
    }
    return 0.0;
}

// ─── Priority Queue ordering ──────────────────────────────────────────────

struct NodeCmp {
    OptimizationMode mode;
    bool operator()(const BnBNode& a, const BnBNode& b) const {
        // Priority 1: Deeper nodes first (Depth-First Dive towards complete plans)
        if (a.slot_index != b.slot_index)
            return a.slot_index < b.slot_index;

        // Priority 2: At same depth, prioritize better bounds
        if (std::abs(a.bound - b.bound) > 1e-6) {
            if (mode == OptimizationMode::VARIETY)
                return a.bound < b.bound;
            return a.bound > b.bound;
        }

        // Priority 3: At same depth and bound, prioritize higher heuristic score
        if (std::abs(a.heuristic_score - b.heuristic_score) > 1e-6)
            return a.heuristic_score < b.heuristic_score;

        return false;
    }
};

class BnBQueue : public std::priority_queue<BnBNode, std::vector<BnBNode>, NodeCmp> {
public:
    explicit BnBQueue(const NodeCmp& cmp)
        : std::priority_queue<BnBNode, std::vector<BnBNode>, NodeCmp>(cmp) {}

    double find_best_bound(OptimizationMode mode) const {
        if (c.empty()) return 0.0;
        double best = c.front().bound;
        for (const auto& node : c) {
            if (mode == OptimizationMode::VARIETY) {
                if (node.bound > best) best = node.bound;
            } else {
                if (node.bound < best) best = node.bound;
            }
        }
        return best;
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

    // Primal Heuristic: Seed with Greedy heuristic in <1ms
    Greedy greedy;
    auto g_res = greedy.solve(dataset, config, mode, limits);
    if (g_res.is_feasible) {
        incumbent_plan = g_res.best_plan;
        incumbent_eval = g_res.evaluation;
        found_any = true;
    }

    // Priority queue (best-first with dive-and-bound)
    NodeCmp cmp{mode};
    BnBQueue pq(cmp);

    // Root node
    BnBNode root;
    root.slot_index = 0;
    root.day_cal.assign(n_days, 0.0);
    root.day_prot.assign(n_days, 0.0);
    root.day_carb.assign(n_days, 0.0);
    root.day_fat.assign(n_days, 0.0);
    root.bound = compute_bound(root, total, n_slots, dataset, config, mode);
    root.heuristic_score = 0.0;
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
                if (node.bound <= incumbent_eval.distinct_recipe_count) prune = true;
            } else {
                if (node.bound >= incumbent_eval.primary_objective_score - 1e-9) prune = true;
            }
            if (prune) { ++result.branches_pruned; continue; }
        }

        // Leaf node: full 7-day meal plan
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

        bool is_last_slot_of_day = ((node.slot_index % n_slots) == (n_slots - 1));

        for (const auto* cr : cit->second) {
            if (interrupted) break;

            // Repetition prune
            if (PlanEvaluator::repetition_exceeded(node.reps, cr->recipe_id,
                                                   config.max_recipe_repetitions)) {
                ++result.branches_pruned;
                continue;
            }

            // Fat prune (monotonic)
            double new_fat = node.day_fat[day] + cr->fat_per_serving;
            if (new_fat > config.constraints.max_fat + 1e-9) {
                ++result.branches_pruned;
                continue;
            }

            // Calorie upper bound prune (monotonic)
            double new_cal = node.day_cal[day] + cr->calories_per_serving;
            if (new_cal > config.constraints.max_calories + 1e-9) {
                ++result.branches_pruned;
                continue;
            }

            // End-of-day daily constraint pruning
            if (is_last_slot_of_day) {
                if (new_cal < config.constraints.min_calories - 1e-9) {
                    ++result.branches_pruned;
                    continue;
                }
                double new_prot = node.day_prot[day] + cr->protein_per_serving;
                if (new_prot < config.constraints.min_protein - 1e-9) {
                    ++result.branches_pruned;
                    continue;
                }
                double new_carb = node.day_carb[day] + cr->carbs_per_serving;
                if (new_carb < config.constraints.min_carbs - 1e-9) {
                    ++result.branches_pruned;
                    continue;
                }
            }

            // Build child
            BnBNode child = node;
            MealSlotAssignment a;
            a.day = day; a.meal_type = meal_type;
            a.recipe_id = cr->recipe_id; a.servings = 1;
            child.partial.push_back(a);
            child.reps[cr->recipe_id]++;
            child.day_cal[day]  = new_cal;
            child.day_prot[day] += cr->protein_per_serving;
            child.day_carb[day] += cr->carbs_per_serving;
            child.day_fat[day]  = new_fat;
            child.accumulated_gross_cost += cr->gross_cost_per_serving;
            for (const auto& ri : cr->ingredients)
                child.ingredient_consumed[ri.ingredient_id] += ri.quantity_g_per_serving;
            child.slot_index = node.slot_index + 1;

            child.bound = compute_bound(child, total, n_slots, dataset, config, mode);
            child.heuristic_score = node.heuristic_score +
                calc_recipe_score(*cr, node, day, config, mode, dataset);

            // Budget constraint prune: purchase cost so far cannot exceed weekly budget
            if (mode == OptimizationMode::BUDGET || mode == OptimizationMode::NUTRITION) {
                double current_purchase_cost = (mode == OptimizationMode::BUDGET)
                    ? child.bound
                    : compute_bound(child, total, n_slots, dataset, config, OptimizationMode::BUDGET);
                if (current_purchase_cost > config.weekly_budget_inr + 1e-9) {
                    ++result.branches_pruned;
                    continue;
                }
            }

            // Prune child by bound against incumbent
            if (found_any) {
                bool prune = false;
                if (mode == OptimizationMode::VARIETY)
                    prune = (child.bound <= incumbent_eval.distinct_recipe_count);
                else
                    prune = (child.bound >= incumbent_eval.primary_objective_score - 1e-9);
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
            if (!pq.empty()) {
                double bound = pq.find_best_bound(mode);
                double inc   = incumbent_eval.primary_objective_score;
                if (mode == OptimizationMode::VARIETY) {
                    result.optimality_gap_pct = std::max(0.0, (bound - inc) / std::max(inc, 1e-9) * 100.0);
                } else {
                    result.optimality_gap_pct = std::max(0.0, (inc - bound) / std::max(std::abs(inc), 1e-9) * 100.0);
                }
            } else {
                result.optimality_gap_pct = 0.0;
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
