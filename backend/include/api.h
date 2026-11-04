#pragma once
// api.h — Crow REST API setup

#include "models.h"
#include "optimization_config.h"
#include "algorithm_interface.h"
#include "benchmark.h"
#include <string>
#include <mutex>
#include <optional>

// Global application state (one session at a time)
struct AppState {
    Dataset                     dataset;
    ValidationResult            validation;
    OptimizationConfig          config;

    // Latest results per mode
    std::optional<AlgorithmResult> latest_nutrition;
    std::optional<AlgorithmResult> latest_budget;
    std::optional<AlgorithmResult> latest_variety;
    std::optional<BenchmarkResult> latest_benchmark;

    std::mutex mtx; // protect concurrent access
};

// Start the Crow server on the given port.
// Blocks until stopped.
void run_api_server(int port, AppState& state);

