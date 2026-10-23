// =============================================================================
// BudgetBite - Affordable Meal Planning System
// File: src/main.cpp
// Description: CLI entry point
//
// Usage:
//   budgetbite [OPTIONS]
//
// Options:
//   --csv    <path>   Path to the main dataset CSV  (default: Data/indian_meal_planner_indb_2024_11 (1).csv)
//   --budget <INR>    Weekly budget in INR           (default: 700)
//   --kcal   <kcal>   Daily calorie target           (default: 2000)
//   --protein <g>     Daily protein target in grams  (default: 50)
//   --diet   <pref>   "vegetarian" | "vegan" | "non-vegetarian" | "any"  (default: any)
//   --out    <path>   Output JSON file path          (default: meal_plan.json)
//
// The program:
//   1. Parses the CSV dataset
//   2. Runs the DP optimizer
//   3. Runs the Greedy baseline
//   4. Writes a single JSON file with both plans + recipe catalog + grocery list
//
// The JSON file is consumed by the web front-end (web/index.html).
// =============================================================================

#include "recipe_loader.hpp"
#include "optimizer.hpp"
#include "json_output.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

// ── Simple argument parser ────────────────────────────────────────────────────
static std::string get_arg(int argc, char* argv[],
                            const std::string& flag,
                            const std::string& def) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == flag) return argv[i + 1];
    }
    return def;
}

static double get_arg_d(int argc, char* argv[],
                         const std::string& flag,
                         double def) {
    std::string s = get_arg(argc, argv, flag, "");
    if (s.empty()) return def;
    try { return std::stod(s); } catch (...) { return def; }
}

// ── Banner ────────────────────────────────────────────────────────────────────
static void print_banner() {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════════╗\n";
    std::cout << "  ║   BudgetBite – Affordable Meal Planner       ║\n";
    std::cout << "  ║   Algorithm: Dynamic Programming vs Greedy   ║\n";
    std::cout << "  ╚══════════════════════════════════════════════╝\n\n";
}

