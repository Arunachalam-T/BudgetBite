// test_exporter.cpp — Excel export tests (structural checks without file I/O)

#include "excel_exporter.h"
#include "recipe_calculator.h"
#include "plan_evaluator.h"
#include <iostream>
#include <fstream>
#include <cmath>

static int xtest_run = 0, xtest_passed = 0;
#define XASSERT(cond, msg) do { \
    ++xtest_run; \
    if (cond) { ++xtest_passed; std::cout << "  PASS: " << msg << "\n"; } \
    else { std::cout << "  FAIL: " << msg << "\n"; } \
} while(0)

static Dataset make_export_dataset() {
    Dataset ds;
    Ingredient ing;
    ing.ingredient_id = "I001"; ing.name = "Rice";
    ing.price_per_kg_inr = 42.0; ing.calories_per_100g = 350.0;
    ing.protein_g_per_100g = 7.0; ing.carbs_g_per_100g = 78.0;
    ing.fat_g_per_100g = 0.5; ing.diet_category = "Plant-based";
    ds.ingredients.push_back(ing);
    ds.ingredient_map[ing.ingredient_id] = ing;

    Ingredient ing2;
    ing2.ingredient_id = "I002"; ing2.name = "Dal";
    ing2.price_per_kg_inr = 90.0; ing2.calories_per_100g = 340.0;
    ing2.protein_g_per_100g = 22.0; ing2.carbs_g_per_100g = 60.0;
    ing2.fat_g_per_100g = 1.0; ing2.diet_category = "Plant-based";
    ds.ingredients.push_back(ing2);
    ds.ingredient_map[ing2.ingredient_id] = ing2;

    Ingredient ing3;
    ing3.ingredient_id = "I003"; ing3.name = "Oats";
    ing3.price_per_kg_inr = 80.0; ing3.calories_per_100g = 389.0;
    ing3.protein_g_per_100g = 17.0; ing3.carbs_g_per_100g = 66.0;
    ing3.fat_g_per_100g = 7.0; ing3.diet_category = "Plant-based";
    ds.ingredients.push_back(ing3);
    ds.ingredient_map[ing3.ingredient_id] = ing3;

    Ingredient ing4;
    ing4.ingredient_id = "I004"; ing4.name = "Roti";
    ing4.price_per_kg_inr = 33.0; ing4.calories_per_100g = 300.0;
    ing4.protein_g_per_100g = 11.0; ing4.carbs_g_per_100g = 60.0;
    ing4.fat_g_per_100g = 1.5; ing4.diet_category = "Plant-based";
    ds.ingredients.push_back(ing4);
    ds.ingredient_map[ing4.ingredient_id] = ing4;

    auto add_recipe = [&](const std::string& id, const std::string& name,
                          const std::string& meal,
                          const std::vector<std::pair<std::string,double>>& ings) {
        Recipe r; r.recipe_id = id; r.recipe_name = name; r.meal_type = meal;
        ds.recipes.push_back(r); ds.recipe_map[id] = r;
        for (const auto& [iid,qty] : ings) {
            RecipeIngredient ri; ri.recipe_id=id; ri.ingredient_id=iid; ri.quantity_g_per_serving=qty;
            ds.recipe_ingredients.push_back(ri);
        }
    };

    add_recipe("R001","Oat Porridge","Breakfast",{{"I003",80}});
    add_recipe("R002","Dal Rice","Lunch",{{"I001",150},{"I002",80}});
    add_recipe("R003","Roti","Dinner",{{"I004",120}});

    RecipeCalculator::calculate(ds);
    ds.is_loaded = true; ds.is_valid = true;
    return ds;
}

