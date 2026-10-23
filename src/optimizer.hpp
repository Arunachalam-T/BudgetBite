// =============================================================================
// BudgetBite - Affordable Meal Planning System
// File: src/optimizer.hpp
// Description: Dynamic Programming and Greedy meal-plan optimizers
//
// ──────────────────────────────────────────────────────────────────────────────
// PROBLEM FORMULATION
// ──────────────────────────────────────────────────────────────────────────────
// We want to select ONE breakfast, ONE lunch, and ONE dinner for each of the
// 7 days such that:
//   1. Total cost  ≤ weekly_budget_inr
//   2. Daily kcal  ≥ daily_kcal_target     (soft constraint – reported)
//   3. Daily protein ≥ daily_protein_g     (soft constraint – reported)
//   4. Meals satisfy the user's dietary preference
//
// We maximize:   ∑  value(meal)
//   where value = (kcal / kcal_target) + (protein / protein_target)
//   subject to the hard budget constraint.
//
// ──────────────────────────────────────────────────────────────────────────────
// DYNAMIC PROGRAMMING (0-1 Knapsack variant)
// ──────────────────────────────────────────────────────────────────────────────
// State:
//   dp[d][b] = best total nutritional value achievable for day slots 0..d-1
//              using exactly b budget-units.
//
// We discretize the budget into BUDGET_UNITS integer steps.
//
// For each day d (0..6):
//   For each meal slot s ∈ {breakfast, lunch, dinner}:
//     For each candidate recipe r eligible for slot s:
//       cost_u = round(r.cost * BUDGET_UNITS / weekly_budget)
//       dp[b] = max(dp[b], dp[b - cost_u] + value(r))   (iterate b downward)
//
// Recurrence:
//   dp[j] = max(dp[j], dp[j – c_k] + v_k)    for j from W down to c_k
//
// Time complexity:  O(7 × 3 × R × W)
//   where R = number of candidate recipes per slot (~200–400)
//         W = BUDGET_UNITS (= 1000)
//   → roughly 7 × 3 × 300 × 1000 = 6,300,000 operations – very fast.
//
// Space complexity: O(W)   (rolling 1-D DP table)
//
// Traceback: we keep a separate 3-D choice table to reconstruct the plan.
//
// ──────────────────────────────────────────────────────────────────────────────
// GREEDY BASELINE
// ──────────────────────────────────────────────────────────────────────────────
// For each slot (day 1..7, slot B/L/D):
//   Sort candidates by value/cost ratio (descending).
//   Pick the best-ratio recipe whose cost fits the remaining budget.
//
// Time complexity:  O(7 × 3 × R × log R)
// Space complexity: O(R)
//
// The greedy algorithm is suboptimal in general (it may get stuck when
// a high-ratio choice leaves too little budget for remaining slots).
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

#include "types.hpp"

#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <limits>
#include <iostream>

// ── Constants ─────────────────────────────────────────────────────────────────
static constexpr int BUDGET_UNITS = 1000;    // DP discretization granularity

// ── Filter helpers ─────────────────────────────────────────────────────────────
static bool diet_ok(const Recipe& r, const std::string& pref) {
    if (pref == "vegetarian")    return r.is_vegetarian;
    if (pref == "vegan")         return r.is_vegan;
    if (pref == "non-vegetarian") return true;   // any recipe is fine
    return true;                               // "any" – no restriction
}

// Compute a nutritional "value" score for a recipe given targets
static double recipe_value(const Recipe& r,
                           double kcal_target,
                           double protein_target) {
    if (!r.nutrition_available) return 0.1;  // unknown – low but non-zero
    double v = (r.kcal_per_serving / kcal_target) * 0.5 +
               (r.protein_g_per_serving / protein_target) * 0.5;
    return v;
}

// Collect valid candidates for a meal slot
static std::vector<const Recipe*> get_candidates(
        const std::vector<Recipe>& recipes,
        bool want_breakfast, bool want_lunch, bool want_dinner,
        const std::string& diet_pref) {
    std::vector<const Recipe*> out;
    for (const auto& r : recipes) {
        if (r.is_beverage) continue;
        if (!r.has_any_cost_estimate()) continue;   // no cost data
        if (!diet_ok(r, diet_pref)) continue;

        bool slot_ok = (want_breakfast && r.is_breakfast) ||
                       (want_lunch    && r.is_lunch)      ||
                       (want_dinner   && r.is_dinner);
        if (slot_ok) out.push_back(&r);
    }
    return out;
}

