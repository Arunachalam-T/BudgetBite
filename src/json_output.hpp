// =============================================================================
// BudgetBite - Affordable Meal Planning System
// File: src/json_output.hpp
// Description: Serializes WeekPlan, Recipe list and grocery list to JSON
//              for consumption by the web front-end.
// =============================================================================
#pragma once

#include "types.hpp"

#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <iomanip>

// ── Escape a string for JSON ───────────────────────────────────────────────────
static std::string json_esc(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

static std::string jstr(const std::string& s) { return "\"" + json_esc(s) + "\""; }

static std::string jdbl(double v, int prec = 2) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(prec) << v;
    return ss.str();
}

static std::string jbool(bool b) { return b ? "true" : "false"; }

// ── Serialize a single DayPlan ─────────────────────────────────────────────────
static std::string serialize_day(const DayPlan& d) {
    std::ostringstream o;
    o << "{"
      << "\"day\":" << d.day_number << ","
      << "\"breakfast_id\":" << jstr(d.breakfast_id) << ","
      << "\"lunch_id\":"     << jstr(d.lunch_id)     << ","
      << "\"dinner_id\":"    << jstr(d.dinner_id)    << ","
      << "\"total_cost_inr\":" << jdbl(d.total_cost_inr) << ","
      << "\"total_kcal\":"     << jdbl(d.total_kcal, 1)  << ","
      << "\"total_protein_g\":" << jdbl(d.total_protein_g) << ","
      << "\"total_carb_g\":"    << jdbl(d.total_carb_g)    << ","
      << "\"total_fat_g\":"     << jdbl(d.total_fat_g)     << ","
      << "\"kcal_met\":"    << jbool(d.kcal_met)    << ","
      << "\"protein_met\":" << jbool(d.protein_met) << ","
      << "\"budget_met\":"  << jbool(d.budget_met)
      << "}";
    return o.str();
}

// ── Serialize a full WeekPlan ──────────────────────────────────────────────────
static std::string serialize_week_plan(const WeekPlan& wp) {
    std::ostringstream o;
    o << "{"
      << "\"algorithm\":" << jstr(wp.algorithm) << ","
      << "\"total_cost_inr\":" << jdbl(wp.total_cost_inr) << ","
      << "\"budget_remaining\":" << jdbl(wp.budget_remaining) << ","
      << "\"budget_satisfied\":" << jbool(wp.budget_satisfied) << ","
      << "\"all_kcal_met\":"    << jbool(wp.all_kcal_met)    << ","
      << "\"all_protein_met\":" << jbool(wp.all_protein_met) << ","
      << "\"complexity_note\":" << jstr(wp.complexity_note) << ","
      << "\"days\":[";
    for (size_t i = 0; i < wp.days.size(); ++i) {
        if (i) o << ",";
        o << serialize_day(wp.days[i]);
    }
    o << "]}";
    return o.str();
}

// ── Serialize a Recipe (summary, for recipe browser) ─────────────────────────
static std::string serialize_recipe_summary(const Recipe& r) {
    std::ostringstream o;
    o << "{"
      << "\"id\":"           << jstr(r.recipe_id)    << ","
      << "\"name\":"         << jstr(r.recipe_name)  << ","
      << "\"serving_unit\":" << jstr(r.serving_unit) << ","
      << "\"kcal\":"         << jdbl(r.kcal_per_serving, 1) << ","
      << "\"protein_g\":"    << jdbl(r.protein_g_per_serving) << ","
      << "\"carb_g\":"       << jdbl(r.carb_g_per_serving)   << ","
      << "\"fat_g\":"        << jdbl(r.fat_g_per_serving)    << ","
      << "\"cost_inr\":"     << jdbl(r.effective_cost_per_serving()) << ","
      << "\"cost_complete\":" << jbool(r.has_complete_cost) << ","
      << "\"cost_coverage_pct\":" << jdbl(r.cost_coverage_pct, 0) << ","
      << "\"cost_status\":"  << jstr(r.cost_status) << ","
      << "\"is_vegetarian\":" << jbool(r.is_vegetarian) << ","
      << "\"is_vegan\":"      << jbool(r.is_vegan)      << ","
      << "\"is_non_veg\":"    << jbool(r.is_non_veg)    << ","
      << "\"is_breakfast\":"  << jbool(r.is_breakfast)  << ","
      << "\"is_lunch\":"      << jbool(r.is_lunch)      << ","
      << "\"is_dinner\":"     << jbool(r.is_dinner)     << ","
      << "\"is_beverage\":"   << jbool(r.is_beverage)   << ","
      << "\"ingredients\":[";
    for (size_t i = 0; i < r.ingredients.size(); ++i) {
        const auto& ing = r.ingredients[i];
        if (i) o << ",";
        o << "{"
          << "\"name\":"   << jstr(ing.name)   << ","
          << "\"amount\":" << jdbl(ing.amount, 2) << ","
          << "\"unit\":"   << jstr(ing.unit)   << ","
          << "\"priced\":" << jbool(ing.is_priced) << ","
          << "\"commodity\":" << jstr(ing.cost_price_commodity) << ","
          << "\"cost_line\":" << jdbl(ing.cost_line_inr, 2)
          << "}";
    }
    o << "]}";
    return o.str();
}

