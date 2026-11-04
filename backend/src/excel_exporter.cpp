// excel_exporter.cpp — Generate output workbook via OpenXLSX

#include "excel_exporter.h"
#include <OpenXLSX.hpp>
#include <string>
#include <cmath>
#include <iomanip>
#include <sstream>

using namespace OpenXLSX;

// ─── Sheet 1: Weekly_Meal_Plan ────────────────────────────────────────────

static void write_meal_plan_sheet(
    XLWorkbook& wb,
    const MealPlan& plan,
    const EvaluationResult& eval,
    const Dataset& dataset)
{
    auto ws = wb.worksheet("Weekly_Meal_Plan");

    // Headers
    std::vector<std::string> headers = {
        "Day", "Meal Type", "Recipe ID", "Recipe Name", "Servings",
        "Calories", "Protein (g)", "Carbs (g)", "Fat (g)",
        "Gross Cost (₹)", "Est. Purchase Cost (₹)"
    };
    for (int c = 0; c < (int)headers.size(); ++c)
        ws.cell(1, c+1).value() = headers[c];

    static const std::vector<std::string> DAY_NAMES = {
        "Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"
    };

    int row = 2;
    for (const auto& a : plan.assignments) {
        auto it = dataset.calc_recipe_map.find(a.recipe_id);
        if (it == dataset.calc_recipe_map.end()) continue;
        const auto& cr = it->second;

        ws.cell(row, 1).value() = DAY_NAMES[a.day];
        ws.cell(row, 2).value() = a.meal_type;
        ws.cell(row, 3).value() = a.recipe_id;
        ws.cell(row, 4).value() = cr.recipe_name;
        ws.cell(row, 5).value() = a.servings;
        ws.cell(row, 6).value() = std::round(cr.calories_per_serving * a.servings * 10) / 10.0;
        ws.cell(row, 7).value() = std::round(cr.protein_per_serving  * a.servings * 100) / 100.0;
        ws.cell(row, 8).value() = std::round(cr.carbs_per_serving    * a.servings * 100) / 100.0;
        ws.cell(row, 9).value() = std::round(cr.fat_per_serving      * a.servings * 100) / 100.0;
        ws.cell(row,10).value() = std::round(cr.gross_cost_per_serving * a.servings * 100) / 100.0;
        // Purchase cost contribution (not allocated per slot — show total in summary)
        ws.cell(row,11).value() = "";
        ++row;
    }

    // Weekly summary
    ++row;
    ws.cell(row, 1).value() = "WEEKLY TOTAL";
    ws.cell(row, 6).value() = std::round(eval.weekly_calories  * 10) / 10.0;
    ws.cell(row, 7).value() = std::round(eval.weekly_protein   * 100) / 100.0;
    ws.cell(row, 8).value() = std::round(eval.weekly_carbs     * 100) / 100.0;
    ws.cell(row, 9).value() = std::round(eval.weekly_fat       * 100) / 100.0;
    ws.cell(row,10).value() = std::round(eval.gross_recipe_cost * 100) / 100.0;
    ws.cell(row,11).value() = std::round(eval.weekly_purchase_cost * 100) / 100.0;
}

// ─── Sheet 2: Nutritional_Summary ────────────────────────────────────────

