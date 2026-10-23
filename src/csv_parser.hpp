// =============================================================================
// BudgetBite - Affordable Meal Planning System
// File: src/csv_parser.hpp
// Description: CSV parser that handles RFC-4180 quoted fields correctly
//
// The main dataset (indian_meal_planner_indb_2024_11.csv) stores each row's
// ingredients as a JSON array in column [08] "ingredients_json".  Because the
// JSON itself contains commas and double-quotes, the whole cell is
// double-quote-escaped per the CSV standard.  This parser handles that
// correctly: a double-quote inside a quoted field is represented as "".
// =============================================================================
#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// ── Trim leading / trailing whitespace ───────────────────────────────────────
static inline std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// ── Parse one CSV row into fields ────────────────────────────────────────────
// Handles:
//   - Quoted fields (may span multiple commas)
//   - Escaped quotes inside a quoted field ("" → ")
//   - Unquoted fields
static std::vector<std::string> parse_csv_row(const std::string& line) {
    std::vector<std::string> fields;
    bool in_quotes = false;
    std::string field;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (in_quotes) {
            if (c == '"') {
                // Peek ahead: "" inside quotes → literal "
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    // Closing quote
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
    fields.push_back(field);   // last field
    return fields;
}

// ── CSVReader: iterate over rows as name→value maps ─────────────────────────
class CSVReader {
public:
    using Row = std::vector<std::pair<std::string,std::string>>;

    explicit CSVReader(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open())
            throw std::runtime_error("Cannot open file: " + filepath);

        std::string line;

        // --- Header row ---
        // Skip UTF-8 BOM (EF BB BF) if present
        if (std::getline(file, line)) {
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            // Remove trailing \r
            if (!line.empty() && line.back() == '\r') line.pop_back();
            headers_ = parse_csv_row(line);
        }

        // --- Data rows ---
        // CSV cells can contain embedded newlines (inside quotes).
        // We accumulate logical lines until the quote-count is even.
        std::string logical;
        int quote_count = 0;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!logical.empty()) logical += '\n';
            logical += line;
            quote_count += static_cast<int>(std::count(line.begin(), line.end(), '"'));
            if (quote_count % 2 == 0) {
                rows_.push_back(parse_csv_row(logical));
                logical.clear();
                quote_count = 0;
            }
        }
        if (!logical.empty()) {
            rows_.push_back(parse_csv_row(logical));
        }
    }

    // Returns a Row (name→value pairs) for the given 0-based index
    Row get_row(size_t idx) const {
        Row r;
        if (idx >= rows_.size()) return r;
        const auto& raw = rows_[idx];
        for (size_t i = 0; i < headers_.size(); ++i) {
            std::string val = (i < raw.size()) ? raw[i] : "";
            r.emplace_back(headers_[i], val);
        }
        return r;
    }

    size_t row_count()                          const { return rows_.size(); }
    const std::vector<std::string>& headers()   const { return headers_; }

    // Helper: get a field by name from a row
    static std::string get(const Row& row, const std::string& key) {
        for (const auto& kv : row)
            if (kv.first == key) return kv.second;
        return "";
    }

    // Helper: get a double field safely
    static double get_double(const Row& row, const std::string& key, double def = 0.0) {
        std::string v = get(row, key);
        if (v.empty()) return def;
        try { return std::stod(v); } catch (...) { return def; }
    }

    // Helper: get an int field safely
    static int get_int(const Row& row, const std::string& key, int def = 0) {
        std::string v = get(row, key);
        if (v.empty()) return def;
        try { return std::stoi(v); } catch (...) { return def; }
    }

private:
    std::vector<std::string>         headers_;
    std::vector<std::vector<std::string>> rows_;
};
