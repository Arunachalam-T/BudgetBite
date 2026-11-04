// test_evaluator.cpp — Plan evaluation tests

#include "plan_evaluator.h"
#include "recipe_calculator.h"
#include <cassert>
#include <iostream>
#include <cmath>

static int etest_run = 0, etest_passed = 0;
#define EASSERT(cond, msg) do { \
    ++etest_run; \
    if (cond) { ++etest_passed; std::cout << "  PASS: " << msg << "\n"; } \
    else { std::cout << "  FAIL: " << msg << "\n"; } \
} while(0)

// Build a small dataset with Breakfast, Lunch, Dinner recipes
static Dataset make_eval_dataset() {
    Dataset ds;

    auto add_ing = [&](const std::string& id, const std::string& name,
                       double price, double cal, double prot, double carb, double fat,
                       const std::string& diet) {
        Ingredient ing;
        ing.ingredient_id = id; ing.name = name;
        ing.price_per_kg_inr = price; ing.calories_per_100g = cal;
        ing.protein_g_per_100g = prot; ing.carbs_g_per_100g = carb;
        ing.fat_g_per_100g = fat; ing.diet_category = diet;
        ds.ingredients.push_back(ing);
        ds.ingredient_map[id] = ing;
    };

    add_ing("I001","Oats",   80.0, 389.0, 17.0, 66.0,  7.0, "Plant-based");
    add_ing("I002","Milk",   62.0, 61.0,  3.2,  4.8,   3.3, "Dairy");
    add_ing("I003","Rice",   42.0, 350.0, 7.0,  78.0,  0.5, "Plant-based");
    add_ing("I004","Dal",    90.0, 340.0, 22.0, 60.0,  1.0, "Plant-based");
    add_ing("I005","Roti",   33.0, 300.0, 11.0, 60.0,  1.5, "Plant-based");
    add_ing("I006","Sabzi",  30.0, 80.0,  2.0,  15.0,  1.0, "Plant-based");
    add_ing("I007","Chicken",250.0,165.0, 31.0, 0.0,   3.6, "Meat");

    auto add_recipe = [&](const std::string& id, const std::string& name,
                          const std::string& meal,
                          const std::vector<std::pair<std::string,double>>& ings) {
        Recipe r; r.recipe_id = id; r.recipe_name = name; r.meal_type = meal;
        ds.recipes.push_back(r);
        ds.recipe_map[id] = r;
        for (const auto& [iid,qty] : ings) {
            RecipeIngredient ri; ri.recipe_id = id; ri.ingredient_id = iid; ri.quantity_g_per_serving = qty;
            ds.recipe_ingredients.push_back(ri);
        }
    };

    add_recipe("R001","Oat Porridge","Breakfast",{{"I001",80},{"I002",200}});
    add_recipe("R002","Dal Rice","Lunch",{{"I003",150},{"I004",80}});
    add_recipe("R003","Roti Sabzi","Dinner",{{"I005",120},{"I006",100}});
    add_recipe("R004","Chicken Rice","Dinner",{{"I003",150},{"I007",120}});

    RecipeCalculator::calculate(ds);

    ds.is_loaded = true;
    ds.is_valid  = true;
    return ds;
}

static OptimizationConfig make_config(int meals_per_day = 3,
                                      DietType diet = DietType::UNRESTRICTED) {
    OptimizationConfig cfg;
    cfg.meals_per_day         = meals_per_day;
    cfg.weekly_budget_inr     = 1500.0;
    cfg.max_recipe_repetitions = 7;
    cfg.diet_type             = diet;
    cfg.constraints.min_calories = 500;
    cfg.constraints.max_calories = 3000;
    cfg.constraints.min_protein  = 20;
    cfg.constraints.min_carbs    = 50;
    cfg.constraints.max_fat      = 100;
    cfg.targets.target_calories  = 2000;
    cfg.targets.target_protein   = 70;
    cfg.targets.target_carbs     = 250;
    cfg.targets.target_fat       = 60;
    return cfg;
}

// Build a 7-day plan with a given recipe repeated for all slots
static MealPlan make_plan_all(const Dataset& ds, const std::string& r_breakfast,
                               const std::string& r_lunch, const std::string& r_dinner) {
    MealPlan plan;
    for (int d = 0; d < 7; ++d) {
        {MealSlotAssignment a; a.day=d; a.meal_type="Breakfast"; a.recipe_id=r_breakfast; a.servings=1; plan.assignments.push_back(a);}
        {MealSlotAssignment a; a.day=d; a.meal_type="Lunch";     a.recipe_id=r_lunch;     a.servings=1; plan.assignments.push_back(a);}
        {MealSlotAssignment a; a.day=d; a.meal_type="Dinner";    a.recipe_id=r_dinner;    a.servings=1; plan.assignments.push_back(a);}
    }
    return plan;
}

