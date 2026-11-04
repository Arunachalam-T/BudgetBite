// validator.cpp — Dataset and config validation

#include "validator.h"
#include <unordered_set>
#include <sstream>
#include <cmath>

// Allowed dietary categories
static const std::unordered_set<std::string> VALID_DIET_CATEGORIES = {
    "Plant-based", "Dairy", "Eggs", "Meat", "Fish/Seafood", "Animal-derived"
};

// Allowed meal types
static const std::unordered_set<std::string> VALID_MEAL_TYPES = {
    "Breakfast", "Lunch", "Snack", "Dinner"
};

// ─── Helper additions ──────────────────────────────────────────────────────

void Validator::add_fatal(std::vector<ValidationMessage>& msgs,
                          const std::string& ws, int row,
                          const std::string& msg) {
    msgs.push_back({ValidationMessage::Severity::FATAL, ws, row, msg});
}

void Validator::add_warning(std::vector<ValidationMessage>& msgs,
                            const std::string& ws, int row,
                            const std::string& msg) {
    msgs.push_back({ValidationMessage::Severity::WARNING, ws, row, msg});
}

// ─── Ingredient validation ─────────────────────────────────────────────────

void Validator::validate_ingredients(const Dataset& ds, std::vector<ValidationMessage>& msgs) {
    std::unordered_set<std::string> seen_ids;
    int row = 2; // 1-based, assuming row 1 is header
    for (const auto& ing : ds.ingredients) {
        if (ing.ingredient_id.empty())
            add_fatal(msgs, "Ingredients", row, "Empty Ingredient_ID.");
        else if (seen_ids.count(ing.ingredient_id))
            add_fatal(msgs, "Ingredients", row, "Duplicate Ingredient_ID: " + ing.ingredient_id);
        else
            seen_ids.insert(ing.ingredient_id);

        if (ing.name.empty())
            add_fatal(msgs, "Ingredients", row, "Empty Name for ingredient: " + ing.ingredient_id);

        if (ing.price_per_kg_inr < 0.0)
            add_fatal(msgs, "Ingredients", row, "Negative Price_per_kg_INR for: " + ing.ingredient_id);

        if (ing.calories_per_100g < 0.0)
            add_fatal(msgs, "Ingredients", row, "Negative Calories_per_100g for: " + ing.ingredient_id);

        if (ing.protein_g_per_100g < 0.0)
            add_fatal(msgs, "Ingredients", row, "Negative Protein_g_per_100g for: " + ing.ingredient_id);

        if (ing.carbs_g_per_100g < 0.0)
            add_fatal(msgs, "Ingredients", row, "Negative Carbs_g_per_100g for: " + ing.ingredient_id);

        if (ing.fat_g_per_100g < 0.0)
            add_fatal(msgs, "Ingredients", row, "Negative Fat_g_per_100g for: " + ing.ingredient_id);

        if (VALID_DIET_CATEGORIES.find(ing.diet_category) == VALID_DIET_CATEGORIES.end())
            add_fatal(msgs, "Ingredients", row,
                      "Invalid or missing Diet_Category '" + ing.diet_category +
                      "' for ingredient: " + ing.ingredient_id);
        ++row;
    }

    if (ds.ingredients.empty())
        add_fatal(msgs, "Ingredients", -1, "Ingredients sheet is empty or missing.");
}

// ─── Recipe validation ─────────────────────────────────────────────────────

void Validator::validate_recipes(const Dataset& ds, std::vector<ValidationMessage>& msgs) {
    std::unordered_set<std::string> seen_ids;
    int row = 2;
    for (const auto& r : ds.recipes) {
        if (r.recipe_id.empty())
            add_fatal(msgs, "Recipes", row, "Empty Recipe_ID.");
        else if (seen_ids.count(r.recipe_id))
            add_fatal(msgs, "Recipes", row, "Duplicate Recipe_ID: " + r.recipe_id);
        else
            seen_ids.insert(r.recipe_id);

        if (r.recipe_name.empty())
            add_fatal(msgs, "Recipes", row, "Empty Recipe_Name for: " + r.recipe_id);

        if (VALID_MEAL_TYPES.find(r.meal_type) == VALID_MEAL_TYPES.end())
            add_fatal(msgs, "Recipes", row,
                      "Invalid Meal_Type '" + r.meal_type + "' for recipe: " + r.recipe_id);
        ++row;
    }

    if (ds.recipes.empty())
        add_fatal(msgs, "Recipes", -1, "Recipes sheet is empty or missing.");
}

