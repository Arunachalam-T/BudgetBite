// api.cpp — Crow HTTP REST API

#include "api.h"
#include "data_loader.h"
#include "validator.h"
#include "plan_evaluator.h"
#include "benchmark.h"
#include "brute_force.h"
#include "greedy.h"
#include "dynamic_programming.h"
#include "backtracking.h"
#include "branch_and_bound.h"
#include "excel_exporter.h"

#include <crow.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

// ─── JSON serialization helpers ───────────────────────────────────────────

static json ingredient_to_json(const Ingredient& ing) {
    return {
        {"ingredient_id",      ing.ingredient_id},
        {"name",               ing.name},
        {"price_per_kg_inr",   ing.price_per_kg_inr},
        {"calories_per_100g",  ing.calories_per_100g},
        {"protein_g_per_100g", ing.protein_g_per_100g},
        {"carbs_g_per_100g",   ing.carbs_g_per_100g},
        {"fat_g_per_100g",     ing.fat_g_per_100g},
        {"diet_category",      ing.diet_category}
    };
}

static json recipe_to_json(const CalculatedRecipe& cr) {
    return {
        {"recipe_id",             cr.recipe_id},
        {"recipe_name",           cr.recipe_name},
        {"meal_type",             cr.meal_type},
        {"calories_per_serving",  cr.calories_per_serving},
        {"protein_per_serving",   cr.protein_per_serving},
        {"carbs_per_serving",     cr.carbs_per_serving},
        {"fat_per_serving",       cr.fat_per_serving},
        {"gross_cost_per_serving",cr.gross_cost_per_serving}
    };
}

static json validation_to_json(const ValidationResult& vr) {
    json msgs = json::array();
    for (const auto& m : vr.messages) {
        msgs.push_back({
            {"severity",  m.severity == ValidationMessage::Severity::FATAL ? "FATAL" : "WARNING"},
            {"worksheet", m.worksheet},
            {"row",       m.row},
            {"message",   m.message}
        });
    }
    return {{"is_valid", vr.is_valid}, {"messages", msgs}};
}

static json grocery_list_to_json(const std::vector<GroceryItem>& list) {
    json arr = json::array();
    for (const auto& gi : list) {
        arr.push_back({
            {"ingredient_id",    gi.ingredient_id},
            {"ingredient_name",  gi.ingredient_name},
            {"total_required_g", gi.total_required_g},
            {"available_g",      gi.available_g},
            {"purchase_g",       gi.purchase_g},
            {"price_per_kg_inr", gi.price_per_kg_inr},
            {"purchase_cost_inr",gi.purchase_cost_inr}
        });
    }
    return arr;
}

static json eval_to_json(const EvaluationResult& e) {
    return {
        {"is_feasible",              e.is_feasible},
        {"violations",               e.violation_messages},
        {"weekly_purchase_cost",     e.weekly_purchase_cost},
        {"gross_recipe_cost",        e.gross_recipe_cost},
        {"daily_calories",           e.daily_calories},
        {"daily_protein",            e.daily_protein},
        {"daily_carbs",              e.daily_carbs},
        {"daily_fat",                e.daily_fat},
        {"weekly_calories",          e.weekly_calories},
        {"weekly_protein",           e.weekly_protein},
        {"weekly_carbs",             e.weekly_carbs},
        {"weekly_fat",               e.weekly_fat},
        {"recipe_repetitions",       e.recipe_repetitions},
        {"distinct_recipe_count",    e.distinct_recipe_count},
        {"nutrition_deviation_score",e.nutrition_deviation_score},
        {"variety_score",            e.variety_score},
        {"primary_objective_score",  e.primary_objective_score},
        {"grocery_list",             grocery_list_to_json(e.grocery_list)},
        {"daily_nutrition_deviation",e.daily_nutrition_deviation}
    };
}

static json plan_to_json(const MealPlan& plan) {
    json arr = json::array();
    for (const auto& a : plan.assignments) {
        arr.push_back({
            {"day",       a.day},
            {"meal_type", a.meal_type},
            {"recipe_id", a.recipe_id},
            {"servings",  a.servings}
        });
    }
    return arr;
}

