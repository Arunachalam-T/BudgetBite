// data_loader.cpp — Unified loading entry point

#include "data_loader.h"
#include "csv_reader.h"
#include "excel_reader.h"
#include "validator.h"
#include "recipe_calculator.h"

Dataset DataLoader::load_excel(const std::string& filepath, std::string& error) {
    return ExcelReader::load_from_excel(filepath, error);
}

Dataset DataLoader::load_csv(
    const std::string& ingredients_path,
    const std::string& recipes_path,
    const std::string& recipe_ingredients_path,
    const std::string& inventory_path,
    std::string& error)
{
    return CsvReader::load_from_csv(
        ingredients_path, recipes_path, recipe_ingredients_path,
        inventory_path, error);
}

bool DataLoader::post_process(Dataset& dataset, ValidationResult& validation) {
    validation = Validator::validate(dataset);
    if (validation.has_fatal()) {
        dataset.is_valid = false;
        return false;
    }
    RecipeCalculator::calculate(dataset);
    dataset.is_valid = true;
    return true;
}