int main(int argc, char* argv[]) {
    print_banner();

    // ── Parse arguments ────────────────────────────────────────────────────────
    std::string csv_path = get_arg(argc, argv, "--csv",
        "Data/indian_meal_planner_indb_2024_11 (1).csv");
    std::string out_path = get_arg(argc, argv, "--out", "web/meal_plan.json");
    std::string diet     = get_arg(argc, argv, "--diet", "any");

    UserConstraints uc;
    uc.weekly_budget_inr  = get_arg_d(argc, argv, "--budget", 700.0);
    uc.daily_kcal_target  = get_arg_d(argc, argv, "--kcal",   2000.0);
    uc.daily_protein_g    = get_arg_d(argc, argv, "--protein", 50.0);
    uc.diet_pref          = diet;

    std::cout << "  Settings:\n";
    std::cout << "    CSV        : " << csv_path << "\n";
    std::cout << "    Budget     : ₹" << uc.weekly_budget_inr << " / week\n";
    std::cout << "    Calories   : " << uc.daily_kcal_target << " kcal / day\n";
    std::cout << "    Protein    : " << uc.daily_protein_g   << " g / day\n";
    std::cout << "    Diet       : " << uc.diet_pref          << "\n";
    std::cout << "    Output     : " << out_path              << "\n\n";

    // ── Step 1: Load recipes ───────────────────────────────────────────────────
    std::cout << "  [1/4] Loading recipes from CSV...\n";
    int parse_errors = 0, missing_nutrition = 0, missing_cost = 0;

    std::vector<Recipe> recipes;
    try {
        recipes = load_recipes(csv_path, parse_errors, missing_nutrition, missing_cost);
    } catch (const std::exception& e) {
        std::cerr << "\n  ERROR: " << e.what() << "\n";
        return 1;
    }

    std::cout << "    Loaded       : " << recipes.size()    << " recipes\n";
    std::cout << "    Parse errors : " << parse_errors      << "\n";
    std::cout << "    No nutrition : " << missing_nutrition << "\n";
    std::cout << "    No cost data : " << missing_cost      << "\n";

    // Count breakfasts / lunches / dinners available
    int bf = 0, lu = 0, di = 0;
    for (const auto& r : recipes) {
        if (!r.is_beverage && r.has_any_cost_estimate()) {
            if (r.is_breakfast) ++bf;
            if (r.is_lunch)     ++lu;
            if (r.is_dinner)    ++di;
        }
    }
    std::cout << "    Eligible B/L/D with cost: "
              << bf << " / " << lu << " / " << di << "\n\n";

    // ── Step 2: Run DP ─────────────────────────────────────────────────────────
    std::cout << "  [2/4] Running Dynamic Programming optimizer...\n";
    WeekPlan dp_plan = run_dp(recipes, uc);
    std::cout << "    Total cost   : ₹" << dp_plan.total_cost_inr     << "\n";
    std::cout << "    Remaining    : ₹" << dp_plan.budget_remaining   << "\n";
    std::cout << "    Budget OK    : " << (dp_plan.budget_satisfied ? "YES" : "NO") << "\n";
    std::cout << "    All kcal met : " << (dp_plan.all_kcal_met    ? "YES" : "NO*") << "\n";
    std::cout << "    All prot met : " << (dp_plan.all_protein_met ? "YES" : "NO*") << "\n";
    std::cout << "    Complexity   : " << dp_plan.complexity_note      << "\n\n";

    // ── Step 3: Run Greedy ─────────────────────────────────────────────────────
    std::cout << "  [3/4] Running Greedy baseline...\n";
    WeekPlan greedy_plan = run_greedy(recipes, uc);
    std::cout << "    Total cost   : ₹" << greedy_plan.total_cost_inr   << "\n";
    std::cout << "    Remaining    : ₹" << greedy_plan.budget_remaining << "\n";
    std::cout << "    Budget OK    : " << (greedy_plan.budget_satisfied ? "YES" : "NO") << "\n";
    std::cout << "    All kcal met : " << (greedy_plan.all_kcal_met    ? "YES" : "NO*") << "\n";
    std::cout << "    All prot met : " << (greedy_plan.all_protein_met ? "YES" : "NO*") << "\n";
    std::cout << "    Complexity   : " << greedy_plan.complexity_note    << "\n\n";

    if (!dp_plan.all_kcal_met || !dp_plan.all_protein_met) {
        std::cout << "  * NOTE: Nutritional targets not fully met (80% threshold used).\n";
        std::cout << "    Consider increasing your budget or relaxing diet preferences.\n\n";
    }
    if (!dp_plan.budget_satisfied) {
        std::cout << "  * WARNING: Budget exceeded. No feasible plan found within budget.\n";
        std::cout << "    Displaying the lowest-cost plan available.\n\n";
    }

    // ── Step 4: Write JSON ─────────────────────────────────────────────────────
    std::cout << "  [4/4] Writing output JSON to " << out_path << "...\n";
    std::string json = build_full_output(dp_plan, greedy_plan, recipes, uc,
                                         parse_errors, missing_nutrition, missing_cost);
    {
        std::ofstream out(out_path);
        if (!out.is_open()) {
            std::cerr << "  ERROR: Cannot write to " << out_path << "\n";
            return 1;
        }
        out << json;
    }

    // Also write companion web/meal_plan_data.js for zero-setup browser viewing
    {
        std::ofstream js_out("web/meal_plan_data.js");
        if (js_out.is_open()) {
            js_out << "// Auto-generated by budgetbite.exe\n";
            js_out << "window.BUDGETBITE_DATA = " << json << ";\n";
        }
    }

    std::cout << "    Done. Open web/index.html in your browser to view the plan.\n\n";

    // ── Comparison summary ──────────────────────────────────────────────────────
    std::cout << "  ┌──────────────────────────────────────────────┐\n";
    std::cout << "  │          Algorithm Comparison                 │\n";
    std::cout << "  ├──────────────────┬───────────┬───────────────┤\n";
    std::cout << "  │ Metric           │ DP        │ Greedy        │\n";
    std::cout << "  ├──────────────────┼───────────┼───────────────┤\n";
    std::cout << "  │ Total Cost (₹)   │ " << std::setw(9) << std::fixed
              << std::setprecision(2) << dp_plan.total_cost_inr
              << " │ " << std::setw(13) << greedy_plan.total_cost_inr << " │\n";
    std::cout << "  │ Budget Satisfied  │ "
              << (dp_plan.budget_satisfied ? "YES      " : "NO       ") << " │ "
              << (greedy_plan.budget_satisfied ? "YES          " : "NO           ") << " │\n";
    std::cout << "  │ All Kcal Met     │ "
              << (dp_plan.all_kcal_met ? "YES      " : "NO       ") << " │ "
              << (greedy_plan.all_kcal_met ? "YES          " : "NO           ") << " │\n";
    std::cout << "  │ All Protein Met  │ "
              << (dp_plan.all_protein_met ? "YES      " : "NO       ") << " │ "
              << (greedy_plan.all_protein_met ? "YES          " : "NO           ") << " │\n";
    std::cout << "  └──────────────────┴───────────┴───────────────┘\n\n";

    std::cout << "  BudgetBite complete!\n\n";
    return 0;
}
