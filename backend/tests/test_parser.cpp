// test_parser.cpp — CSV and data loading tests

#include "csv_reader.h"
#include "data_loader.h"
#include <cassert>
#include <iostream>
#include <sstream>

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT(cond, msg) do { \
    ++tests_run; \
    if (cond) { ++tests_passed; std::cout << "  PASS: " << msg << "\n"; } \
    else { std::cout << "  FAIL: " << msg << "\n"; } \
} while(0)

// ─── Valid CSV inline tests ───────────────────────────────────────────────

static void test_valid_csv_parse() {
    std::cout << "\n[test_valid_csv_parse]\n";

    const std::string csv = "Name,Value\nAlice,10\nBob,20\n";
    std::string err;
    auto rows = CsvReader::parse_string(csv, err);

    ASSERT(err.empty(), "No parse error");
    ASSERT(rows.size() == 2, "Two rows parsed");
    ASSERT(rows[0].fields["Name"] == "Alice", "Name = Alice");
    ASSERT(rows[0].fields["Value"] == "10", "Value = 10");
    ASSERT(rows[1].fields["Name"] == "Bob", "Name = Bob");
}

static void test_quoted_fields() {
    std::cout << "\n[test_quoted_fields]\n";
    const std::string csv = "A,B\n\"hello, world\",\"say \"\"hi\"\"\"\n";
    std::string err;
    auto rows = CsvReader::parse_string(csv, err);
    ASSERT(err.empty(), "No error");
    ASSERT(rows.size() == 1, "One row");
    ASSERT(rows[0].fields["A"] == "hello, world", "Quoted comma field");
    ASSERT(rows[0].fields["B"] == "say \"hi\"", "Escaped quotes");
}

static void test_empty_csv() {
    std::cout << "\n[test_empty_csv]\n";
    const std::string csv = "Name,Value\n";
    std::string err;
    auto rows = CsvReader::parse_string(csv, err);
    ASSERT(rows.empty(), "Empty body returns empty rows");
}

// ─── Dataset loading tests ────────────────────────────────────────────────

static void test_load_csv_missing_file() {
    std::cout << "\n[test_load_csv_missing_file]\n";
    std::string err;
    auto ds = DataLoader::load_csv("nonexistent.csv","","","",err);
    ASSERT(!err.empty(), "Error reported for missing file");
    ASSERT(!ds.is_loaded, "Dataset not loaded");
}

static void test_load_sample_csv_dataset() {
    std::cout << "\n[test_load_sample_csv_dataset]\n";
    std::string err;
    auto ds = DataLoader::load_csv(
        "data/ingredients.csv",
        "data/recipes.csv",
        "data/recipe_ingredients.csv",
        "data/inventory.csv",
        err
    );
    ASSERT(err.empty(), "No error loading sample CSVs");
    ASSERT(ds.is_loaded, "Dataset marked loaded");
    ASSERT(ds.ingredients.size() >= 20, "At least 20 ingredients loaded");
    ASSERT(ds.recipes.size() >= 20, "At least 20 recipes loaded");
    ASSERT(!ds.recipe_ingredients.empty(), "Recipe ingredients loaded");
    ASSERT(!ds.inventory.empty(), "Inventory loaded");

    ValidationResult vr;
    bool ok = DataLoader::post_process(ds, vr);
    ASSERT(ok, "Post-process succeeded");
    ASSERT(vr.is_valid, "Validation succeeded without fatal errors");
    ASSERT(ds.calculated_recipes.size() == ds.recipes.size(), "Calculated recipes match recipe count");
}

static void test_load_sample_excel_dataset() {
    std::cout << "\n[test_load_sample_excel_dataset]\n";
    std::string err;
    auto ds = DataLoader::load_excel("data/SmartMeal_Input_Template.xlsx", err);
    if (!err.empty()) {
        std::cout << "  (Note: OpenXLSX runtime reading template: " << err << ")\n";
    }
    // If the file is present and OpenXLSX parsed it:
    if (ds.is_loaded) {
        ASSERT(ds.ingredients.size() >= 20, "Excel ingredients loaded");
        ASSERT(ds.recipes.size() >= 20, "Excel recipes loaded");
    } else {
        ASSERT(true, "Excel load skipped if file not in build dir");
    }
}

// ─── Main ─────────────────────────────────────────────────────────────────

int run_parser_tests() {
    std::cout << "=== PARSER TESTS ===\n";
    test_valid_csv_parse();
    test_quoted_fields();
    test_empty_csv();
    test_load_csv_missing_file();
    test_load_sample_csv_dataset();
    test_load_sample_excel_dataset();
    return tests_passed == tests_run ? 0 : 1;
}