// ─── Recipe_Ingredients validation ────────────────────────────────────────

void Validator::validate_recipe_ingredients(const Dataset& ds, std::vector<ValidationMessage>& msgs) {
    std::unordered_set<std::string> seen_pairs; // recipe_id + ":" + ingredient_id
    int row = 2;
    for (const auto& ri : ds.recipe_ingredients) {
        if (ri.recipe_id.empty())
            add_fatal(msgs, "Recipe_Ingredients", row, "Empty Recipe_ID.");
        if (ri.ingredient_id.empty())
            add_fatal(msgs, "Recipe_Ingredients", row, "Empty Ingredient_ID.");

        if (ri.quantity_g_per_serving < 0.0)
            add_fatal(msgs, "Recipe_Ingredients", row,
                      "Negative quantity for Recipe: " + ri.recipe_id +
                      ", Ingredient: " + ri.ingredient_id);

        if (ri.quantity_g_per_serving == 0.0)
            add_warning(msgs, "Recipe_Ingredients", row,
                        "Zero quantity for Recipe: " + ri.recipe_id +
                        ", Ingredient: " + ri.ingredient_id);

        std::string pair_key = ri.recipe_id + ":" + ri.ingredient_id;
        if (seen_pairs.count(pair_key))
            add_fatal(msgs, "Recipe_Ingredients", row,
                      "Duplicate recipe-ingredient mapping: " + pair_key);
        else
            seen_pairs.insert(pair_key);
        ++row;
    }
}

// ─── Inventory validation ─────────────────────────────────────────────────

void Validator::validate_inventory(const Dataset& ds, std::vector<ValidationMessage>& msgs) {
    std::unordered_set<std::string> seen_ids;
    int row = 2;
    for (const auto& item : ds.inventory) {
        if (item.ingredient_id.empty())
            add_fatal(msgs, "Inventory", row, "Empty Ingredient_ID.");
        else if (seen_ids.count(item.ingredient_id))
            add_fatal(msgs, "Inventory", row, "Duplicate Ingredient_ID: " + item.ingredient_id);
        else
            seen_ids.insert(item.ingredient_id);

        if (item.available_quantity_g < 0.0)
            add_fatal(msgs, "Inventory", row,
                      "Negative Available_Quantity_g for: " + item.ingredient_id);
        ++row;
    }
}

// ─── Cross-reference validation ───────────────────────────────────────────

void Validator::validate_cross_references(const Dataset& ds, std::vector<ValidationMessage>& msgs) {
    // Build sets for fast lookup
    std::unordered_set<std::string> ingredient_ids, recipe_ids;
    for (const auto& i : ds.ingredients) ingredient_ids.insert(i.ingredient_id);
    for (const auto& r : ds.recipes)     recipe_ids.insert(r.recipe_id);

    // Recipe_Ingredients references
    for (const auto& ri : ds.recipe_ingredients) {
        if (!recipe_ids.count(ri.recipe_id))
            add_fatal(msgs, "Recipe_Ingredients", -1,
                      "Unknown Recipe_ID reference: " + ri.recipe_id);
        if (!ingredient_ids.count(ri.ingredient_id))
            add_fatal(msgs, "Recipe_Ingredients", -1,
                      "Unknown Ingredient_ID reference: " + ri.ingredient_id +
                      " in recipe: " + ri.recipe_id);
    }

    // Inventory references
    for (const auto& item : ds.inventory) {
        if (!ingredient_ids.count(item.ingredient_id))
            add_warning(msgs, "Inventory", -1,
                        "Unknown Ingredient_ID in inventory: " + item.ingredient_id);
    }

    // Check for recipes with no ingredients
    std::unordered_set<std::string> recipes_with_ingredients;
    for (const auto& ri : ds.recipe_ingredients)
        recipes_with_ingredients.insert(ri.recipe_id);

    for (const auto& r : ds.recipes) {
        if (!recipes_with_ingredients.count(r.recipe_id))
            add_fatal(msgs, "Recipes", -1,
                      "Recipe has no ingredients: " + r.recipe_id + " (" + r.recipe_name + ")");
    }
}

