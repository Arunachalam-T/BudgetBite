#pragma once
// recipe_calculator.h — Computes per-serving nutrition and cost for each recipe

#include "models.h"

class RecipeCalculator {
public:
    // Populate Dataset::calculated_recipes from raw loaded data.
    // Must be called after successful validation.
    static void calculate(Dataset& dataset);

    // Calculate a single recipe's nutrition/cost.
    static CalculatedRecipe calculate_one(
        const Recipe& recipe,
        const std::vector<RecipeIngredient>& ingredients,
        const std::unordered_map<std::string, Ingredient>& ingredient_map
    );
};

