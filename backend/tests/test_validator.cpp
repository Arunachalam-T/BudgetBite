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

int run_validator_tests() {
    std::cout << "=== VALIDATOR TESTS ===\n";
    test_valid_dataset();
    test_duplicate_ingredient_id();
    test_invalid_diet_category();
    test_recipe_without_ingredients();
    test_unknown_ingredient_reference();
    test_negative_price();
    return vtest_passed == vtest_run ? 0 : 1;
}

