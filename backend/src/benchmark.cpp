// benchmark.cpp — Run all five algorithms under identical conditions

#include "benchmark.h"
#include "brute_force.h"
#include "greedy.h"
#include "dynamic_programming.h"
#include "backtracking.h"
#include "branch_and_bound.h"
#include <chrono>
#include <ctime>

BenchmarkResult Benchmark::run_all(
    const Dataset&            dataset,
    const OptimizationConfig& config,
    OptimizationMode          mode,
    const ExecutionLimits&    limits)
{
    BenchmarkResult bench;
    bench.optimization_mode = optimization_mode_to_string(mode);
    bench.dataset_name      = dataset.source_name;
    bench.timestamp_unix    = static_cast<double>(
        std::chrono::system_clock::now().time_since_epoch().count());

    // Run algorithms in the specified order
    std::vector<std::unique_ptr<IOptimizationAlgorithm>> algorithms;
    algorithms.push_back(std::make_unique<BruteForce>());
    algorithms.push_back(std::make_unique<Greedy>());
    algorithms.push_back(std::make_unique<DynamicProgramming>());
    algorithms.push_back(std::make_unique<Backtracking>());
    algorithms.push_back(std::make_unique<BranchAndBound>());

    bench.results.reserve(algorithms.size());
    for (auto& algo : algorithms) {
        auto result = algo->solve(dataset, config, mode, limits);
        bench.results.push_back(std::move(result));
    }

    return bench;
}