static json algo_result_to_json(const AlgorithmResult& r) {
    return {
        {"algorithm_name",        r.algorithm_name},
        {"status",                result_status_to_string(r.status)},
        {"is_feasible",           r.is_feasible},
        {"optimality_proven",     r.optimality_proven},
        {"execution_time_ms",     r.execution_time_ms},
        {"nodes_explored",        r.nodes_explored},
        {"branches_pruned",       r.branches_pruned},
        {"peak_memory_bytes",     r.peak_memory_bytes},
        {"primary_objective",     r.primary_objective},
        {"weekly_purchase_cost",  r.weekly_purchase_cost},
        {"nutrition_deviation",   r.nutrition_deviation},
        {"distinct_recipe_count", r.distinct_recipe_count},
        {"optimality_gap_pct",    r.optimality_gap_pct},
        {"termination_reason",    r.termination_reason},
        {"plan",                  plan_to_json(r.best_plan)},
        {"evaluation",            eval_to_json(r.evaluation)}
    };
}

static OptimizationConfig config_from_json(const json& j) {
    OptimizationConfig cfg;
    if (j.contains("weekly_budget_inr"))     cfg.weekly_budget_inr     = j["weekly_budget_inr"];
    if (j.contains("meals_per_day"))         cfg.meals_per_day          = j["meals_per_day"];
    if (j.contains("max_recipe_repetitions"))cfg.max_recipe_repetitions = j["max_recipe_repetitions"];
    if (j.contains("diet_type"))             cfg.diet_type = string_to_diet_type(j["diet_type"]);
    if (j.contains("min_calories"))          cfg.constraints.min_calories = j["min_calories"];
    if (j.contains("max_calories"))          cfg.constraints.max_calories = j["max_calories"];
    if (j.contains("min_protein"))           cfg.constraints.min_protein  = j["min_protein"];
    if (j.contains("min_carbs"))             cfg.constraints.min_carbs    = j["min_carbs"];
    if (j.contains("max_fat"))               cfg.constraints.max_fat      = j["max_fat"];
    if (j.contains("target_calories"))       cfg.targets.target_calories  = j["target_calories"];
    if (j.contains("target_protein"))        cfg.targets.target_protein   = j["target_protein"];
    if (j.contains("target_carbs"))          cfg.targets.target_carbs     = j["target_carbs"];
    if (j.contains("target_fat"))            cfg.targets.target_fat       = j["target_fat"];
    if (j.contains("excluded_ingredient_ids"))
        cfg.excluded_ingredient_ids = j["excluded_ingredient_ids"].get<std::vector<std::string>>();
    if (j.contains("preferred_ingredient_ids"))
        cfg.preferred_ingredient_ids = j["preferred_ingredient_ids"].get<std::vector<std::string>>();
    return cfg;
}

// ─── API Server ───────────────────────────────────────────────────────────