// ── Shared data used during DP back-tracking ──────────────────────────────────
struct SlotChoice {
    int day;          // 0-based
    int slot;         // 0=B 1=L 2=D
    const Recipe* recipe = nullptr;
};

// ──────────────────────────────────────────────────────────────────────────────
// DP OPTIMIZER
// ──────────────────────────────────────────────────────────────────────────────
inline WeekPlan run_dp(const std::vector<Recipe>& all_recipes,
                       const UserConstraints& uc) {
    const int DAYS  = 7;
    const int SLOTS = 3;  // 0=breakfast 1=lunch 2=dinner

    // Cost per budget-unit (INR per unit)
    double budget_per_unit = uc.weekly_budget_inr / BUDGET_UNITS;

    // Collect candidates per slot type (same pool, filtered differently)
    auto bf_pool = get_candidates(all_recipes, true,  false, false, uc.diet_pref);
    auto lu_pool = get_candidates(all_recipes, false, true,  false, uc.diet_pref);
    auto di_pool = get_candidates(all_recipes, false, false, true,  uc.diet_pref);

    // If any pool is empty, relax to any-meal
    if (bf_pool.empty()) bf_pool = get_candidates(all_recipes, true, true, true, uc.diet_pref);
    if (lu_pool.empty()) lu_pool = get_candidates(all_recipes, true, true, true, uc.diet_pref);
    if (di_pool.empty()) di_pool = get_candidates(all_recipes, true, true, true, uc.diet_pref);

    std::vector<std::vector<const Recipe*>> pools = {bf_pool, lu_pool, di_pool};

    int total_slots = DAYS * SLOTS;

    // dp[j] = best value using j budget-units
    std::vector<double> dp(BUDGET_UNITS + 1, 0.0);

    // choice[slot_idx][budget_unit] = which recipe was chosen at that state
    // slot_idx = day*3 + slot (0..20)
    std::vector<std::vector<int>> choice(total_slots,
                                         std::vector<int>(BUDGET_UNITS + 1, -1));

    // We also need a "previous dp table" snapshot per slot to enable traceback
    // in the standard 0-1 knapsack manner.
    // Approach: process all 21 slots in sequence with a 1-D rolling DP.
    // For traceback we store, for each (slot, budget_unit), the candidate index
    // that was chosen when we transitioned from dp_prev to dp_curr.

    std::vector<double> prev_dp = dp;  // before this slot

    // Keep per-slot snapshots of dp table before that slot was processed.
    // We need O(total_slots × W) space for traceback.
    std::vector<std::vector<double>> dp_snapshot(total_slots + 1,
                                                  std::vector<double>(BUDGET_UNITS + 1, 0.0));
    dp_snapshot[0] = dp;

    for (int s = 0; s < total_slots; ++s) {
        int slot_type = s % SLOTS;  // 0,1,2
        const auto& pool = pools[slot_type];

        // Standard 0-1 knapsack: iterate budget downward
        for (const Recipe* rp : pool) {
            double raw_cost  = rp->effective_cost_per_serving();
            double val       = recipe_value(*rp, uc.daily_kcal_target / 3.0,
                                                 uc.daily_protein_g   / 3.0);
            int cost_u = static_cast<int>(std::round(raw_cost / budget_per_unit));
            if (cost_u < 1) cost_u = 1;
            if (cost_u > BUDGET_UNITS) continue;  // too expensive

            // Recipe index in pool (for traceback)
            int r_idx = static_cast<int>(rp - pool[0]);  // pointer arithmetic in pool

            for (int j = BUDGET_UNITS; j >= cost_u; --j) {
                double candidate = dp_snapshot[s][j - cost_u] + val;
                if (candidate > dp[j]) {
                    dp[j] = candidate;
                    choice[s][j] = r_idx;
                }
            }
        }
        dp_snapshot[s + 1] = dp;
    }

    // ── Traceback ─────────────────────────────────────────────────────────────
    // Find the best total budget usage
    int best_budget = BUDGET_UNITS;
    double best_val = dp[BUDGET_UNITS];
    for (int j = BUDGET_UNITS; j >= 0; --j) {
        if (dp[j] >= best_val) { best_val = dp[j]; best_budget = j; }
    }

    std::vector<const Recipe*> selected(total_slots, nullptr);
    int rem = best_budget;
    for (int s = total_slots - 1; s >= 0; --s) {
        int slot_type = s % SLOTS;
        const auto& pool = pools[slot_type];
        int idx = choice[s][rem];
        if (idx >= 0 && idx < static_cast<int>(pool.size())) {
            selected[s] = pool[idx];
            double raw_cost = pool[idx]->effective_cost_per_serving();
            int cost_u = static_cast<int>(std::round(raw_cost / budget_per_unit));
            if (cost_u < 1) cost_u = 1;
            rem -= cost_u;
            if (rem < 0) rem = 0;
        }
    }

    // ── Build output ──────────────────────────────────────────────────────────
    WeekPlan wp;
    wp.algorithm = "DP";
    wp.complexity_note =
        "Knapsack DP: T=O(7×3×R×W), S=O(W) where R≈avg candidates/slot, W=" +
        std::to_string(BUDGET_UNITS);

    double total_cost = 0.0;
    bool all_kcal   = true;
    bool all_prot   = true;

    for (int d = 0; d < DAYS; ++d) {
        DayPlan day;
        day.day_number = d + 1;
        double day_kcal = 0, day_prot = 0, day_carb = 0, day_fat = 0;

        for (int sl = 0; sl < SLOTS; ++sl) {
            int s = d * SLOTS + sl;
            const Recipe* rp = selected[s];
            if (!rp) {
                // Fallback: pick the cheapest recipe in pool
                const auto& pool = pools[sl];
                double best_eff = 1e9;
                for (auto* p : pool) {
                    double c = p->effective_cost_per_serving();
                    if (c > 0 && c < best_eff) { best_eff = c; rp = p; }
                }
            }
            if (rp) {
                if (sl == 0) day.breakfast_id = rp->recipe_id;
                if (sl == 1) day.lunch_id     = rp->recipe_id;
                if (sl == 2) day.dinner_id    = rp->recipe_id;
                double cost = rp->effective_cost_per_serving();
                day.total_cost_inr  += cost;
                total_cost          += cost;
                day_kcal  += rp->kcal_per_serving;
                day_prot  += rp->protein_g_per_serving;
                day_carb  += rp->carb_g_per_serving;
                day_fat   += rp->fat_g_per_serving;
            }
        }
        day.total_kcal      = day_kcal;
        day.total_protein_g = day_prot;
        day.total_carb_g    = day_carb;
        day.total_fat_g     = day_fat;
        day.kcal_met    = (day_kcal >= uc.daily_kcal_target * 0.8);  // 80% threshold
        day.protein_met = (day_prot >= uc.daily_protein_g  * 0.8);
        day.budget_met  = (day.total_cost_inr <= uc.weekly_budget_inr / 7.0 * 1.5);
        if (!day.kcal_met)    all_kcal = false;
        if (!day.protein_met) all_prot = false;
        wp.days.push_back(day);
    }

    wp.total_cost_inr   = total_cost;
    wp.budget_remaining = uc.weekly_budget_inr - total_cost;
    wp.budget_satisfied = (total_cost <= uc.weekly_budget_inr);
    wp.all_kcal_met     = all_kcal;
    wp.all_protein_met  = all_prot;

    return wp;
}

