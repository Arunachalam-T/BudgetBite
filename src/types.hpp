// =============================================================================
// BudgetBite - Affordable Meal Planning System
// File: src/types.hpp
// Description: Shared data types used across all modules
// =============================================================================
#pragma once

#include <string>
#include <vector>

// ── Ingredient (parsed from ingredients_json inside the CSV) ─────────────────
struct Ingredient {
    std::string name;
    double      amount    = 0.0;
    std::string unit;
    std::string cost_price_status;   // "priced" | "unpriced" | "zero_cost_water_excluded"
    std::string cost_price_commodity;
    double      cost_line_inr       = 0.0;  // per full recipe batch
    bool        is_priced           = false;
};

// ── A recipe loaded from the main CSV ────────────────────────────────────────
struct Recipe {
    // --- Identifiers ---
    std::string recipe_id;
    std::string recipe_name;
    std::string source;
    int         servings         = 1;
    std::string serving_unit;

    // --- Nutrition per serving ---
    double kcal_per_serving      = 0.0;
    double protein_g_per_serving = 0.0;
    double carb_g_per_serving    = 0.0;
    double fat_g_per_serving     = 0.0;
    double fibre_g_per_serving   = 0.0;
    bool   nutrition_available   = false;

    // --- Cost per serving ---
    // cost_inr_per_serving is set only if cost_status == "complete_estimate"
    // cost_subtotal_inr_per_serving is a partial subtotal (may cover <100%)
    double cost_inr_per_serving          = 0.0;   // 0 → unknown / partial
    double cost_subtotal_inr_per_serving = 0.0;   // partial subtotal
    double cost_coverage_pct             = 0.0;   // 0–100
    std::string cost_status;                       // "complete_estimate" | "partial_cost"
    bool   has_complete_cost             = false;

    // --- Ingredients list ---
    std::vector<Ingredient> ingredients;

    // --- Meal-type tags inferred from category / name ---
    bool is_breakfast = false;
    bool is_lunch     = false;
    bool is_dinner    = false;
    bool is_snack     = false;
    bool is_beverage  = false;

    // --- Dietary flags (populated by heuristics on ingredient list) ---
    bool is_vegetarian  = true;
    bool is_vegan       = false;
    bool is_non_veg     = false;

    // --- For DP: effective cost used in optimization ---
    double effective_cost_per_serving() const {
        if (has_complete_cost) return cost_inr_per_serving;
        if (cost_subtotal_inr_per_serving > 0 && cost_coverage_pct > 0)
            return cost_subtotal_inr_per_serving / (cost_coverage_pct / 100.0);
        return 0.0; // unknown – DP will treat this as zero-cost candidate
    }

    bool has_any_cost_estimate() const {
        return has_complete_cost || cost_subtotal_inr_per_serving > 0;
    }
};

// ── User constraints supplied via CLI / JSON ─────────────────────────────────
struct UserConstraints {
    double weekly_budget_inr  = 700.0;
    double daily_kcal_target  = 2000.0;
    double daily_protein_g    = 50.0;
    std::string diet_pref;              // "vegetarian" | "vegan" | "non-vegetarian" | "any"
};

// ── A single day's meal plan ─────────────────────────────────────────────────
struct DayPlan {
    int day_number = 0;  // 1–7
    std::string breakfast_id;
    std::string lunch_id;
    std::string dinner_id;

    double total_cost_inr    = 0.0;
    double total_kcal        = 0.0;
    double total_protein_g   = 0.0;
    double total_carb_g      = 0.0;
    double total_fat_g       = 0.0;

    bool kcal_met    = false;
    bool protein_met = false;
    bool budget_met  = false;
};

// ── The full 7-day plan output ────────────────────────────────────────────────
struct WeekPlan {
    std::string algorithm;           // "DP" or "Greedy"
    std::vector<DayPlan> days;

    double total_cost_inr    = 0.0;
    double budget_remaining  = 0.0;
    bool   budget_satisfied  = false;
    bool   all_kcal_met      = false;
    bool   all_protein_met   = false;

    // Time & space complexity note
    std::string complexity_note;
};
