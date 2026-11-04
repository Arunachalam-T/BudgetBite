// test_calculator.cpp — Nutrition and cost calculation tests

#include "recipe_calculator.h"
#include <cassert>
#include <iostream>
#include <cmath>

static int ctest_run = 0, ctest_passed = 0;
#define CASSERT(cond, msg) do { \
    ++ctest_run; \
    if (cond) { ++ctest_passed; std::cout << "  PASS: " << msg << "\n"; } \
    else { std::cout << "  FAIL: " << msg << "\n"; } \
} while(0)

static bool approx(double a, double b, double eps = 0.001) {
    return std::abs(a - b) < eps;
}

// Recipe: 150g Rice + 80g Dal
// Rice: price=42/kg, cal=350/100g, prot=7, carb=78, fat=0.5
// Dal:  price=90/kg, cal=340/100g, prot=22, carb=60, fat=1.0
//
// Cost  = (150/1000)*42 + (80/1000)*90 = 6.3 + 7.2 = 13.5
// Cal   = (150/100)*350 + (80/100)*340 = 525 + 272 = 797
// Prot  = (150/100)*7   + (80/100)*22  = 10.5 + 17.6 = 28.1
// Carb  = (150/100)*78  + (80/100)*60  = 117 + 48 = 165
// Fat   = (150/100)*0.5 + (80/100)*1.0 = 0.75 + 0.8 = 1.55

static Dataset make_calc_dataset() {
    Dataset ds;

    Ingredient ing1;
    ing1.ingredient_id = "I001"; ing1.name = "Rice";
    ing1.price_per_kg_inr = 42.0; ing1.calories_per_100g = 350.0;
    ing1.protein_g_per_100g = 7.0; ing1.carbs_g_per_100g = 78.0;
    ing1.fat_g_per_100g = 0.5; ing1.diet_category = "Plant-based";
    ds.ingredients.push_back(ing1);
    ds.ingredient_map[ing1.ingredient_id] = ing1;

    Ingredient ing2;
    ing2.ingredient_id = "I002"; ing2.name = "Dal";
    ing2.price_per_kg_inr = 90.0; ing2.calories_per_100g = 340.0;
    ing2.protein_g_per_100g = 22.0; ing2.carbs_g_per_100g = 60.0;
    ing2.fat_g_per_100g = 1.0; ing2.diet_category = "Plant-based";
    ds.ingredients.push_back(ing2);
    ds.ingredient_map[ing2.ingredient_id] = ing2;

    Recipe r1;
    r1.recipe_id = "R001"; r1.recipe_name = "Dal Rice"; r1.meal_type = "Lunch";
    ds.recipes.push_back(r1);
    ds.recipe_map[r1.recipe_id] = r1;

    RecipeIngredient ri1; ri1.recipe_id = "R001"; ri1.ingredient_id = "I001"; ri1.quantity_g_per_serving = 150.0;
    RecipeIngredient ri2; ri2.recipe_id = "R001"; ri2.ingredient_id = "I002"; ri2.quantity_g_per_serving = 80.0;
    ds.recipe_ingredients.push_back(ri1);
    ds.recipe_ingredients.push_back(ri2);

    return ds;
}

static void test_recipe_cost_and_nutrition() {
    std::cout << "\n[test_recipe_cost_and_nutrition]\n";
    auto ds = make_calc_dataset();
    RecipeCalculator::calculate(ds);

    CASSERT(ds.calculated_recipes.size() == 1, "One calculated recipe");
    const auto& cr = ds.calculated_recipes[0];

    CASSERT(approx(cr.gross_cost_per_serving, 13.5), "Cost = 13.50 INR");
    CASSERT(approx(cr.calories_per_serving, 797.0), "Calories = 797");
    CASSERT(approx(cr.protein_per_serving, 28.1), "Protein = 28.1g");
    CASSERT(approx(cr.carbs_per_serving, 165.0), "Carbs = 165g");
    CASSERT(approx(cr.fat_per_serving, 1.55), "Fat = 1.55g");
}

static void test_fractional_quantities() {
    std::cout << "\n[test_fractional_quantities]\n";
    Dataset ds;
    Ingredient ing;
    ing.ingredient_id = "I003"; ing.name = "Oil";
    ing.price_per_kg_inr = 150.0; ing.calories_per_100g = 884.0;
    ing.protein_g_per_100g = 0.0; ing.carbs_g_per_100g = 0.0;
    ing.fat_g_per_100g = 100.0; ing.diet_category = "Plant-based";
    ds.ingredients.push_back(ing);
    ds.ingredient_map[ing.ingredient_id] = ing;

    Recipe r; r.recipe_id = "R002"; r.recipe_name = "Stir Fry"; r.meal_type = "Dinner";
    ds.recipes.push_back(r);
    ds.recipe_map[r.recipe_id] = r;

    RecipeIngredient ri; ri.recipe_id = "R002"; ri.ingredient_id = "I003"; ri.quantity_g_per_serving = 7.5;
    ds.recipe_ingredients.push_back(ri);

    RecipeCalculator::calculate(ds);
    const auto& cr = ds.calculated_recipes[0];

    // Cost = (7.5/1000)*150 = 1.125
    // Cal  = (7.5/100)*884  = 66.3
    // Fat  = (7.5/100)*100  = 7.5
    CASSERT(approx(cr.gross_cost_per_serving, 1.125), "Fractional cost correct");
    CASSERT(approx(cr.calories_per_serving, 66.3), "Fractional calories correct");
    CASSERT(approx(cr.fat_per_serving, 7.5), "Fractional fat correct");
}

int run_calculator_tests() {
    std::cout << "=== CALCULATOR TESTS ===\n";
    test_recipe_cost_and_nutrition();
    test_fractional_quantities();
    return ctest_passed == ctest_run ? 0 : 1;
}

