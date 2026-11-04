// excel_reader.cpp — Excel (.xlsx) dataset reader via OpenXLSX

#include "excel_reader.h"
#include <OpenXLSX.hpp>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

using namespace OpenXLSX;

// ─── Helpers ───────────────────────────────────────────────────────────────

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string to_lower_clean(const std::string& s) {
    std::string res;
    for (char c : s) {
        if (c != ' ' && c != '_' && c != '-')
            res += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return res;
}

static std::string find_sheet_name(const XLWorkbook& wb, const std::string& target) {
    std::string clean_target = to_lower_clean(target);
    for (const auto& name : wb.worksheetNames()) {
        if (to_lower_clean(name) == clean_target)
            return name;
    }
    return "";
}

static std::string cell_string(const XLCell& cell) {
    auto t = cell.value().type();
    switch (t) {
        case XLValueType::String:
            return trim(cell.value().get<std::string>());
        case XLValueType::Integer:
            return std::to_string(cell.value().get<int64_t>());
        case XLValueType::Float: {
            double d = cell.value().get<double>();
            if (std::floor(d) == d) return std::to_string(static_cast<int64_t>(d));
            return std::to_string(d);
        }
        case XLValueType::Boolean:
            return cell.value().get<bool>() ? "true" : "false";
        default:
            return "";
    }
}

static double cell_double(const XLCell& cell, double default_val = 0.0) {
    auto t = cell.value().type();
    switch (t) {
        case XLValueType::Float:
            return cell.value().get<double>();
        case XLValueType::Integer:
            return static_cast<double>(cell.value().get<int64_t>());
        case XLValueType::String: {
            std::string s = trim(cell.value().get<std::string>());
            if (s.empty()) return default_val;
            try { return std::stod(s); } catch (...) { return default_val; }
        }
        case XLValueType::Boolean:
            return cell.value().get<bool>() ? 1.0 : 0.0;
        default:
            return default_val;
    }
}

static std::unordered_map<std::string, uint16_t> build_header_map(const XLWorksheet& ws) {
    std::unordered_map<std::string, uint16_t> header_map;
    uint16_t col_count = ws.columnCount();
    for (uint16_t c = 1; c <= col_count; ++c) {
        std::string h = cell_string(ws.cell(1, c));
        if (!h.empty()) {
            header_map[to_lower_clean(h)] = c;
        }
    }
    return header_map;
}

static std::string get_field_str(const XLWorksheet& ws, uint32_t row,
                                 const std::unordered_map<std::string, uint16_t>& map,
                                 const std::string& col_name)
{
    auto it = map.find(to_lower_clean(col_name));
    if (it == map.end()) return "";
    return cell_string(ws.cell(row, it->second));
}

static double get_field_double(const XLWorksheet& ws, uint32_t row,
                               const std::unordered_map<std::string, uint16_t>& map,
                               const std::string& col_name,
                               double default_val = 0.0)
{
    auto it = map.find(to_lower_clean(col_name));
    if (it == map.end()) return default_val;
    return cell_double(ws.cell(row, it->second), default_val);
}

// ─── Main load function ────────────────────────────────────────────────────

Dataset ExcelReader::load_from_excel(const std::string& filepath, std::string& error) {
    Dataset ds;
    ds.source_name = filepath;

    try {
        XLDocument doc;
        doc.open(filepath);
        auto wb = doc.workbook();

        // 1. Ingredients sheet (required)
        std::string ing_sheet = find_sheet_name(wb, "Ingredients");
        if (ing_sheet.empty()) {
            error = "Missing required worksheet: Ingredients";
            return ds;
        }
        {
            auto ws = wb.worksheet(ing_sheet);
            auto headers = build_header_map(ws);
            uint32_t row_count = ws.rowCount();
            for (uint32_t r = 2; r <= row_count; ++r) {
                Ingredient ing;
                ing.ingredient_id      = get_field_str(ws, r, headers, "Ingredient_ID");
                ing.name               = get_field_str(ws, r, headers, "Name");
                ing.price_per_kg_inr   = get_field_double(ws, r, headers, "Price_per_kg_INR");
                ing.calories_per_100g  = get_field_double(ws, r, headers, "Calories_per_100g");
                ing.protein_g_per_100g = get_field_double(ws, r, headers, "Protein_g_per_100g");
                ing.carbs_g_per_100g   = get_field_double(ws, r, headers, "Carbs_g_per_100g");
                ing.fat_g_per_100g     = get_field_double(ws, r, headers, "Fat_g_per_100g");
                ing.diet_category      = get_field_str(ws, r, headers, "Diet_Category");

                // Only add if at least ID or Name is non-empty
                if (!ing.ingredient_id.empty() || !ing.name.empty()) {
                    ds.ingredients.push_back(ing);
                    if (!ing.ingredient_id.empty()) {
                        ds.ingredient_map[ing.ingredient_id] = ing;
                    }
                }
            }
        }

        // 2. Recipes sheet (required)
        std::string rec_sheet = find_sheet_name(wb, "Recipes");
        if (rec_sheet.empty()) {
            error = "Missing required worksheet: Recipes";
            return ds;
        }
        {
            auto ws = wb.worksheet(rec_sheet);
            auto headers = build_header_map(ws);
            uint32_t row_count = ws.rowCount();
            for (uint32_t r = 2; r <= row_count; ++r) {
                Recipe recipe;
                recipe.recipe_id   = get_field_str(ws, r, headers, "Recipe_ID");
                recipe.recipe_name = get_field_str(ws, r, headers, "Recipe_Name");
                recipe.meal_type   = get_field_str(ws, r, headers, "Meal_Type");

                if (!recipe.recipe_id.empty() || !recipe.recipe_name.empty()) {
                    ds.recipes.push_back(recipe);
                    if (!recipe.recipe_id.empty()) {
                        ds.recipe_map[recipe.recipe_id] = recipe;
                    }
                }
            }
        }

        // 3. Recipe_Ingredients sheet (required)
        std::string ri_sheet = find_sheet_name(wb, "Recipe_Ingredients");
        if (ri_sheet.empty()) {
            ri_sheet = find_sheet_name(wb, "RecipeIngredients");
        }
        if (ri_sheet.empty()) {
            error = "Missing required worksheet: Recipe_Ingredients";
            return ds;
        }
        {
            auto ws = wb.worksheet(ri_sheet);
            auto headers = build_header_map(ws);
            uint32_t row_count = ws.rowCount();
            for (uint32_t r = 2; r <= row_count; ++r) {
                RecipeIngredient ri;
                ri.recipe_id              = get_field_str(ws, r, headers, "Recipe_ID");
                ri.ingredient_id          = get_field_str(ws, r, headers, "Ingredient_ID");
                ri.quantity_g_per_serving = get_field_double(ws, r, headers, "Quantity_g_per_serving");

                if (!ri.recipe_id.empty() && !ri.ingredient_id.empty()) {
                    ds.recipe_ingredients.push_back(ri);
                }
            }
        }

        // 4. Inventory sheet (optional)
        std::string inv_sheet = find_sheet_name(wb, "Inventory");
        if (!inv_sheet.empty()) {
            auto ws = wb.worksheet(inv_sheet);
            auto headers = build_header_map(ws);
            uint32_t row_count = ws.rowCount();
            for (uint32_t r = 2; r <= row_count; ++r) {
                InventoryItem item;
                item.ingredient_id        = get_field_str(ws, r, headers, "Ingredient_ID");
                item.available_quantity_g = get_field_double(ws, r, headers, "Available_Quantity_g");

                if (!item.ingredient_id.empty()) {
                    ds.inventory.push_back(item);
                    ds.inventory_map[item.ingredient_id] = item.available_quantity_g;
                }
            }
        }

        doc.close();
        ds.is_loaded = true;
        return ds;

    } catch (const std::exception& ex) {
        error = std::string("Failed to read Excel file: ") + ex.what();
        return ds;
    }
}

