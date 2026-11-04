#pragma once
// data_loader.h — Unified data loading entry point

#include "models.h"
#include <string>

class DataLoader {
public:
    // Load from uploaded Excel file (.xlsx).
    // Returns a Dataset; check dataset.is_valid after validation.
    static Dataset load_excel(const std::string& filepath, std::string& error);

    // Load from four separate CSV files (inventory optional).
    static Dataset load_csv(
        const std::string& ingredients_path,
        const std::string& recipes_path,
        const std::string& recipe_ingredients_path,
        const std::string& inventory_path, // empty = skip
        std::string& error
    );

    // Post-load processing: validate, then calculate nutrition.
    // Mutates the dataset in-place.
    // Returns false if there are fatal validation errors.
    static bool post_process(Dataset& dataset, ValidationResult& validation);
};

