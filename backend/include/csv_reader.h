#pragma once
// csv_reader.h — Lightweight CSV parser

#include "models.h"
#include <string>
#include <vector>
#include <unordered_map>

struct CsvRow {
    std::unordered_map<std::string, std::string> fields;
};

class CsvReader {
public:
    // Parse a CSV file. Returns rows with named columns.
    static std::vector<CsvRow> parse_file(const std::string& filepath, std::string& error);

    // Parse CSV text directly (for testing).
    static std::vector<CsvRow> parse_string(const std::string& csv_text, std::string& error);

    // Load dataset from four CSV files.
    // ingredients_path, recipes_path, recipe_ingredients_path are required.
    // inventory_path is optional (empty string = skip).
    static Dataset load_from_csv(
        const std::string& ingredients_path,
        const std::string& recipes_path,
        const std::string& recipe_ingredients_path,
        const std::string& inventory_path,
        std::string& error
    );

private:
    static std::vector<CsvRow> parse_stream(std::istream& stream, std::string& error);
};

