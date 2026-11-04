#pragma once
// optimization_config.h — User-configurable optimization settings

#include <string>
#include <vector>
#include <unordered_set>

// Diet type enum
enum class DietType {
    VEGETARIAN,
    VEGAN,
    NON_VEGETARIAN,
    UNRESTRICTED
};

// Optimization mode
enum class OptimizationMode {
    NUTRITION,   // minimize deviation from explicit daily targets
    BUDGET,      // minimize weekly purchase cost
    VARIETY      // maximize distinct recipe count
};

// Hard daily nutritional constraints
struct NutritionConstraints {
    double min_calories  = 1500.0;
    double max_calories  = 2500.0;
    double min_protein   = 50.0;
    double min_carbs     = 150.0;
    double max_fat       = 80.0;
};

// Explicit preferred daily targets (mandatory for Nutrition mode)
struct NutritionTargets {
    double target_calories  = 2000.0;
    double target_protein   = 75.0;
    double target_carbs     = 250.0;
    double target_fat       = 60.0;
};

struct OptimizationConfig {
    // Weekly settings
    double weekly_budget_inr          = 2000.0;
    int    meals_per_day              = 3;       // 2, 3, or 4
    int    max_recipe_repetitions     = 3;

    // Diet
    DietType diet_type = DietType::UNRESTRICTED;

    // Hard nutritional constraints
    NutritionConstraints constraints;

    // Explicit daily targets (kept separate from hard constraints)
    NutritionTargets targets;

    // Ingredient preferences
    std::vector<std::string> excluded_ingredient_ids;
    std::vector<std::string> preferred_ingredient_ids;

    // Derived: meal slots for chosen meals_per_day
    // 2 -> [Breakfast, Dinner]
    // 3 -> [Breakfast, Lunch, Dinner]
    // 4 -> [Breakfast, Lunch, Snack, Dinner]
    std::vector<std::string> meal_slots() const {
        switch (meals_per_day) {
            case 2: return {"Breakfast", "Dinner"};
            case 3: return {"Breakfast", "Lunch", "Dinner"};
            case 4: return {"Breakfast", "Lunch", "Snack", "Dinner"};
            default: return {"Breakfast", "Lunch", "Dinner"};
        }
    }
};

// Convert DietType <-> string
inline std::string diet_type_to_string(DietType dt) {
    switch (dt) {
        case DietType::VEGETARIAN:     return "Vegetarian";
        case DietType::VEGAN:          return "Vegan";
        case DietType::NON_VEGETARIAN: return "Non-vegetarian";
        case DietType::UNRESTRICTED:   return "Unrestricted";
        default:                       return "Unrestricted";
    }
}

inline DietType string_to_diet_type(const std::string& s) {
    if (s == "Vegetarian")     return DietType::VEGETARIAN;
    if (s == "Vegan")          return DietType::VEGAN;
    if (s == "Non-vegetarian") return DietType::NON_VEGETARIAN;
    return DietType::UNRESTRICTED;
}

inline std::string optimization_mode_to_string(OptimizationMode m) {
    switch (m) {
        case OptimizationMode::NUTRITION: return "Nutrition";
        case OptimizationMode::BUDGET:    return "Budget";
        case OptimizationMode::VARIETY:   return "Variety";
        default:                          return "Nutrition";
    }
}

inline OptimizationMode string_to_optimization_mode(const std::string& s) {
    if (s == "Budget")  return OptimizationMode::BUDGET;
    if (s == "Variety") return OptimizationMode::VARIETY;
    return OptimizationMode::NUTRITION;
}

