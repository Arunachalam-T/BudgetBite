#pragma once
// validator.h — Dataset validation interface

#include "models.h"
#include "optimization_config.h"
#include <string>
#include <vector>

class Validator {
public:
    // Full validation of a loaded dataset
    static ValidationResult validate(const Dataset& dataset);

    // Validate OptimizationConfig against itself and the dataset
    static ValidationResult validate_config(
        const OptimizationConfig& config,
        const Dataset& dataset
    );

private:
    static void validate_ingredients(const Dataset& ds, std::vector<ValidationMessage>& msgs);
    static void validate_recipes(const Dataset& ds, std::vector<ValidationMessage>& msgs);
    static void validate_recipe_ingredients(const Dataset& ds, std::vector<ValidationMessage>& msgs);
    static void validate_inventory(const Dataset& ds, std::vector<ValidationMessage>& msgs);
    static void validate_cross_references(const Dataset& ds, std::vector<ValidationMessage>& msgs);

    static void add_fatal(std::vector<ValidationMessage>& msgs,
                          const std::string& worksheet, int row,
                          const std::string& message);
    static void add_warning(std::vector<ValidationMessage>& msgs,
                            const std::string& worksheet, int row,
                            const std::string& message);
};


