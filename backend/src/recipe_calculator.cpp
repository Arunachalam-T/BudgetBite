// recipe_calculator.cpp — Nutrition and cost computation

#include "recipe_calculator.h"
#include <unordered_map>
#include <vector>

CalculatedRecipe RecipeCalculator::calculate_one(
    const Recipe& recipe,
    const std::vector<RecipeIngredient>& ingredients,
    const std::unordered_map<std::string, Ingredient>& ingredient_map)
{
    CalculatedRecipe cr;
    cr.recipe_id   = recipe.recipe_id;
    cr.recipe_name = recipe.recipe_name;
    cr.meal_type   = recipe.meal_type;
    cr.ingredients = ingredients;

    for (const auto& ri : ingredients) {
        auto it = ingredient_map.find(ri.ingredient_id);
        if (it == ingredient_map.end()) continue;

        const Ingredient& ing = it->second;
        double qty_g = ri.quantity_g_per_serving;

        // Cost: qty_g / 1000 * price_per_kg
        cr.gross_cost_per_serving += (qty_g / 1000.0) * ing.price_per_kg_inr;

        // Nutrition: qty_g / 100 * nutrient_per_100g
        cr.calories_per_serving += (qty_g / 100.0) * ing.calories_per_100g;
        cr.protein_per_serving  += (qty_g / 100.0) * ing.protein_g_per_100g;
        cr.carbs_per_serving    += (qty_g / 100.0) * ing.carbs_g_per_100g;
        cr.fat_per_serving      += (qty_g / 100.0) * ing.fat_g_per_100g;
    }
    return cr;
}

void RecipeCalculator::calculate(Dataset& dataset) {
    // Build recipe -> ingredients mapping
    std::unordered_map<std::string, std::vector<RecipeIngredient>> ri_map;
    for (const auto& ri : dataset.recipe_ingredients)
        ri_map[ri.recipe_id].push_back(ri);

    dataset.calculated_recipes.clear();
    dataset.calc_recipe_map.clear();

    for (const auto& r : dataset.recipes) {
        auto& ri_list = ri_map[r.recipe_id];
        auto cr = calculate_one(r, ri_list, dataset.ingredient_map);
        dataset.calculated_recipes.push_back(cr);
        dataset.calc_recipe_map[cr.recipe_id] = cr;
    }
}

