// test_validator.cpp — Validation tests
#include "validator.h"
#include <cassert>
#include <iostream>

static int vtest_run = 0, vtest_passed = 0;
#define VASSERT(cond, msg) do { \
    ++vtest_run; \
    if (cond) { ++vtest_passed; std::cout << "  PASS: " << msg << "\n"; } \
    else { std::cout << "  FAIL: " << msg << "\n"; } \
} while(0)

static Dataset make_valid_dataset() {
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

    Recipe r;
    r.recipe_id = "R001"; r.recipe_name = "Dal Rice"; r.meal_type = "Lunch";
    ds.recipes.push_back(r);
    ds.recipe_map[r.recipe_id] = r;

    RecipeIngredient ri1;
    ri1.recipe_id = "R001"; ri1.ingredient_id = "I001"; ri1.quantity_g_per_serving = 150.0;
    ds.recipe_ingredients.push_back(ri1);

    RecipeIngredient ri2;
    ri2.recipe_id = "R001"; ri2.ingredient_id = "I002"; ri2.quantity_g_per_serving = 80.0;
    ds.recipe_ingredients.push_back(ri2);

    ds.is_loaded = true;
    return ds;
}

static void test_valid_dataset() {
    std::cout << "\n[test_valid_dataset]\n";
    auto ds = make_valid_dataset();
    auto vr = Validator::validate(ds);
    VASSERT(vr.is_valid, "Valid dataset passes");
    VASSERT(!vr.has_fatal(), "No fatal errors");
}

static void test_duplicate_ingredient_id() {
    std::cout << "\n[test_duplicate_ingredient_id]\n";
    auto ds = make_valid_dataset();
    // Add duplicate
    ds.ingredients.push_back(ds.ingredients[0]);
    auto vr = Validator::validate(ds);
    VASSERT(!vr.is_valid, "Duplicate ingredient ID detected");
}

static void test_invalid_diet_category() {
    std::cout << "\n[test_invalid_diet_category]\n";
    auto ds = make_valid_dataset();
    ds.ingredients[0].diet_category = "UNKNOWN";
    auto vr = Validator::validate(ds);
    VASSERT(!vr.is_valid, "Invalid diet_category detected");
}

static void test_recipe_without_ingredients() {
    std::cout << "\n[test_recipe_without_ingredients]\n";
    auto ds = make_valid_dataset();
    Recipe r2; r2.recipe_id = "R002"; r2.recipe_name = "Empty"; r2.meal_type = "Breakfast";
    ds.recipes.push_back(r2);
    ds.recipe_map[r2.recipe_id] = r2;
    auto vr = Validator::validate(ds);
    VASSERT(!vr.is_valid, "Recipe without ingredients detected");
}

static void test_unknown_ingredient_reference() {
    std::cout << "\n[test_unknown_ingredient_reference]\n";
    auto ds = make_valid_dataset();
    RecipeIngredient ri; ri.recipe_id = "R001"; ri.ingredient_id = "XBAD"; ri.quantity_g_per_serving = 10.0;
    ds.recipe_ingredients.push_back(ri);
    auto vr = Validator::validate(ds);
    VASSERT(!vr.is_valid, "Unknown ingredient reference detected");
}

static void test_negative_price() {
    std::cout << "\n[test_negative_price]\n";
    auto ds = make_valid_dataset();
    ds.ingredients[0].price_per_kg_inr = -5.0;
    auto vr = Validator::validate(ds);
    VASSERT(!vr.is_valid, "Negative price detected");
}

static void test_invalid_config_meals_per_day() {
    std::cout << "\n[test_invalid_config_meals_per_day]\n";
    auto ds = make_valid_dataset();
    OptimizationConfig cfg;
    cfg.meals_per_day = 1;
    auto vr1 = Validator::validate_config(cfg, ds);
    VASSERT(!vr1.is_valid && vr1.has_fatal(), "meals_per_day = 1 rejected");

    cfg.meals_per_day = 5;
    auto vr5 = Validator::validate_config(cfg, ds);
    VASSERT(!vr5.is_valid && vr5.has_fatal(), "meals_per_day = 5 rejected");

    cfg.meals_per_day = 3;
    cfg.weekly_budget_inr = 2000;
    cfg.constraints.min_calories = 1500;
    cfg.constraints.max_calories = 2500;
    cfg.targets.target_calories = 2000;
    cfg.constraints.min_protein = 50;
    cfg.targets.target_protein = 75;
    cfg.constraints.min_carbs = 100;
    cfg.targets.target_carbs = 200;
    cfg.constraints.max_fat = 80;
    cfg.targets.target_fat = 60;
    auto vrValid = Validator::validate_config(cfg, ds);
    VASSERT(vrValid.is_valid && !vrValid.has_fatal(), "valid config accepted");
}

static void test_invalid_config_budget_and_calories() {
    std::cout << "\n[test_invalid_config_budget_and_calories]\n";
    auto ds = make_valid_dataset();
    OptimizationConfig cfg;
    cfg.weekly_budget_inr = -500;
    cfg.constraints.min_calories = 2500;
    cfg.constraints.max_calories = 1500; // inverted
    auto vr = Validator::validate_config(cfg, ds);
    VASSERT(!vr.is_valid && vr.has_fatal(), "Negative budget and inverted calories rejected");
}

int run_validator_tests() {
    std::cout << "=== VALIDATOR TESTS ===\n";
    test_valid_dataset();
    test_duplicate_ingredient_id();
    test_invalid_diet_category();
    test_recipe_without_ingredients();
    test_unknown_ingredient_reference();
    test_negative_price();
    test_invalid_config_meals_per_day();
    test_invalid_config_budget_and_calories();
    return vtest_passed == vtest_run ? 0 : 1;
}

