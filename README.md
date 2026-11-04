# SmartMeal Optimizer

A university-level Design and Analysis of Algorithms project that generates optimized 7-day Indian meal plans using five algorithm implementations, a C++ REST API backend, and a React dashboard.

---

## Features

- Upload ingredient/recipe datasets as CSV or Excel (.xlsx)
- Full dataset validation with clear error messages
- Three optimization modes: **Nutrition**, **Budget**, **Variety**
- Five algorithm implementations: Brute Force, Greedy, Dynamic Programming, Backtracking, Branch & Bound
- Algorithm benchmarking with execution time, nodes explored, pruned branches, memory, and optimality status
- Inventory-aware grocery cost calculation
- Excel export with three sheets: Weekly_Meal_Plan, Nutritional_Summary, Grocery_List
- Interactive React dashboard with Chart.js visualizations

---

## Technology Stack

| Layer    | Technology |
|----------|------------|
| Backend  | C++17, CMake, Crow (HTTP), nlohmann/json, OpenXLSX |
| Frontend | React 18, Vite, Chart.js, Lucide icons |
| Platform | Windows, MSYS2 UCRT64 (GCC) |

---

## Requirements

### Backend
- MSYS2 with UCRT64 toolchain (`pacman -S mingw-w64-ucrt-x86_64-gcc`)
- CMake ≥ 3.20
- Git (for FetchContent dependencies)
- Internet access during the first CMake configure (to download Crow, nlohmann/json, OpenXLSX)

### Frontend
- Node.js ≥ 18
- npm or pnpm

---

## Installation & Build

### 1. Backend

Open **MSYS2 UCRT64** terminal:

```bash
cd "c:/Users/Arunachalam/OneDrive/Desktop/Meal Manager/backend"

# Configure (downloads dependencies automatically)
cmake -B build/release -G "MinGW Makefiles" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_COMPILER=gcc \
      -DCMAKE_CXX_COMPILER=g++

# Build
cmake --build build/release --parallel

# Run the server (default port 8080)
./build/release/bin/smartmeal_server.exe

# Run tests
./build/release/bin/smartmeal_tests.exe
```

Or use **VS Code** with CMake Tools extension:
1. Open the `backend/` folder in VS Code
2. Select the UCRT64 GCC kit
3. Select the `release-windows` preset
4. Press F7 to build

### 2. Frontend

```bash
cd "c:/Users/Arunachalam/OneDrive/Desktop/Meal Manager/frontend"
npm install
npm run dev
```

Open http://localhost:5173 in your browser.

> The Vite dev server proxies `/api` requests to `http://localhost:8080`.

---

## Dataset Format

### Option A: Four CSV files

| File                      | Required columns |
|---------------------------|-----------------|
| `ingredients.csv`         | `Ingredient_ID, Name, Price_per_kg_INR, Calories_per_100g, Protein_g_per_100g, Carbs_g_per_100g, Fat_g_per_100g, Diet_Category` |
| `recipes.csv`             | `Recipe_ID, Recipe_Name, Meal_Type` |
| `recipe_ingredients.csv`  | `Recipe_ID, Ingredient_ID, Quantity_g_per_serving` |
| `inventory.csv`           | `Ingredient_ID, Available_Quantity_g` (optional) |

Valid `Diet_Category` values: `Plant-based`, `Dairy`, `Eggs`, `Meat`, `Fish/Seafood`, `Animal-derived`

Valid `Meal_Type` values: `Breakfast`, `Lunch`, `Snack`, `Dinner`

### Option B: Excel workbook (.xlsx)

Same worksheets and columns as above, in a single `.xlsx` file.

---

## API Overview

| Method | Endpoint | Purpose |
|--------|----------|---------|
| GET | `/api/health` | Health check |
| POST | `/api/dataset/upload` | Upload dataset |
| GET | `/api/dataset/summary` | Dataset summary |
| GET | `/api/dataset/ingredients` | All ingredients |
| GET | `/api/dataset/recipes` | All calculated recipes |
| POST | `/api/optimization/validate-config` | Validate configuration |
| POST | `/api/optimization/generate-plan` | Generate plan for one mode |
| POST | `/api/algorithms/benchmark` | Run all five algorithms |
| GET | `/api/results/latest` | Latest results |
| POST | `/api/export/excel` | Export Excel workbook |

---

## Usage

1. **Load Dataset** — Go to Dataset page and provide CSV file paths
2. **Configure** — Go to Settings and set budget, nutritional constraints, targets, and diet
3. **Generate Plans** — Go to Meals page and generate plans for each optimization mode
4. **Compare Algorithms** — Go to Algorithms page and click "Run All Algorithms"
5. **Export** — Click Export to generate a three-sheet Excel workbook

---

## Project Structure

```
Meal Manager/
├── backend/
│   ├── include/     # Header files (17 .h)
│   ├── src/         # C++ source files (17 .cpp)
│   ├── tests/       # Automated test suites
│   └── CMakeLists.txt
├── frontend/
│   ├── src/
│   │   ├── pages/       # Dashboard, Meals, Algorithms, Dataset, Settings
│   │   ├── charts/      # NutritionChart, BudgetChart, VarietyChart, BenchmarkCharts
│   │   ├── services/    # API service layer
│   │   └── styles/      # Global CSS
│   └── package.json
├── data/            # Sample Indian meal data (CSV)
├── docs/            # Mathematical formulation and user guide
├── CMakeLists.txt
└── README.md
```

---

## Algorithm Optimality Guarantees

| Algorithm | Optimality | Notes |
|-----------|-----------|-------|
| Brute Force | ✓ (when exhausted) | Baseline; exponential in search space |
| Greedy | ✗ Never | Fast heuristic, no guarantee |
| Dynamic Programming | ✓ (when exhausted) | Exact via memoization |
| Backtracking | ✓ (when exhausted) | Exact with safe pruning |
| Branch & Bound | ✓ (when queue empty) | Best-first; reports optimality gap if interrupted |

All algorithms are subject to 15-second / 1,000,000-node execution limits.

