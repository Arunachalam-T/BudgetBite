// test_algorithms.cpp — Algorithm correctness and comparison tests

#include "brute_force.h"
#include "greedy.h"
#include "dynamic_programming.h"
#include "backtracking.h"
#include "branch_and_bound.h"
#include "plan_evaluator.h"
#include "recipe_calculator.h"
#include <iostream>
#include <cmath>

static int atest_run = 0, atest_passed = 0;
#define AASSERT(cond, msg) do { \
    ++atest_run; \
    if (cond) { ++atest_passed; std::cout << "  PASS: " << msg << "\n"; } \
    else { std::cout << "  FAIL: " << msg << "\n"; } \
} while(0)

// ─── Small dataset: 2 meals/day, 3 recipes per type ──────────────────────

static Dataset make_small_dataset() {
    Dataset ds;

    auto add_ing = [&](const std::string& id, double cal, double prot, double carb, double fat) {
        Ingredient ing;
        ing.ingredient_id = id; ing.name = id;
        ing.price_per_kg_inr = 50.0; ing.calories_per_100g = cal;
        ing.protein_g_per_100g = prot; ing.carbs_g_per_100g = carb;
        ing.fat_g_per_100g = fat; ing.diet_category = "Plant-based";
        ds.ingredients.push_back(ing);
        ds.ingredient_map[id] = ing;
    };

    add_ing("IA", 300.0, 10.0, 60.0, 2.0);
    add_ing("IB", 400.0, 15.0, 70.0, 5.0);
    add_ing("IC", 200.0,  8.0, 40.0, 1.0);

    auto add_recipe = [&](const std::string& id, const std::string& meal,
                          const std::string& ing_id, double qty) {
        Recipe r; r.recipe_id = id; r.recipe_name = id; r.meal_type = meal;
        ds.recipes.push_back(r);
        ds.recipe_map[id] = r;
        RecipeIngredient ri; ri.recipe_id = id; ri.ingredient_id = ing_id; ri.quantity_g_per_serving = qty;
        ds.recipe_ingredients.push_back(ri);
    };

    // 3 Breakfast options, 3 Dinner options
    add_recipe("B1","Breakfast","IA",200.0);  // cal=600, prot=20, carb=120, fat=4
    add_recipe("B2","Breakfast","IB",150.0);  // cal=600, prot=22.5, carb=105, fat=7.5
    add_recipe("B3","Breakfast","IC",300.0);  // cal=600, prot=24, carb=120, fat=3
    add_recipe("D1","Dinner","IA",300.0);     // cal=900, prot=30, carb=180, fat=6
    add_recipe("D2","Dinner","IB",200.0);     // cal=800, prot=30, carb=140, fat=10
    add_recipe("D3","Dinner","IC",400.0);     // cal=800, prot=32, carb=160, fat=4

    RecipeCalculator::calculate(ds);
    ds.is_loaded = true;
    ds.is_valid  = true;
    return ds;
}

static OptimizationConfig make_small_config() {
    OptimizationConfig cfg;
    cfg.meals_per_day         = 2;
    cfg.weekly_budget_inr     = 10000.0; // generous
    cfg.max_recipe_repetitions = 7;
    cfg.diet_type             = DietType::UNRESTRICTED;
    cfg.constraints.min_calories = 1000;
    cfg.constraints.max_calories = 3000;
    cfg.constraints.min_protein  = 30;
    cfg.constraints.min_carbs    = 100;
    cfg.constraints.max_fat      = 100;
    cfg.targets.target_calories  = 1500;
    cfg.targets.target_protein   = 60;
    cfg.targets.target_carbs     = 250;
    cfg.targets.target_fat       = 40;
    return cfg;
}