// ──────────────────────────────────────────────────────────────────────────────
// GREEDY BASELINE
// ──────────────────────────────────────────────────────────────────────────────
inline WeekPlan run_greedy(const std::vector<Recipe>& all_recipes,
                           const UserConstraints& uc) {
    const int DAYS  = 7;
    const int SLOTS = 3;

    auto bf_pool = get_candidates(all_recipes, true,  false, false, uc.diet_pref);
    auto lu_pool = get_candidates(all_recipes, false, true,  false, uc.diet_pref);
    auto di_pool = get_candidates(all_recipes, false, false, true,  uc.diet_pref);
    if (bf_pool.empty()) bf_pool = get_candidates(all_recipes, true, true, true, uc.diet_pref);
    if (lu_pool.empty()) lu_pool = get_candidates(all_recipes, true, true, true, uc.diet_pref);
    if (di_pool.empty()) di_pool = get_candidates(all_recipes, true, true, true, uc.diet_pref);

    // Sort each pool by value/cost ratio descending
    auto sort_pool = [&](std::vector<const Recipe*>& pool) {
        std::sort(pool.begin(), pool.end(), [&](const Recipe* a, const Recipe* b) {
            double ca = a->effective_cost_per_serving();
            double cb = b->effective_cost_per_serving();
            double per_slot_target_kcal = uc.daily_kcal_target / 3.0;
            double per_slot_target_prot = uc.daily_protein_g   / 3.0;
            double va = (ca > 0) ? recipe_value(*a, per_slot_target_kcal, per_slot_target_prot) / ca : 0;
            double vb = (cb > 0) ? recipe_value(*b, per_slot_target_kcal, per_slot_target_prot) / cb : 0;
            return va > vb;
        });
    };
    sort_pool(bf_pool);
    sort_pool(lu_pool);
    sort_pool(di_pool);

    std::vector<std::vector<const Recipe*>> pools = {bf_pool, lu_pool, di_pool};

    WeekPlan wp;
    wp.algorithm = "Greedy";
    wp.complexity_note = "Greedy ratio sort: T=O(7×3×R×logR), S=O(R)";

    double budget_left = uc.weekly_budget_inr;
    bool all_kcal = true, all_prot = true;

    // Keep track of used recipe IDs to avoid repeats within the week
    std::vector<std::string> used_ids;

    for (int d = 0; d < DAYS; ++d) {
        DayPlan day;
        day.day_number = d + 1;
        double day_kcal = 0, day_prot = 0, day_carb = 0, day_fat = 0;

        // Reserve budget proportionally for remaining days
        for (int sl = 0; sl < SLOTS; ++sl) {
            const auto& pool = pools[sl];
            const Recipe* chosen = nullptr;

            for (const Recipe* rp : pool) {
                double cost = rp->effective_cost_per_serving();
                if (cost <= 0) continue;
                if (cost > budget_left) continue;
                // Prefer not to repeat within 2 days (soft)
                bool recently_used = false;
                for (const auto& id : used_ids)
                    if (id == rp->recipe_id) { recently_used = true; break; }
                if (recently_used) continue;
                chosen = rp;
                break;
            }
            // If nothing found (too strict), relax the repeat constraint
            if (!chosen) {
                for (const Recipe* rp : pool) {
                    double cost = rp->effective_cost_per_serving();
                    if (cost > 0 && cost <= budget_left) { chosen = rp; break; }
                }
            }
            // Absolute fallback – pick cheapest
            if (!chosen && !pool.empty()) {
                const Recipe* cheapest = nullptr;
                double min_c = 1e9;
                for (auto* rp : pool) {
                    double c = rp->effective_cost_per_serving();
                    if (c > 0 && c < min_c) { min_c = c; cheapest = rp; }
                }
                chosen = cheapest;
            }

            if (chosen) {
                if (sl == 0) day.breakfast_id = chosen->recipe_id;
                if (sl == 1) day.lunch_id     = chosen->recipe_id;
                if (sl == 2) day.dinner_id    = chosen->recipe_id;
                double cost = chosen->effective_cost_per_serving();
                day.total_cost_inr  += cost;
                budget_left         -= cost;
                day_kcal += chosen->kcal_per_serving;
                day_prot += chosen->protein_g_per_serving;
                day_carb += chosen->carb_g_per_serving;
                day_fat  += chosen->fat_g_per_serving;
                used_ids.push_back(chosen->recipe_id);
                // Only keep last 14 used to avoid blocking too many
                if (used_ids.size() > 14) used_ids.erase(used_ids.begin());
            }
        }

        day.total_kcal      = day_kcal;
        day.total_protein_g = day_prot;
        day.total_carb_g    = day_carb;
        day.total_fat_g     = day_fat;
        day.kcal_met    = (day_kcal >= uc.daily_kcal_target * 0.8);
        day.protein_met = (day_prot >= uc.daily_protein_g   * 0.8);
        day.budget_met  = true;
        if (!day.kcal_met)    all_kcal = false;
        if (!day.protein_met) all_prot = false;
        wp.days.push_back(day);
    }

    wp.total_cost_inr   = uc.weekly_budget_inr - budget_left;
    wp.budget_remaining = budget_left;
    wp.budget_satisfied = (wp.total_cost_inr <= uc.weekly_budget_inr);
    wp.all_kcal_met     = all_kcal;
    wp.all_protein_met  = all_prot;

    return wp;
}
