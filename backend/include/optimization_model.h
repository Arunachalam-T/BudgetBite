#pragma once
// optimization_model.h — Problem formulation, slots, candidates, and problem bounds

#include "models.h"
#include "optimization_config.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

struct SlotDefinition {
    int slot_index = 0;   // 0 to total_slots - 1
    int day = 0;          // 0 to 6
    std::string meal_type; // e.g., "Breakfast", "Lunch", "Dinner"
};

class OptimizationModel {
public:
    OptimizationModel(const Dataset& dataset, const OptimizationConfig& config, OptimizationMode mode);

    const Dataset& dataset() const { return dataset_; }
    const OptimizationConfig& config() const { return config_; }
    OptimizationMode mode() const { return mode_; }

    int total_days() const { return 7; }
    int meals_per_day() const { return config_.meals_per_day; }
    int total_slots() const { return 7 * config_.meals_per_day; }

    const std::vector<SlotDefinition>& slots() const { return slots_; }
    const std::vector<const CalculatedRecipe*>& candidates_for_slot(int slot_index) const;
    const std::vector<const CalculatedRecipe*>& candidates_for_meal_type(const std::string& meal_type) const;

    bool is_recipe_eligible(const CalculatedRecipe& recipe) const;
    bool has_sufficient_candidates() const;

    // Bounds and precomputed heuristics
    double min_cost_per_slot(const std::string& meal_type) const;
    double max_cost_per_slot(const std::string& meal_type) const;
    double max_calories_per_slot(const std::string& meal_type) const;
    double max_protein_per_slot(const std::string& meal_type) const;
    double min_calories_per_slot(const std::string& meal_type) const;
    double min_protein_per_slot(const std::string& meal_type) const;

private:
    const Dataset& dataset_;
    OptimizationConfig config_;
    OptimizationMode mode_;

    std::vector<SlotDefinition> slots_;
    std::unordered_set<std::string> excluded_ids_;
    std::unordered_map<std::string, std::vector<const CalculatedRecipe*>> candidates_by_type_;
    std::vector<const CalculatedRecipe*> empty_list_;

    void build_slots();
    void filter_candidates();
};

