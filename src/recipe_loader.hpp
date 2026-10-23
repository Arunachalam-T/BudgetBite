// =============================================================================
// BudgetBite - Affordable Meal Planning System
// File: src/recipe_loader.hpp
// Description: Loads and classifies recipes from the main CSV dataset
// =============================================================================
#pragma once

#include "types.hpp"
#include "csv_parser.hpp"
#include "json_lite.hpp"

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <set>

// ── Lower-case helper ─────────────────────────────────────────────────────────
static std::string to_lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// ── Keyword sets for meal-type classification ─────────────────────────────────
// Since there is no explicit meal-type column in the dataset, we classify
// recipes using recipe_id prefix and common name keywords.
//
// Prefix codes (observed):
//   ASC → Anuscript / Assorted (beverages, snacks, desserts)
//   BFP → Breakfast + Lunch + Dinner (main meals)
//   OSR → Other South-region / special recipes

static bool contains_any(const std::string& text,
                          const std::vector<std::string>& keywords) {
    std::string lo = to_lower(text);
    for (const auto& kw : keywords) {
        if (lo.find(kw) != std::string::npos) return true;
    }
    return false;
}

// Non-vegetarian ingredient names (heuristic)
static const std::vector<std::string> NON_VEG_KEYWORDS = {
    "chicken", "mutton", "beef", "pork", "fish", "prawn", "shrimp",
    "lamb", "egg", "meat", "keema", "mince", "biryani", "murg",
    "gosht", "machli", "maas", "tuna", "salmon", "crab"
};

static const std::vector<std::string> BEVERAGE_KEYWORDS = {
    "tea", "coffee", "juice", "drink", "punch", "lassi", "buttermilk",
    "milk shake", "milkshake", "sharbat", "panna", "cooler", "nimbu",
    "lemonade", "sherbet", "water", "cocoa", "cider"
};

static const std::vector<std::string> BREAKFAST_KEYWORDS = {
    "idli", "dosa", "upma", "poha", "paratha", "parantha", "thepla",
    "oats", "cornflakes", "porridge", "uttapam", "puttu", "appam",
    "ragi", "rava", "semolina", "dhokla", "bread", "toast", "egg",
    "sandwich", "pancake", "puri halwa", "seera", "sheera"
};

static const std::vector<std::string> DESSERT_KEYWORDS = {
    "halwa", "kheer", "payasam", "ladoo", "laddoo", "barfi", "burfi",
    "gulab jamun", "jalebi", "rasgulla", "rasmalai", "kulfi", "ice cream",
    "pudding", "cake", "sweet", "mithai", "modak"
};

static const std::vector<std::string> SNACK_KEYWORDS = {
    "chaat", "pakora", "bhajia", "vada", "samosa", "murukku", "chakli",
    "bhel", "sev", "popcorn", "namkeen", "chips", "roasted", "makhana",
    "peanut", "cutlet", "tikki", "biscuit"
};

static const std::vector<std::string> DINNER_KEYWORDS = {
    "dal", "sabzi", "curry", "rice", "roti", "chapati", "naan", "biryani",
    "pulao", "rajma", "chole", "paneer", "kadhai", "korma", "sambar",
    "rasam", "avial", "kootu", "poriyal", "khichdi"
};

// ── Classify a recipe's meal type ────────────────────────────────────────────
static void classify_recipe(Recipe& r) {
    const std::string& id   = r.recipe_id;
    const std::string& name = r.recipe_name;

    // Beverages
    if (id.substr(0,3) == "ASC" || contains_any(name, BEVERAGE_KEYWORDS)) {
        r.is_beverage = true;
        return;  // Beverages don't count as B/L/D meals
    }

    // Dessert / snack hints
    bool is_dessert = contains_any(name, DESSERT_KEYWORDS);
    bool is_snack   = contains_any(name, SNACK_KEYWORDS);

    // Breakfast indicators
    if (contains_any(name, BREAKFAST_KEYWORDS)) {
        r.is_breakfast = true;
    }

    // Main meal (lunch/dinner) indicators
    if (contains_any(name, DINNER_KEYWORDS)) {
        r.is_lunch  = true;
        r.is_dinner = true;
    }

    // Fallback: if nothing matched for BFP prefix, it can be any meal
    if (!r.is_breakfast && !r.is_lunch && !r.is_dinner && !is_dessert && !is_snack) {
        r.is_breakfast = true;
        r.is_lunch     = true;
        r.is_dinner    = true;
    }

    // OSR prefix – treat as lunch/dinner
    if (id.substr(0,3) == "OSR") {
        r.is_lunch  = true;
        r.is_dinner = true;
    }

    r.is_snack = is_snack || is_dessert;
}

