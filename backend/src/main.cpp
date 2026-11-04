// main.cpp — SmartMeal Optimizer backend entry point

#include "api.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        try { port = std::stoi(argv[1]); }
        catch (...) { port = 8080; }
    }

    std::cout << "╔══════════════════════════════════════╗\n";
    std::cout << "║   SmartMeal Optimizer  v1.0.0        ║\n";
    std::cout << "║   Backend API Server                 ║\n";
    std::cout << "╚══════════════════════════════════════╝\n";
    std::cout << "Starting server on http://localhost:" << port << "\n";
    std::cout << "Press Ctrl+C to stop.\n\n";

    AppState state;
    run_api_server(port, state);

    return 0;
}