static void write_nutrition_sheet(
    XLWorkbook& wb,
    const EvaluationResult& eval,
    const OptimizationConfig& config)
{
    auto ws = wb.worksheet("Nutritional_Summary");

    std::vector<std::string> headers = {
        "Day",
        "Actual Cal", "Target Cal", "Min Cal", "Max Cal",
        "Actual Prot(g)", "Target Prot(g)", "Min Prot(g)",
        "Actual Carbs(g)", "Target Carbs(g)", "Min Carbs(g)",
        "Actual Fat(g)", "Target Fat(g)", "Max Fat(g)",
        "Daily Dev Score"
    };
    for (int c = 0; c < (int)headers.size(); ++c)
        ws.cell(1, c+1).value() = headers[c];

    static const std::vector<std::string> DAY_NAMES = {
        "Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"
    };

    for (int d = 0; d < 7; ++d) {
        int row = d + 2;
        ws.cell(row, 1).value()  = DAY_NAMES[d];
        ws.cell(row, 2).value()  = std::round(eval.daily_calories[d]  * 10) / 10.0;
        ws.cell(row, 3).value()  = config.targets.target_calories;
        ws.cell(row, 4).value()  = config.constraints.min_calories;
        ws.cell(row, 5).value()  = config.constraints.max_calories;
        ws.cell(row, 6).value()  = std::round(eval.daily_protein[d]   * 100) / 100.0;
        ws.cell(row, 7).value()  = config.targets.target_protein;
        ws.cell(row, 8).value()  = config.constraints.min_protein;
        ws.cell(row, 9).value()  = std::round(eval.daily_carbs[d]     * 100) / 100.0;
        ws.cell(row,10).value()  = config.targets.target_carbs;
        ws.cell(row,11).value()  = config.constraints.min_carbs;
        ws.cell(row,12).value()  = std::round(eval.daily_fat[d]       * 100) / 100.0;
        ws.cell(row,13).value()  = config.targets.target_fat;
        ws.cell(row,14).value()  = config.constraints.max_fat;
        double dev = d < (int)eval.daily_nutrition_deviation.size()
                     ? eval.daily_nutrition_deviation[d] : 0.0;
        ws.cell(row,15).value()  = std::round(dev * 10000) / 10000.0;
    }

    // Weekly totals
    int row = 10;
    ws.cell(row, 1).value() = "WEEKLY AVG/TOTAL";
    ws.cell(row, 2).value() = std::round(eval.weekly_calories / 7.0 * 10) / 10.0;
    ws.cell(row, 6).value() = std::round(eval.weekly_protein  / 7.0 * 100) / 100.0;
    ws.cell(row, 9).value() = std::round(eval.weekly_carbs    / 7.0 * 100) / 100.0;
    ws.cell(row,12).value() = std::round(eval.weekly_fat      / 7.0 * 100) / 100.0;
    ws.cell(row,15).value() = std::round(eval.nutrition_deviation_score * 10000) / 10000.0;
}

// ─── Sheet 3: Grocery_List ────────────────────────────────────────────────

static void write_grocery_sheet(
    XLWorkbook& wb,
    const EvaluationResult& eval,
    const Dataset& dataset)
{
    (void)dataset;
    auto ws = wb.worksheet("Grocery_List");

    std::vector<std::string> headers = {
        "Ingredient ID", "Ingredient Name",
        "Total Required (g)", "Available Inventory (g)",
        "Purchase Qty (g)", "Price per kg (₹)", "Est. Purchase Cost (₹)"
    };
    for (int c = 0; c < (int)headers.size(); ++c)
        ws.cell(1, c+1).value() = headers[c];

    int row = 2;
    double total_cost = 0.0;
    for (const auto& gi : eval.grocery_list) {
        ws.cell(row, 1).value() = gi.ingredient_id;
        ws.cell(row, 2).value() = gi.ingredient_name;
        ws.cell(row, 3).value() = std::round(gi.total_required_g  * 10) / 10.0;
        ws.cell(row, 4).value() = std::round(gi.available_g        * 10) / 10.0;
        ws.cell(row, 5).value() = std::round(gi.purchase_g         * 10) / 10.0;
        ws.cell(row, 6).value() = gi.price_per_kg_inr;
        ws.cell(row, 7).value() = std::round(gi.purchase_cost_inr  * 100) / 100.0;
        total_cost += gi.purchase_cost_inr;
        ++row;
    }

    ++row;
    ws.cell(row, 1).value() = "TOTAL GROCERY COST (₹)";
    ws.cell(row, 7).value() = std::round(total_cost * 100) / 100.0;
}

// ─── Export entry point ───────────────────────────────────────────────────

bool ExcelExporter::export_workbook(
    const std::string&        filepath,
    const MealPlan&           plan,
    const EvaluationResult&   eval,
    const OptimizationConfig& config,
    const Dataset&            dataset,
    const std::string&        mode_name,
    std::string&              error)
{
    (void)mode_name;
    try {
        XLDocument doc;
        doc.create(filepath);
        auto wb = doc.workbook();

        // Create exactly three worksheets
        wb.addWorksheet("Weekly_Meal_Plan");
        wb.addWorksheet("Nutritional_Summary");
        wb.addWorksheet("Grocery_List");

        // Remove default Sheet1 if it exists
        try { wb.deleteSheet("Sheet1"); } catch (...) {}

        write_meal_plan_sheet(wb, plan, eval, dataset);
        write_nutrition_sheet(wb, eval, config);
        write_grocery_sheet(wb, eval, dataset);

        doc.save();
        doc.close();
        return true;
    } catch (const std::exception& ex) {
        error = std::string("Excel export failed: ") + ex.what();
        return false;
    }
}