void run_api_server(int port, AppState& state) {
    crow::SimpleApp app;

    // CORS middleware — apply to all responses
    auto add_cors = [](crow::response& res) {
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
    };

    // ─── OPTIONS preflight ───────────────────────────────────────────────
    CROW_ROUTE(app, "/api/<path>").methods(crow::HTTPMethod::OPTIONS)
    ([&add_cors](const crow::request&, crow::response& res, std::string) {
        add_cors(res);
        res.code = 204;
        res.end();
    });

    // ─── GET /api/health ────────────────────────────────────────────────
    CROW_ROUTE(app, "/api/health")(
    [&add_cors]() {
        crow::response res;
        add_cors(res);
        res.body = json{{"status","ok"},{"version","1.0.0"}}.dump();
        res.set_header("Content-Type","application/json");
        return res;
    });

    // ─── POST /api/dataset/upload ────────────────────────────────────────
    CROW_ROUTE(app, "/api/dataset/upload").methods(crow::HTTPMethod::POST)
    ([&state, &add_cors](const crow::request& req) {
        crow::response res;
        add_cors(res);
        res.set_header("Content-Type","application/json");

        try {
            auto body = json::parse(req.body);
            // Expects JSON: {type:"csv", ingredients:"/path", recipes:"/path",
            //                recipe_ingredients:"/path", inventory:"/path"}
            std::lock_guard<std::mutex> lock(state.mtx);

            std::string err;
            Dataset ds;

            std::string upload_type = body.value("type","csv");
            if (upload_type == "csv") {
                std::string ing  = body.value("ingredients","");
                std::string rec  = body.value("recipes","");
                std::string ri   = body.value("recipe_ingredients","");
                std::string inv  = body.value("inventory","");
                ds = DataLoader::load_csv(ing, rec, ri, inv, err);
            } else {
                std::string path = body.value("filepath","");
                ds = DataLoader::load_excel(path, err);
            }

            if (!err.empty()) {
                res.code = 400;
                res.body = json{{"error", err}}.dump();
                return res;
            }

            ValidationResult vr;
            DataLoader::post_process(ds, vr);
            state.dataset    = ds;
            state.validation = vr;

            res.code = 200;
            res.body = json{
                {"success", true},
                {"validation", validation_to_json(vr)},
                {"ingredients_count", ds.ingredients.size()},
                {"recipes_count", ds.recipes.size()}
            }.dump();
        } catch (const std::exception& e) {
            res.code = 500;
            res.body = json{{"error", e.what()}}.dump();
        }
        return res;
    });

    // ─── GET /api/dataset/summary ────────────────────────────────────────
    CROW_ROUTE(app, "/api/dataset/summary")(
    [&state, &add_cors]() {
        crow::response res;
        add_cors(res);
        res.set_header("Content-Type","application/json");
        std::lock_guard<std::mutex> lock(state.mtx);
        if (!state.dataset.is_loaded) {
            res.code = 404;
            res.body = json{{"error","No dataset loaded"}}.dump();
            return res;
        }
        res.body = json{
            {"source_name",       state.dataset.source_name},
            {"is_valid",          state.dataset.is_valid},
            {"ingredients_count", state.dataset.ingredients.size()},
            {"recipes_count",     state.dataset.recipes.size()},
            {"inventory_count",   state.dataset.inventory.size()},
            {"validation",        validation_to_json(state.validation)}
        }.dump();
        return res;
    });

    // ─── GET /api/dataset/ingredients ───────────────────────────────────
    CROW_ROUTE(app, "/api/dataset/ingredients")(
    [&state, &add_cors]() {
        crow::response res;
        add_cors(res);
        res.set_header("Content-Type","application/json");
        std::lock_guard<std::mutex> lock(state.mtx);
        json arr = json::array();
        for (const auto& ing : state.dataset.ingredients)
            arr.push_back(ingredient_to_json(ing));
        res.body = arr.dump();
        return res;
    });

    // ─── GET /api/dataset/recipes ────────────────────────────────────────
    CROW_ROUTE(app, "/api/dataset/recipes")(
    [&state, &add_cors]() {
        crow::response res;
        add_cors(res);
        res.set_header("Content-Type","application/json");
        std::lock_guard<std::mutex> lock(state.mtx);
        json arr = json::array();
        for (const auto& cr : state.dataset.calculated_recipes)
            arr.push_back(recipe_to_json(cr));
        res.body = arr.dump();
        return res;
    });

    // ─── POST /api/optimization/validate-config ──────────────────────────
    CROW_ROUTE(app, "/api/optimization/validate-config").methods(crow::HTTPMethod::POST)
    ([&state, &add_cors](const crow::request& req) {
        crow::response res;
        add_cors(res);
        res.set_header("Content-Type","application/json");
        try {
            auto body = json::parse(req.body);
            std::lock_guard<std::mutex> lock(state.mtx);
            auto cfg = config_from_json(body);
            auto vr  = Validator::validate_config(cfg, state.dataset);
            res.body = validation_to_json(vr).dump();
        } catch (const std::exception& e) {
            res.code = 400;
            res.body = json{{"error", e.what()}}.dump();
        }
        return res;
    });

    // ─── POST /api/optimization/generate-plan ───────────────────────────
    CROW_ROUTE(app, "/api/optimization/generate-plan").methods(crow::HTTPMethod::POST)
    ([&state, &add_cors](const crow::request& req) {
        crow::response res;
        add_cors(res);
        res.set_header("Content-Type","application/json");
        try {
            auto body = json::parse(req.body);
            std::lock_guard<std::mutex> lock(state.mtx);

            if (!state.dataset.is_valid) {
                res.code = 400;
                res.body = json{{"error","Dataset not valid"}}.dump();
                return res;
            }

            auto cfg  = config_from_json(body);
            auto mode = string_to_optimization_mode(body.value("mode","Nutrition"));
            std::string algo_name = body.value("algorithm","Backtracking");

            ExecutionLimits limits;
            if (body.contains("max_time_seconds")) limits.max_time_seconds = body["max_time_seconds"];
            if (body.contains("max_nodes"))        limits.max_nodes        = body["max_nodes"];

            // Validate configuration and limits
            auto vr = Validator::validate_config(cfg, state.dataset);
            if (limits.max_time_seconds <= 0.0) {
                vr.messages.push_back({ValidationMessage::Severity::FATAL, "Limits", -1, "max_time_seconds must be positive"});
                vr.is_valid = false;
            }
            if (limits.max_nodes <= 0) {
                vr.messages.push_back({ValidationMessage::Severity::FATAL, "Limits", -1, "max_nodes must be positive"});
                vr.is_valid = false;
            }

            if (!vr.is_valid || vr.has_fatal()) {
                res.code = 400;
                res.body = json{
                    {"error", "Invalid optimization configuration"},
                    {"validation", validation_to_json(vr)}
                }.dump();
                return res;
            }

            std::unique_ptr<IOptimizationAlgorithm> algo;
            if (algo_name == "Brute Force")         algo = std::make_unique<BruteForce>();
            else if (algo_name == "Greedy")         algo = std::make_unique<Greedy>();
            else if (algo_name == "Dynamic Programming") algo = std::make_unique<DynamicProgramming>();
            else if (algo_name == "Branch and Bound") algo = std::make_unique<BranchAndBound>();
            else                                    algo = std::make_unique<Backtracking>();

            auto result = algo->solve(state.dataset, cfg, mode, limits);
            state.config = cfg;

            // Store per mode paired with the configuration used
            switch (mode) {
                case OptimizationMode::NUTRITION:
                    state.latest_nutrition = result;
                    state.latest_nutrition_config = cfg;
                    break;
                case OptimizationMode::BUDGET:
                    state.latest_budget = result;
                    state.latest_budget_config = cfg;
                    break;
                case OptimizationMode::VARIETY:
                    state.latest_variety = result;
                    state.latest_variety_config = cfg;
                    break;
            }

            res.body = algo_result_to_json(result).dump();
        } catch (const std::exception& e) {
            res.code = 500;
            res.body = json{{"error", e.what()}}.dump();
        }
        return res;
    });

    // ─── POST /api/algorithms/benchmark ─────────────────────────────────
    CROW_ROUTE(app, "/api/algorithms/benchmark").methods(crow::HTTPMethod::POST)
    ([&state, &add_cors](const crow::request& req) {
        crow::response res;
        add_cors(res);
        res.set_header("Content-Type","application/json");
        try {
            auto body = json::parse(req.body);
            std::lock_guard<std::mutex> lock(state.mtx);

            if (!state.dataset.is_valid) {
                res.code = 400;
                res.body = json{{"error","Dataset not valid"}}.dump();
                return res;
            }

            auto cfg  = config_from_json(body);
            auto mode = string_to_optimization_mode(body.value("mode","Nutrition"));

            ExecutionLimits limits;
            if (body.contains("max_time_seconds")) limits.max_time_seconds = body["max_time_seconds"];
            if (body.contains("max_nodes"))        limits.max_nodes        = body["max_nodes"];

            // Validate configuration and limits
            auto vr = Validator::validate_config(cfg, state.dataset);
            if (limits.max_time_seconds <= 0.0) {
                vr.messages.push_back({ValidationMessage::Severity::FATAL, "Limits", -1, "max_time_seconds must be positive"});
                vr.is_valid = false;
            }
            if (limits.max_nodes <= 0) {
                vr.messages.push_back({ValidationMessage::Severity::FATAL, "Limits", -1, "max_nodes must be positive"});
                vr.is_valid = false;
            }

            if (!vr.is_valid || vr.has_fatal()) {
                res.code = 400;
                res.body = json{
                    {"error", "Invalid optimization configuration"},
                    {"validation", validation_to_json(vr)}
                }.dump();
                return res;
            }

            auto bench = Benchmark::run_all(state.dataset, cfg, mode, limits);
            state.latest_benchmark = bench;
            state.latest_benchmark_config = cfg;
            state.config = cfg;

            json results = json::array();
            for (const auto& r : bench.results)
                results.push_back(algo_result_to_json(r));

            res.body = json{
                {"optimization_mode", bench.optimization_mode},
                {"dataset_name",      bench.dataset_name},
                {"results",           results}
            }.dump();
        } catch (const std::exception& e) {
            res.code = 500;
            res.body = json{{"error", e.what()}}.dump();
        }
        return res;
    });

    // ─── GET /api/results/latest ─────────────────────────────────────────
    CROW_ROUTE(app, "/api/results/latest")(
    [&state, &add_cors]() {
        crow::response res;
        add_cors(res);
        res.set_header("Content-Type","application/json");
        std::lock_guard<std::mutex> lock(state.mtx);
        json obj;
        if (state.latest_nutrition) obj["nutrition"] = algo_result_to_json(*state.latest_nutrition);
        if (state.latest_budget)    obj["budget"]    = algo_result_to_json(*state.latest_budget);
        if (state.latest_variety)   obj["variety"]   = algo_result_to_json(*state.latest_variety);
        if (state.latest_benchmark) {
            json results = json::array();
            for (const auto& r : state.latest_benchmark->results)
                results.push_back(algo_result_to_json(r));
            obj["benchmark"] = results;
        }
        res.body = obj.dump();
        return res;
    });

    // ─── POST /api/export/excel ──────────────────────────────────────────
    CROW_ROUTE(app, "/api/export/excel").methods(crow::HTTPMethod::POST)
    ([&state, &add_cors](const crow::request& req) {
        crow::response res;
        add_cors(res);
        res.set_header("Content-Type","application/json");
        try {
            auto body = json::parse(req.body);
            std::lock_guard<std::mutex> lock(state.mtx);

            std::string mode_str = body.value("mode","Nutrition");
            auto mode = string_to_optimization_mode(mode_str);
            std::string filepath = body.value("filepath","SmartMeal_Output.xlsx");

            std::optional<AlgorithmResult>* src = nullptr;
            std::optional<OptimizationConfig>* src_cfg = nullptr;
            switch (mode) {
                case OptimizationMode::NUTRITION:
                    src = &state.latest_nutrition;
                    src_cfg = &state.latest_nutrition_config;
                    break;
                case OptimizationMode::BUDGET:
                    src = &state.latest_budget;
                    src_cfg = &state.latest_budget_config;
                    break;
                case OptimizationMode::VARIETY:
                    src = &state.latest_variety;
                    src_cfg = &state.latest_variety_config;
                    break;
            }

            if (!src || !*src || !(*src)->is_feasible || !src_cfg || !*src_cfg) {
                res.code = 400;
                res.body = json{{"error","No feasible plan available for mode: " + mode_str}}.dump();
                return res;
            }

            std::string err;
            bool ok = ExcelExporter::export_workbook(
                filepath, (*src)->best_plan, (*src)->evaluation,
                **src_cfg, state.dataset, mode_str, err);

            if (!ok) {
                res.code = 500;
                res.body = json{{"error", err}}.dump();
            } else {
                res.body = json{{"success",true},{"filepath",filepath}}.dump();
            }
        } catch (const std::exception& e) {
            res.code = 500;
            res.body = json{{"error", e.what()}}.dump();
        }
        return res;
    });

    CROW_LOG_INFO << "SmartMeal Optimizer API running on port " << port;
    app.port(port).multithreaded().run();
}

