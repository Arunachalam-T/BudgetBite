// csv_reader.cpp — Lightweight CSV parser (RFC 4180-compatible)

#include "csv_reader.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// ─── Internal CSV line parser ─────────────────────────────────────────────

static std::vector<std::string> parse_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i+1] == '"') {
                    field += '"';
                    ++i; // skip escaped quote
                } else {
                    in_quotes = false;
                }
            } else {
                field += c;
            }
        } else {
            if (c == '"') {
                in_quotes = true;
            } else if (c == ',') {
                fields.push_back(field);
                field.clear();
            } else {
                field += c;
            }
        }
    }
    fields.push_back(field);
    return fields;
}

// Trim whitespace from both ends
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// ─── Parse stream ────────────────────────────────────────────────────────

std::vector<CsvRow> CsvReader::parse_stream(std::istream& stream, std::string& error) {
    (void)error;
    std::vector<CsvRow> rows;
    std::string line;
    std::vector<std::string> headers;
    bool first = true;

    while (std::getline(stream, line)) {
        // Remove BOM if present
        if (!line.empty() && (unsigned char)line[0] == 0xEF) {
            if (line.size() >= 3 &&
                (unsigned char)line[1] == 0xBB &&
                (unsigned char)line[2] == 0xBF)
                line = line.substr(3);
        }
        // Remove trailing carriage return
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.empty()) continue;

        auto fields = parse_csv_line(line);

        if (first) {
            for (auto& h : fields) headers.push_back(trim(h));
            first = false;
        } else {
            CsvRow row;
            for (size_t i = 0; i < headers.size(); ++i) {
                std::string val = i < fields.size() ? trim(fields[i]) : "";
                row.fields[headers[i]] = val;
            }
            rows.push_back(row);
        }
    }
    return rows;
}

std::vector<CsvRow> CsvReader::parse_file(const std::string& filepath, std::string& error) {
    std::ifstream f(filepath);
    if (!f.is_open()) {
        error = "Cannot open file: " + filepath;
        return {};
    }
    return parse_stream(f, error);
}

std::vector<CsvRow> CsvReader::parse_string(const std::string& text, std::string& error) {
    std::istringstream iss(text);
    return parse_stream(iss, error);
}

// ─── Helper: get double field ─────────────────────────────────────────────

static double get_double(const CsvRow& row, const std::string& col, double default_val = 0.0) {
    auto it = row.fields.find(col);
    if (it == row.fields.end() || it->second.empty()) return default_val;
    try { return std::stod(it->second); }
    catch (...) { return default_val; }
}

static std::string get_str(const CsvRow& row, const std::string& col) {
    auto it = row.fields.find(col);
    if (it == row.fields.end()) return "";
    return it->second;
}

// ─── Load Dataset from CSV files ─────────────────────────────────────────

Dataset CsvReader::load_from_csv(
    const std::string& ingredients_path,
    const std::string& recipes_path,
    const std::string& recipe_ingredients_path,
    const std::string& inventory_path,
    std::string& error)
{
    Dataset ds;
    ds.source_name = ingredients_path;

    // Ingredients
    {
        auto rows = parse_file(ingredients_path, error);
        if (!error.empty()) return ds;
        for (const auto& row : rows) {
            Ingredient ing;
            ing.ingredient_id      = get_str(row, "Ingredient_ID");
            ing.name               = get_str(row, "Name");
            ing.price_per_kg_inr   = get_double(row, "Price_per_kg_INR");
            ing.calories_per_100g  = get_double(row, "Calories_per_100g");
            ing.protein_g_per_100g = get_double(row, "Protein_g_per_100g");
            ing.carbs_g_per_100g   = get_double(row, "Carbs_g_per_100g");
            ing.fat_g_per_100g     = get_double(row, "Fat_g_per_100g");
            ing.diet_category      = get_str(row, "Diet_Category");
            if (!ing.ingredient_id.empty()) {
                ds.ingredients.push_back(ing);
                ds.ingredient_map[ing.ingredient_id] = ing;
            }
        }
    }

    // Recipes
    {
        auto rows = parse_file(recipes_path, error);
        if (!error.empty()) return ds;
        for (const auto& row : rows) {
            Recipe r;
            r.recipe_id   = get_str(row, "Recipe_ID");
            r.recipe_name = get_str(row, "Recipe_Name");
            r.meal_type   = get_str(row, "Meal_Type");
            if (!r.recipe_id.empty()) {
                ds.recipes.push_back(r);
                ds.recipe_map[r.recipe_id] = r;
            }
        }
    }

    // Recipe_Ingredients
    {
        auto rows = parse_file(recipe_ingredients_path, error);
        if (!error.empty()) return ds;
        for (const auto& row : rows) {
            RecipeIngredient ri;
            ri.recipe_id             = get_str(row, "Recipe_ID");
            ri.ingredient_id         = get_str(row, "Ingredient_ID");
            ri.quantity_g_per_serving = get_double(row, "Quantity_g_per_serving");
            if (!ri.recipe_id.empty() && !ri.ingredient_id.empty())
                ds.recipe_ingredients.push_back(ri);
        }
    }

    // Inventory (optional)
    if (!inventory_path.empty()) {
        std::string inv_err;
        auto rows = parse_file(inventory_path, inv_err);
        if (inv_err.empty()) {
            for (const auto& row : rows) {
                InventoryItem item;
                item.ingredient_id      = get_str(row, "Ingredient_ID");
                item.available_quantity_g = get_double(row, "Available_Quantity_g");
                if (!item.ingredient_id.empty()) {
                    ds.inventory.push_back(item);
                    ds.inventory_map[item.ingredient_id] = item.available_quantity_g;
                }
            }
        }
        // Non-fatal if inventory file is missing
    }

    ds.is_loaded = true;
    return ds;
}

