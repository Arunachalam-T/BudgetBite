#pragma once
// excel_reader.h — Excel (.xlsx) dataset reader via OpenXLSX

#include "models.h"
#include <string>

class ExcelReader {
public:
    // Load dataset from an Excel workbook (.xlsx) containing worksheets:
    //   - Ingredients
    //   - Recipes
    //   - Recipe_Ingredients
    //   - Inventory (optional)
    // Returns loaded Dataset; sets error message on failure.
    static Dataset load_from_excel(const std::string& filepath, std::string& error);
};