static void test_feasible_plan() {
    std::cout << "\n[test_feasible_plan]\n";
    auto ds  = make_eval_dataset();
    auto cfg = make_config();
    auto plan = make_plan_all(ds, "R001","R002","R003");
    auto eval = PlanEvaluator::evaluate(plan, ds, cfg, OptimizationMode::NUTRITION);
    EASSERT(eval.is_feasible, "Plan is feasible");
    EASSERT(eval.daily_calories.size() == 7, "7 daily calorie values");
    EASSERT(eval.weekly_purchase_cost > 0, "Positive purchase cost");
}

static void test_budget_violation() {
    std::cout << "\n[test_budget_violation]\n";
    auto ds  = make_eval_dataset();
    auto cfg = make_config();
    cfg.weekly_budget_inr = 0.01; // extremely low
    auto plan = make_plan_all(ds, "R001","R002","R003");
    auto eval = PlanEvaluator::evaluate(plan, ds, cfg, OptimizationMode::BUDGET);
    EASSERT(!eval.is_feasible, "Budget violation detected");
}

static void test_fat_violation() {
    std::cout << "\n[test_fat_violation]\n";
    auto ds  = make_eval_dataset();
    auto cfg = make_config();
    cfg.constraints.max_fat = 0.0; // impossible
    auto plan = make_plan_all(ds, "R001","R002","R003");
    auto eval = PlanEvaluator::evaluate(plan, ds, cfg, OptimizationMode::NUTRITION);
    EASSERT(!eval.is_feasible, "Fat violation detected");
}

static void test_vegetarian_incompatibility() {
    std::cout << "\n[test_vegetarian_incompatibility]\n";
    auto ds  = make_eval_dataset();
    auto cfg = make_config(3, DietType::VEGETARIAN);
    // R004 contains chicken (Meat) — should be incompatible
    auto plan = make_plan_all(ds, "R001","R002","R004");
    auto eval = PlanEvaluator::evaluate(plan, ds, cfg, OptimizationMode::NUTRITION);
    EASSERT(!eval.is_feasible, "Vegetarian incompatibility detected for chicken recipe");
}

static void test_inventory_deduction() {
    std::cout << "\n[test_inventory_deduction]\n";
    auto ds  = make_eval_dataset();
    // Add inventory: 1000g of Rice
    ds.inventory_map["I003"] = 1000.0;
    auto cfg = make_config();
    auto plan = make_plan_all(ds, "R001","R002","R003");
    auto eval = PlanEvaluator::evaluate(plan, ds, cfg, OptimizationMode::BUDGET);

    // Rice consumed = 150g * 7 = 1050g; inventory = 1000g; purchase = 50g
    bool found_rice = false;
    for (const auto& gi : eval.grocery_list) {
        if (gi.ingredient_id == "I003") {
            found_rice = true;
            EASSERT(std::abs(gi.total_required_g - 1050.0) < 1.0, "Rice total = 1050g");
            EASSERT(std::abs(gi.available_g - 1000.0) < 1.0, "Rice inventory = 1000g");
            EASSERT(std::abs(gi.purchase_g - 50.0) < 1.0, "Rice purchase = 50g");
        }
    }
    EASSERT(found_rice, "Rice found in grocery list");
}

static void test_excluded_ingredient() {
    std::cout << "\n[test_excluded_ingredient]\n";
    auto ds  = make_eval_dataset();
    auto cfg = make_config();
    cfg.excluded_ingredient_ids = {"I007"}; // exclude chicken

    // Recipe R004 uses I007 (chicken) — should fail compatibility
    EASSERT(!PlanEvaluator::recipe_compatible(ds.calc_recipe_map["R004"], ds, cfg),
            "R004 (Chicken Rice) incompatible with excluded chicken");
    EASSERT(PlanEvaluator::recipe_compatible(ds.calc_recipe_map["R002"], ds, cfg),
            "R002 (Dal Rice) compatible with excluded chicken");
}

int run_evaluator_tests() {
    std::cout << "=== EVALUATOR TESTS ===\n";
    test_feasible_plan();
    test_budget_violation();
    test_fat_violation();
    test_vegetarian_incompatibility();
    test_inventory_deduction();
    test_excluded_ingredient();
    return etest_passed == etest_run ? 0 : 1;
}

