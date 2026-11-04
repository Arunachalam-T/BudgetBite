// Test runner — calls all test suites

#include <iostream>

// Forward declarations
int run_parser_tests();
int run_validator_tests();
int run_calculator_tests();
int run_evaluator_tests();
int run_algorithm_tests();
int run_exporter_tests();

int main() {
    int failures = 0;

    failures += run_parser_tests();
    failures += run_validator_tests();
    failures += run_calculator_tests();
    failures += run_evaluator_tests();
    failures += run_algorithm_tests();
    failures += run_exporter_tests();

    std::cout << "\n═══════════════════════════════\n";
    if (failures == 0) {
        std::cout << "ALL TESTS PASSED\n";
    } else {
        std::cout << failures << " TEST SUITE(S) HAD FAILURES\n";
    }
    std::cout << "═══════════════════════════════\n";

    return failures;
}