// ── Grocery list: aggregate ingredients from a WeekPlan ───────────────────────
struct GroceryItem {
    std::string name;
    std::string commodity;
    double total_amount = 0.0;
    std::string unit;
    bool all_priced = true;
};

static std::string build_grocery_json(
        const WeekPlan& wp,
        const std::vector<Recipe>& all_recipes) {
    // Build id→recipe index
    std::map<std::string, const Recipe*> id_map;
    for (const auto& r : all_recipes) id_map[r.recipe_id] = &r;

    // Aggregate per ingredient name
    std::map<std::string, GroceryItem> agg;

    auto add_recipe = [&](const std::string& rid) {
        auto it = id_map.find(rid);
        if (it == id_map.end()) return;
        for (const auto& ing : it->second->ingredients) {
            auto& gi = agg[ing.name];
            gi.name      = ing.name;
            gi.commodity = ing.cost_price_commodity;
            gi.unit      = ing.unit;
            gi.total_amount += ing.amount;
            if (!ing.is_priced) gi.all_priced = false;
        }
    };

    for (const auto& d : wp.days) {
        add_recipe(d.breakfast_id);
        add_recipe(d.lunch_id);
        add_recipe(d.dinner_id);
    }

    std::ostringstream o;
    o << "[";
    bool first = true;
    for (const auto& item : agg) {
        const auto& gi = item.second;
        if (!first) o << ",";
        first = false;
        o << "{"
          << "\"name\":"       << jstr(gi.name)       << ","
          << "\"commodity\":"  << jstr(gi.commodity)  << ","
          << "\"amount\":"     << jdbl(gi.total_amount, 2) << ","
          << "\"unit\":"       << jstr(gi.unit)       << ","
          << "\"all_priced\":" << jbool(gi.all_priced)
          << "}";
    }
    o << "]";
    return o.str();
}

// ── Full JSON output document ──────────────────────────────────────────────────
inline std::string build_full_output(
        const WeekPlan& dp_plan,
        const WeekPlan& greedy_plan,
        const std::vector<Recipe>& all_recipes,
        const UserConstraints& uc,
        int parse_errors,
        int missing_nutrition,
        int missing_cost) {

    std::ostringstream o;
    o << "{\n"
      << "  \"budget_inr\":"         << jdbl(uc.weekly_budget_inr, 2) << ",\n"
      << "  \"daily_kcal_target\":"  << jdbl(uc.daily_kcal_target, 1) << ",\n"
      << "  \"daily_protein_g\":"    << jdbl(uc.daily_protein_g, 1)   << ",\n"
      << "  \"diet_pref\":"          << jstr(uc.diet_pref) << ",\n"
      << "  \"total_recipes\":"      << all_recipes.size() << ",\n"
      << "  \"parse_errors\":"       << parse_errors        << ",\n"
      << "  \"missing_nutrition\":"  << missing_nutrition   << ",\n"
      << "  \"missing_cost\":"       << missing_cost        << ",\n"
      << "  \"dp_plan\":"            << serialize_week_plan(dp_plan)     << ",\n"
      << "  \"greedy_plan\":"        << serialize_week_plan(greedy_plan) << ",\n"
      << "  \"grocery_list_dp\":"    << build_grocery_json(dp_plan,     all_recipes) << ",\n"
      << "  \"grocery_list_greedy\":" << build_grocery_json(greedy_plan, all_recipes) << ",\n";

    // Recipe summaries (all)
    o << "  \"recipes\":[\n";
    for (size_t i = 0; i < all_recipes.size(); ++i) {
        if (i) o << ",\n";
        o << "    " << serialize_recipe_summary(all_recipes[i]);
    }
    o << "\n  ]\n}";
    return o.str();
}