static void test_algorithms_find_feasible() {
    std::cout << "\n[test_algorithms_find_feasible]\n";
    auto ds  = make_small_dataset();
    auto cfg = make_small_config();
    ExecutionLimits limits; limits.max_time_seconds = 10.0; limits.max_nodes = 500000;

    BruteForce bf;
    Greedy     gr;
    DynamicProgramming dp;
    Backtracking bt;
    BranchAndBound bnb;

    auto res_bf  = bf.solve(ds, cfg, OptimizationMode::NUTRITION, limits);
    auto res_gr  = gr.solve(ds, cfg, OptimizationMode::NUTRITION, limits);
    auto res_dp  = dp.solve(ds, cfg, OptimizationMode::NUTRITION, limits);
    auto res_bt  = bt.solve(ds, cfg, OptimizationMode::NUTRITION, limits);
    auto res_bnb = bnb.solve(ds, cfg, OptimizationMode::NUTRITION, limits);

    AASSERT(res_bf.is_feasible  || res_bf.status == ResultStatus::INFEASIBLE,  "Brute Force terminates correctly");
    AASSERT(res_gr.is_feasible,  "Greedy finds feasible plan");
    AASSERT(res_dp.is_feasible  || res_dp.status == ResultStatus::INFEASIBLE,  "DP terminates correctly");
    AASSERT(res_bt.is_feasible  || res_bt.status == ResultStatus::INFEASIBLE,  "Backtracking terminates correctly");
    AASSERT(res_bnb.is_feasible || res_bnb.status == ResultStatus::INFEASIBLE, "B&B terminates correctly");
}

static void test_exact_algorithms_agree() {
    std::cout << "\n[test_exact_algorithms_agree]\n";
    auto ds  = make_small_dataset();
    auto cfg = make_small_config();
    ExecutionLimits limits; limits.max_time_seconds = 10.0; limits.max_nodes = 500000;

    BruteForce     bf;
    Backtracking   bt;
    BranchAndBound bnb;
    DynamicProgramming dp;

    auto res_bf  = bf.solve(ds,  cfg, OptimizationMode::BUDGET, limits);
    auto res_bt  = bt.solve(ds,  cfg, OptimizationMode::BUDGET, limits);
    auto res_bnb = bnb.solve(ds, cfg, OptimizationMode::BUDGET, limits);
    auto res_dp  = dp.solve(ds,  cfg, OptimizationMode::BUDGET, limits);

    bool bf_proven  = (res_bf.status  == ResultStatus::OPTIMAL);
    bool bt_proven  = (res_bt.status  == ResultStatus::OPTIMAL);
    bool bnb_proven = (res_bnb.status == ResultStatus::OPTIMAL);
    bool dp_proven  = (res_dp.status  == ResultStatus::OPTIMAL);

    if (bf_proven && bt_proven) {
        AASSERT(std::abs(res_bf.weekly_purchase_cost - res_bt.weekly_purchase_cost) < 0.01,
                "BF and BT agree on optimal budget cost");
    }
    if (bf_proven && bnb_proven) {
        AASSERT(std::abs(res_bf.weekly_purchase_cost - res_bnb.weekly_purchase_cost) < 0.01,
                "BF and B&B agree on optimal budget cost");
    }
    if (bf_proven && dp_proven) {
        AASSERT(std::abs(res_bf.weekly_purchase_cost - res_dp.weekly_purchase_cost) < 0.01,
                "BF and DP agree on optimal budget cost");
    }

    // All returned feasible plans must pass the evaluator
    auto check_eval = [&](const AlgorithmResult& r, const std::string& name) {
        if (r.is_feasible) {
            auto eval = PlanEvaluator::evaluate(r.best_plan, ds, cfg, OptimizationMode::BUDGET);
            AASSERT(eval.is_feasible, name + " plan passes evaluator");
        }
    };
    check_eval(res_bf,  "Brute Force");
    check_eval(res_bt,  "Backtracking");
    check_eval(res_bnb, "B&B");
    check_eval(res_dp,  "DP");
}

static void test_infeasible_config() {
    std::cout << "\n[test_infeasible_config]\n";
    auto ds  = make_small_dataset();
    auto cfg = make_small_config();
    cfg.constraints.min_calories = 99999; // impossible
    ExecutionLimits limits; limits.max_time_seconds = 5.0; limits.max_nodes = 100000;

    BruteForce bf;
    auto res = bf.solve(ds, cfg, OptimizationMode::NUTRITION, limits);
    AASSERT(!res.is_feasible, "Infeasible config: no plan found");
}

int run_algorithm_tests() {
    std::cout << "=== ALGORITHM TESTS ===\n";
    test_algorithms_find_feasible();
    test_exact_algorithms_agree();
    test_infeasible_config();
    return atest_passed == atest_run ? 0 : 1;
}