// ── Dietary classification ────────────────────────────────────────────────────
static void classify_diet(Recipe& r) {
    bool has_egg = false;
    for (const auto& ing : r.ingredients) {
        std::string lo = to_lower(ing.name);
        if (lo.find("egg") != std::string::npos) has_egg = true;
        for (const auto& kw : NON_VEG_KEYWORDS) {
            if (lo.find(kw) != std::string::npos) {
                r.is_non_veg    = true;
                r.is_vegetarian = false;
                r.is_vegan      = false;
                return;
            }
        }
    }
    if (has_egg) {
        // Eggs make it non-vegetarian in strict Indian classification;
        // for this system we keep it as non-veg flag.
        r.is_non_veg    = true;
        r.is_vegetarian = false;
    } else {
        r.is_vegetarian = true;
        // Simple vegan check – no dairy / honey
        bool dairy = false;
        for (const auto& ing : r.ingredients) {
            std::string lo = to_lower(ing.name);
            if (lo.find("milk") != std::string::npos ||
                lo.find("ghee") != std::string::npos ||
                lo.find("butter") != std::string::npos ||
                lo.find("paneer") != std::string::npos ||
                lo.find("curd") != std::string::npos ||
                lo.find("yogurt") != std::string::npos ||
                lo.find("honey") != std::string::npos ||
                lo.find("cream") != std::string::npos) {
                dairy = true; break;
            }
        }
        r.is_vegan = !dairy;
    }
}

// ── Parse ingredients_json field into Ingredient structs ─────────────────────
static std::vector<Ingredient> parse_ingredients(const std::string& json_str) {
    std::vector<Ingredient> result;
    if (json_str.empty()) return result;

    JsonValue arr = json_try_parse(json_str);
    if (!arr.is_array()) return result;

    for (const auto& item : arr.as_array()) {
        if (!item.is_object()) continue;
        Ingredient ing;
        ing.name                = item["ingredient_name"].str();
        ing.amount              = item["amount"].num(0.0);
        ing.unit                = item["unit"].str();
        ing.cost_price_status   = item["cost_price_status"].str();
        ing.cost_price_commodity= item["cost_price_commodity"].str();

        std::string cost_str = item["cost_line_inr_per_recipe_batch"].str();
        if (!cost_str.empty()) {
            try { ing.cost_line_inr = std::stod(cost_str); } catch (...) {}
        }
        ing.is_priced = (ing.cost_price_status == "priced");
        result.push_back(ing);
    }
    return result;
}

// ── Main loader function ──────────────────────────────────────────────────────
inline std::vector<Recipe> load_recipes(const std::string& csv_path,
                                        int& parse_errors,
                                        int& missing_nutrition,
                                        int& missing_cost) {
    parse_errors      = 0;
    missing_nutrition = 0;
    missing_cost      = 0;

    CSVReader reader(csv_path);
    std::vector<Recipe> recipes;
    recipes.reserve(reader.row_count());

    for (size_t i = 0; i < reader.row_count(); ++i) {
        auto row = reader.get_row(i);
        Recipe r;

        r.recipe_id   = CSVReader::get(row, "recipe_id");
        r.recipe_name = CSVReader::get(row, "recipe_name");
        r.source      = CSVReader::get(row, "recipe_primarysource");
        r.serving_unit= CSVReader::get(row, "nutrition_serving_unit");

        std::string srv = CSVReader::get(row, "servings_in_recipe");
        if (!srv.empty()) { try { r.servings = std::stoi(srv); } catch (...) {} }
        if (r.servings < 1) r.servings = 1;

        // Nutrition per serving
        std::string nutr_status = CSVReader::get(row, "nutrient_data_status");
        if (nutr_status == "per_serving_available") {
            r.kcal_per_serving      = CSVReader::get_double(row, "energy_kcal_per_serving");
            r.protein_g_per_serving = CSVReader::get_double(row, "protein_g_per_serving");
            r.carb_g_per_serving    = CSVReader::get_double(row, "carb_g_per_serving");
            r.fat_g_per_serving     = CSVReader::get_double(row, "fat_g_per_serving");
            r.fibre_g_per_serving   = CSVReader::get_double(row, "fibre_g_per_serving");
            r.nutrition_available   = true;
        } else {
            ++missing_nutrition;
        }

        // Cost
        r.cost_status = CSVReader::get(row, "cost_status");
        r.cost_inr_per_serving = CSVReader::get_double(row, "cost_inr_per_serving");
        r.cost_subtotal_inr_per_serving = CSVReader::get_double(row, "priced_ingredients_subtotal_inr_per_serving");
        r.cost_coverage_pct  = CSVReader::get_double(row, "cost_coverage_pct");
        r.has_complete_cost  = (r.cost_status == "complete_estimate" && r.cost_inr_per_serving > 0);
        if (!r.has_any_cost_estimate()) ++missing_cost;

        // Ingredients
        std::string ing_json = CSVReader::get(row, "ingredients_json");
        try {
            r.ingredients = parse_ingredients(ing_json);
        } catch (...) {
            ++parse_errors;
        }

        classify_diet(r);
        classify_recipe(r);

        if (!r.recipe_id.empty()) {
            recipes.push_back(std::move(r));
        }
    }

    return recipes;
}
