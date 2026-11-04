// optimization_model.cpp — Optimization problem representation and candidate management

#include "optimization_model.h"
#include "plan_evaluator.h"
#include <algorithm>
#include <limits>

OptimizationModel::OptimizationModel(const Dataset& dataset,
                                     const OptimizationConfig& config,
                                     OptimizationMode mode)
    : dataset_(dataset), config_(config), mode_(mode)
{
    for (const auto& id : config_.excluded_ingredient_ids) {
        excluded_ids_.insert(id);
    }
    build_slots();
    filter_candidates();
}

void OptimizationModel::build_slots() {
    slots_.clear();
    auto meal_types = config_.meal_slots();
    int slot_idx = 0;
    for (int day = 0; day < 7; ++day) {
        for (const auto& mt : meal_types) {
            SlotDefinition sd;
            sd.slot_index = slot_idx++;
            sd.day = day;
            sd.meal_type = mt;
            slots_.push_back(sd);
        }
    }
}

bool OptimizationModel::is_recipe_eligible(const CalculatedRecipe& recipe) const {
    return PlanEvaluator::recipe_compatible(recipe, dataset_, config_);
}

void OptimizationModel::filter_candidates() {
    candidates_by_type_.clear();
    for (const auto& cr : dataset_.calculated_recipes) {
        if (!is_recipe_eligible(cr)) continue;
        candidates_by_type_[cr.meal_type].push_back(&cr);
    }
}

const std::vector<const CalculatedRecipe*>&
OptimizationModel::candidates_for_slot(int slot_index) const {
    if (slot_index < 0 || slot_index >= static_cast<int>(slots_.size())) {
        return empty_list_;
    }
    return candidates_for_meal_type(slots_[slot_index].meal_type);
}

const std::vector<const CalculatedRecipe*>&
OptimizationModel::candidates_for_meal_type(const std::string& meal_type) const {
    auto it = candidates_by_type_.find(meal_type);
    if (it == candidates_by_type_.end()) {
        return empty_list_;
    }
    return it->second;
}

bool OptimizationModel::has_sufficient_candidates() const {
    for (const auto& mt : config_.meal_slots()) {
        auto it = candidates_by_type_.find(mt);
        if (it == candidates_by_type_.end() || it->second.empty()) {
            return false;
        }
    }
    return true;
}

double OptimizationModel::min_cost_per_slot(const std::string& meal_type) const {
    const auto& cands = candidates_for_meal_type(meal_type);
    if (cands.empty()) return 0.0;
    double m = std::numeric_limits<double>::infinity();
    for (const auto* r : cands) {
        m = std::min(m, r->gross_cost_per_serving);
    }
    return m == std::numeric_limits<double>::infinity() ? 0.0 : m;
}

double OptimizationModel::max_cost_per_slot(const std::string& meal_type) const {
    const auto& cands = candidates_for_meal_type(meal_type);
    double m = 0.0;
    for (const auto* r : cands) {
        m = std::max(m, r->gross_cost_per_serving);
    }
    return m;
}

double OptimizationModel::max_calories_per_slot(const std::string& meal_type) const {
    const auto& cands = candidates_for_meal_type(meal_type);
    double m = 0.0;
    for (const auto* r : cands) {
        m = std::max(m, r->calories_per_serving);
    }
    return m;
}

double OptimizationModel::max_protein_per_slot(const std::string& meal_type) const {
    const auto& cands = candidates_for_meal_type(meal_type);
    double m = 0.0;
    for (const auto* r : cands) {
        m = std::max(m, r->protein_per_serving);
    }
    return m;
}

double OptimizationModel::min_calories_per_slot(const std::string& meal_type) const {
    const auto& cands = candidates_for_meal_type(meal_type);
    if (cands.empty()) return 0.0;
    double m = std::numeric_limits<double>::infinity();
    for (const auto* r : cands) {
        m = std::min(m, r->calories_per_serving);
    }
    return m == std::numeric_limits<double>::infinity() ? 0.0 : m;
}

double OptimizationModel::min_protein_per_slot(const std::string& meal_type) const {
    const auto& cands = candidates_for_meal_type(meal_type);
    if (cands.empty()) return 0.0;
    double m = std::numeric_limits<double>::infinity();
    for (const auto* r : cands) {
        m = std::min(m, r->protein_per_serving);
    }
    return m == std::numeric_limits<double>::infinity() ? 0.0 : m;
}

