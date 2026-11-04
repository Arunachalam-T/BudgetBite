#pragma once
// models.h — Core data structures for SmartMeal Optimizer

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

// ─── Raw dataset structures ────────────────────────────────────────────────

struct Ingredient {
    std::string ingredient_id;
    std::string name;
    double price_per_kg_inr      = 0.0;
    double calories_per_100g     = 0.0;
    double protein_g_per_100g    = 0.0;
    double carbs_g_per_100g      = 0.0;
    double fat_g_per_100g        = 0.0;
    std::string diet_category;   // Plant-based | Dairy | Eggs | Meat | Fish/Seafood | Animal-derived
};

struct Recipe {
    std::string recipe_id;
    std::string recipe_name;
    std::string meal_type;       // Breakfast | Lunch | Snack | Dinner
};

struct RecipeIngredient {
    std::string recipe_id;
    std::string ingredient_id;
    double quantity_g_per_serving = 0.0;
};

struct InventoryItem {
    std::string ingredient_id;
    double available_quantity_g = 0.0;
};

// ─── Calculated recipe (computed after loading) ───────────────────────────

struct CalculatedRecipe {
    std::string recipe_id;
    std::string recipe_name;
    std::string meal_type;

    double calories_per_serving    = 0.0;
    double protein_per_serving     = 0.0;
    double carbs_per_serving       = 0.0;
    double fat_per_serving         = 0.0;
    double gross_cost_per_serving  = 0.0; // sum of ingredient costs (no inventory deduction)

    std::vector<RecipeIngredient> ingredients;
};

// ─── Dataset (all loaded and indexed data) ───────────────────────────────

struct Dataset {
    std::vector<Ingredient>             ingredients;
    std::vector<Recipe>                 recipes;
    std::vector<RecipeIngredient>       recipe_ingredients;
    std::vector<InventoryItem>          inventory;
    std::vector<CalculatedRecipe>       calculated_recipes;

    // Fast lookups
    std::unordered_map<std::string, Ingredient>         ingredient_map;   // id -> ingredient
    std::unordered_map<std::string, Recipe>             recipe_map;       // id -> recipe
    std::unordered_map<std::string, CalculatedRecipe>   calc_recipe_map;  // id -> calc recipe
    std::unordered_map<std::string, double>             inventory_map;    // id -> qty_g

    bool is_loaded   = false;
    bool is_valid    = false;
    std::string source_name; // filename used to load
};

// ─── Validation result ────────────────────────────────────────────────────

struct ValidationMessage {
    enum class Severity { FATAL, WARNING };
    Severity    severity;
    std::string worksheet;
    int         row = -1;         // -1 if not row-specific
    std::string message;
};

struct ValidationResult {
    bool                          is_valid = false;
    std::vector<ValidationMessage> messages;

    bool has_fatal() const {
        for (auto& m : messages)
            if (m.severity == ValidationMessage::Severity::FATAL)
                return true;
        return false;
    }
};

// ─── Meal plan structures ─────────────────────────────────────────────────

struct MealSlotAssignment {
    int         day;          // 0-6
    std::string meal_type;    // Breakfast | Lunch | Snack | Dinner
    std::string recipe_id;
    int         servings = 1;
};

struct MealPlan {
    std::vector<MealSlotAssignment> assignments;  // 7 days * meals_per_day entries
    bool is_empty() const { return assignments.empty(); }
};

// ─── Grocery item ─────────────────────────────────────────────────────────

struct GroceryItem {
    std::string ingredient_id;
    std::string ingredient_name;
    double total_required_g    = 0.0;
    double available_g         = 0.0;
    double purchase_g          = 0.0;
    double price_per_kg_inr    = 0.0;
    double purchase_cost_inr   = 0.0;
};

// ─── Evaluation result ───────────────────────────────────────────────────

struct EvaluationResult {
    bool   is_feasible = false;
    std::vector<std::string> violation_messages;

    double weekly_purchase_cost = 0.0;
    double gross_recipe_cost    = 0.0;

    // [7] daily values
    std::vector<double> daily_calories;   // size 7
    std::vector<double> daily_protein;
    std::vector<double> daily_carbs;
    std::vector<double> daily_fat;

    double weekly_calories  = 0.0;
    double weekly_protein   = 0.0;
    double weekly_carbs     = 0.0;
    double weekly_fat       = 0.0;

    std::unordered_map<std::string, double> ingredient_consumption; // id -> total g consumed
    std::unordered_map<std::string, double> ingredient_purchased;   // id -> purchase qty g
    std::vector<GroceryItem>                grocery_list;

    std::unordered_map<std::string, int> recipe_repetitions; // recipe_id -> count
    int    distinct_recipe_count = 0;

    double nutrition_deviation_score = 0.0;  // lower is better (Nutrition mode)
    double variety_score             = 0.0;  // 0-100 (Variety mode)
    double primary_objective_score   = 0.0;  // mode-specific

    // Daily deviation per nutrient (size 7)
    std::vector<double> daily_nutrition_deviation;
};