static void test_export_creates_file() {
    std::cout << "\n[test_export_creates_file]\n";
    auto ds = make_export_dataset();

    OptimizationConfig cfg;
    cfg.meals_per_day = 3; cfg.weekly_budget_inr = 1500.0;
    cfg.max_recipe_repetitions = 7; cfg.diet_type = DietType::UNRESTRICTED;
    cfg.constraints.min_calories=500; cfg.constraints.max_calories=3000;
    cfg.constraints.min_protein=10; cfg.constraints.min_carbs=50; cfg.constraints.max_fat=100;
    cfg.targets.target_calories=1800; cfg.targets.target_protein=60;
    cfg.targets.target_carbs=220; cfg.targets.target_fat=50;

    MealPlan plan;
    for (int d = 0; d < 7; ++d) {
        {MealSlotAssignment a; a.day=d; a.meal_type="Breakfast"; a.recipe_id="R001"; a.servings=1; plan.assignments.push_back(a);}
        {MealSlotAssignment a; a.day=d; a.meal_type="Lunch";     a.recipe_id="R002"; a.servings=1; plan.assignments.push_back(a);}
        {MealSlotAssignment a; a.day=d; a.meal_type="Dinner";    a.recipe_id="R003"; a.servings=1; plan.assignments.push_back(a);}
    }

    auto eval = PlanEvaluator::evaluate(plan, ds, cfg, OptimizationMode::NUTRITION);

    const std::string test_path = "test_output.xlsx";
    std::string err;
    bool ok = ExcelExporter::export_workbook(test_path, plan, eval, cfg, ds, "Nutrition", err);

    XASSERT(ok, "Export returned true");
    XASSERT(err.empty(), "No export error: " + err);

    // Check file exists
    std::ifstream f(test_path);
    XASSERT(f.good(), "Output file exists");
    f.close();

    // Clean up
    std::remove(test_path.c_str());
}

static void test_grocery_list_accounting() {
    std::cout << "\n[test_grocery_list_accounting]\n";
    auto ds = make_export_dataset();
    ds.inventory_map["I001"] = 500.0; // 500g rice in inventory

    OptimizationConfig cfg;
    cfg.meals_per_day = 3; cfg.weekly_budget_inr = 1500.0;
    cfg.max_recipe_repetitions = 7; cfg.diet_type = DietType::UNRESTRICTED;
    cfg.constraints.min_calories=500; cfg.constraints.max_calories=3000;
    cfg.constraints.min_protein=10; cfg.constraints.min_carbs=50; cfg.constraints.max_fat=100;
    cfg.targets.target_calories=1800; cfg.targets.target_protein=60;
    cfg.targets.target_carbs=220; cfg.targets.target_fat=50;

    MealPlan plan;
    for (int d = 0; d < 7; ++d) {
        {MealSlotAssignment a; a.day=d; a.meal_type="Breakfast"; a.recipe_id="R001"; a.servings=1; plan.assignments.push_back(a);}
        {MealSlotAssignment a; a.day=d; a.meal_type="Lunch";     a.recipe_id="R002"; a.servings=1; plan.assignments.push_back(a);}
        {MealSlotAssignment a; a.day=d; a.meal_type="Dinner";    a.recipe_id="R003"; a.servings=1; plan.assignments.push_back(a);}
    }

    auto eval = PlanEvaluator::evaluate(plan, ds, cfg, OptimizationMode::BUDGET);

    bool found = false;
    for (const auto& gi : eval.grocery_list) {
        if (gi.ingredient_id == "I001") {
            found = true;
            // 150g * 7 = 1050g total; 500g inventory; 550g purchase
            XASSERT(std::abs(gi.total_required_g - 1050.0) < 0.5, "Rice required = 1050g");
            XASSERT(std::abs(gi.available_g - 500.0) < 0.5, "Rice available = 500g");
            XASSERT(std::abs(gi.purchase_g - 550.0) < 0.5, "Rice purchase = 550g");
        }
    }
    XASSERT(found, "Rice found in grocery list");
}

int run_exporter_tests() {
    std::cout << "=== EXPORTER TESTS ===\n";
    test_export_creates_file();
    test_grocery_list_accounting();
    return xtest_passed == xtest_run ? 0 : 1;
}

