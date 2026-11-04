#pragma once
// excel_exporter.h — Generates the output Excel workbook with three sheets

#include "models.h"
#include "optimization_config.h"
#include <string>

class ExcelExporter {
public:
    // Generate a workbook with exactly three sheets:
    //   1. Weekly_Meal_Plan
    //   2. Nutritional_Summary
    //   3. Grocery_List
    //
    // filepath:   destination .xlsx path
    // plan:       the meal plan to export
    // eval:       evaluation result (nutrition, costs, grocery list)
    // config:     user config (targets, constraints, budget)
    // dataset:    for ingredient/recipe metadata
    // mode_name:  e.g., "Nutrition", "Budget", "Variety"
    //
    // Returns true on success; sets error on failure.
    static bool export_workbook(
        const std::string&        filepath,
        const MealPlan&           plan,
        const EvaluationResult&   eval,
        const OptimizationConfig& config,
        const Dataset&            dataset,
        const std::string&        mode_name,
        std::string&              error
    );
};