// ─── Main validate ────────────────────────────────────────────────────────

ValidationResult Validator::validate(const Dataset& ds) {
    ValidationResult result;
    auto& msgs = result.messages;

    validate_ingredients(ds, msgs);
    validate_recipes(ds, msgs);
    validate_recipe_ingredients(ds, msgs);
    validate_inventory(ds, msgs);
    validate_cross_references(ds, msgs);

    result.is_valid = !result.has_fatal();
    return result;
}

// ─── Config validation ────────────────────────────────────────────────────

ValidationResult Validator::validate_config(const OptimizationConfig& cfg,
                                            const Dataset& ds) {
    ValidationResult result;
    auto& msgs = result.messages;

    if (cfg.weekly_budget_inr <= 0)
        add_fatal(msgs, "Configuration", -1, "Weekly budget must be positive.");

    if (cfg.meals_per_day < 2 || cfg.meals_per_day > 4)
        add_fatal(msgs, "Configuration", -1, "Meals per day must be 2, 3, or 4.");

    if (cfg.max_recipe_repetitions < 1)
        add_fatal(msgs, "Configuration", -1, "Max recipe repetitions must be at least 1.");

    // Nutritional constraints
    const auto& c = cfg.constraints;
    if (c.min_calories < 0)
        add_fatal(msgs, "Configuration", -1, "Min calories cannot be negative.");
    if (c.max_calories <= c.min_calories)
        add_fatal(msgs, "Configuration", -1, "Max calories must be greater than min calories.");
    if (c.min_protein < 0)
        add_fatal(msgs, "Configuration", -1, "Min protein cannot be negative.");
    if (c.min_carbs < 0)
        add_fatal(msgs, "Configuration", -1, "Min carbs cannot be negative.");
    if (c.max_fat < 0)
        add_fatal(msgs, "Configuration", -1, "Max fat cannot be negative.");

    // Nutritional targets
    const auto& t = cfg.targets;
    if (t.target_calories <= 0)
        add_fatal(msgs, "Configuration", -1, "Target calories must be positive.");
    if (t.target_protein <= 0)
        add_fatal(msgs, "Configuration", -1, "Target protein must be positive.");
    if (t.target_carbs <= 0)
        add_fatal(msgs, "Configuration", -1, "Target carbs must be positive.");
    if (t.target_fat <= 0)
        add_fatal(msgs, "Configuration", -1, "Target fat must be positive.");

    if (t.target_calories < c.min_calories || t.target_calories > c.max_calories)
        add_fatal(msgs, "Configuration", -1,
                  "Target calories must be within [min_calories, max_calories].");
    if (t.target_protein < c.min_protein)
        add_fatal(msgs, "Configuration", -1, "Target protein must be >= min protein.");
    if (t.target_carbs < c.min_carbs)
        add_fatal(msgs, "Configuration", -1, "Target carbs must be >= min carbs.");
    if (t.target_fat > c.max_fat)
        add_fatal(msgs, "Configuration", -1, "Target fat must be <= max fat.");

    // Warn if excluded ingredient IDs are unknown
    for (const auto& eid : cfg.excluded_ingredient_ids) {
        if (!ds.ingredient_map.count(eid))
            add_warning(msgs, "Configuration", -1,
                        "Excluded ingredient ID not found in dataset: " + eid);
    }

    result.is_valid = !result.has_fatal();
    return result;
}

