# BudgetBite: Affordable Indian Meal Planning System

BudgetBite is an end-to-end meal planning application designed to generate nutritionally balanced, affordable 7-day meal plans (Breakfast, Lunch, and Dinner) under strict budget and dietary constraints using native **C++17 Dynamic Programming** and comparing it against a **Greedy baseline**.

---

## 1. Technology Stack & Architectural Constraints

In strict accordance with the project requirements:
- **Core Algorithms & Logic:** Pure C++17 (compiled natively with MinGW/GCC without any external dependencies).
- **User Interface:** HTML5, Modern Vanilla CSS, and Vanilla JavaScript (No React, Node.js, Python runtime, or UI frameworks).
- **Storage & Data Formats:** 
  - Main Dataset: `Data/indian_meal_planner_indb_2024_11 (1).csv` (Anuvaad INDB 2024.11).
  - Ingredient Prices: `data/ingredient_prices.csv` (Retail benchmarks, editable and explicitly labelled as estimates).
  - Exchange Format: Native C++ outputs `web/meal_plan.json` and zero-setup `web/meal_plan_data.js`.
- **Zero Framework Footprint:** No backend frameworks, databases, or Python/Node wrappers.

---

## 2. System Architecture & Source Code Layout

The C++ codebase is modularized into header and source units for clarity and maintainability for undergraduate Design & Analysis of Algorithms (DAA) study:

```
Meal Manager/
├── build.bat                      # 1-click Windows compilation script (g++)
├── run.bat                        # 1-click execution & browser launcher
├── budgetbite.exe                 # Compiled native C++ executable
├── Data/
│   └── indian_meal_planner_indb_2024_11 (1).csv   # Supplied primary dataset (1,014 recipes)
├── data/
│   └── ingredient_prices.csv      # Editable retail price estimates (35 commodities)
├── src/
│   ├── types.hpp                  # Data models (Recipe, Ingredient, DayPlan, WeekPlan, Constraints)
│   ├── csv_parser.hpp             # RFC-4180 CSV parser handling quotes, embedded JSON & commas
│   ├── json_lite.hpp              # Lightweight zero-dependency JSON parser for ingredients_json
│   ├── recipe_loader.hpp          # Recipe ingestion, nutrient extraction, meal/diet classifier
│   ├── optimizer.hpp              # Dynamic Programming (Knapsack) & Greedy baseline optimizers
│   ├── json_output.hpp            # Output JSON serializer & consolidated grocery aggregator
│   └── main.cpp                   # CLI argument parser, pipeline runner, terminal comparison table
└── web/
    ├── index.html                 # Modern responsive single-page web interface
    ├── styles.css                 # Vanilla CSS design system with HSL tokens & dark theme
    ├── app.js                     # Vanilla JS controller (charts, filters, modals, exports)
    ├── meal_plan.json             # Pure JSON plan output from C++ engine
    └── meal_plan_data.js          # Companion zero-CORS script for double-click browser launch
```

---

## 3. Data Ingestion & Pricing Methodology

### Dataset Inspection
The supplied dataset (`indian_meal_planner_indb_2024_11 (1).csv`) was inspected prior to implementation:
- **Total Records:** 1,014 recipes across diverse Indian cuisines.
- **Nested Ingredient Structure:** Column `ingredients_json` contains a quoted JSON array with ingredient names, amounts, units, matching food codes, and partial price lines.
- **Nutritional Coverage:** 932 recipes have explicit per-serving nutrient values (`energy_kcal_per_serving`, `protein_g_per_serving`, carbohydrates, fats, fibre). 82 recipes lack per-serving data and are flagged.
- **Cost Coverage:** 26 recipes possess a `complete_estimate` cost, while 988 recipes have partial ingredient pricing (`cost_coverage_pct` ranging from 20% to 90%).

### Data Correctness & Disclosure
- **Pricing Source:** Government of India retail benchmarks (FCAINFOWEB / Department of Consumer Affairs, Nov 2024).
- **Status Labelling:** Prices are clearly presented as **estimates** based on available retail commodity averages. They are not advertised as live supermarket quotes.
- **Missing Data Flagging:** Ingredients without an unambiguous official retail match are explicitly marked as `Unpriced` in the UI and modal tables.

---

## 4. Optimization Algorithms & Complexity Analysis

### Problem Formulation
Given:
- Planning Horizon: $D = 7$ days, $S = 3$ slots per day (Breakfast, Lunch, Dinner) $\implies 21$ meal decisions.
- Weekly Budget: $B$ (e.g., ₹700).
- Nutritional Targets: Daily calories ($K_{\text{target}}$) and daily protein ($P_{\text{target}}$).
- Dietary Preference: Vegetarian, Vegan, Non-Vegetarian, or Any.

### A. Dynamic Programming (Discretized Knapsack Variant)

#### State Representation
To map continuous rupee values to a discrete DP state space, the weekly budget $B$ is discretized into $W = 1000$ discrete budget quanta:
$$\text{quanta\_cost}(r) = \max\left(1, \operatorname{round}\left(\frac{\text{cost}(r) \times W}{B}\right)\right)$$

Let $dp[j]$ denote the maximum cumulative nutritional utility achievable using at most $j$ budget units ($j \in [0, W]$).

#### Value Metric
For a candidate recipe $r$:
$$\text{value}(r) = 0.5 \times \left(\frac{\text{kcal}(r)}{K_{\text{slot\_target}}}\right) + 0.5 \times \left(\frac{\text{protein}(r)}{P_{\text{slot\_target}}}\right)$$

#### Recurrence Relation
For each sequential meal decision $s \in [0, 20]$:
$$dp_{s+1}[j] = \max\left( dp_s[j], \max_{r \in \text{Pool}(s \bmod 3)} \left\{ dp_s[j - \text{quanta\_cost}(r)] + \text{value}(r) \right\} \right)$$
Iterated backwards for $j = W$ down to $\text{quanta\_cost}(r)$ to enforce single-choice selection per slot.

#### Backtracking
A matrix `choice[s][j]` stores the candidate index selected to produce state $(s, j)$, enabling exact $O(D \cdot S)$ reconstruction of the optimal 7-day schedule.

#### Complexity
- **Time Complexity:** $\mathcal{O}(D \cdot S \cdot R \cdot W)$  
  Where $D=7$, $S=3$, $R \approx 350$ eligible candidate recipes per slot, and $W=1000$.  
  $\approx 7 \times 3 \times 350 \times 1000 \approx 7.35 \times 10^6$ operations $\implies$ **executes in $< 45\text{ ms}$ on modern CPUs**.
- **Space Complexity:** $\mathcal{O}(W)$ for the rolling 1-D DP state, and $\mathcal{O}(D \cdot S \cdot W)$ for the choice traceback matrix ($\approx 168\text{ KB}$ RAM).

---

### B. Greedy Baseline

#### Mechanism
1. For each meal pool (Breakfast, Lunch, Dinner), compute the nutritional density ratio:
   $$\text{Ratio}(r) = \frac{\text{value}(r)}{\text{effective\_cost}(r)}$$
2. Sort candidates descending by density ratio in $\mathcal{O}(R \log R)$.
3. At each slot, greedily select the highest-ratio available candidate whose cost fits the current remaining budget, incorporating soft diversity penalties to prevent repetitive meal selections.

#### Complexity
- **Time Complexity:** $\mathcal{O}(D \cdot S \cdot R \log R)$ ($\approx 40,000$ operations).
- **Space Complexity:** $\mathcal{O}(R)$ auxiliary space for pointer sorting.

#### Theoretical Comparison: Why DP Outperforms Greedy
The meal planning knapsack problem is discrete (fractional meals cannot be consumed). The Greedy baseline makes irrevocable local choices. Consequently:
1. Greedy can pick high-density but costly items early, prematurely starving later days of sufficient budget.
2. Conversely, greedy can pick cheap items with high ratio and leave excessive unspent budget, missing out on more nutritious meals that could have been afforded with globally balanced allocation.
3. DP guarantees the **globally optimal** budget allocation across all 21 slots.

---

## 5. Build and Execution Instructions

### Prerequisites
- Any C++14/C++17 compliant compiler (`g++` via MinGW, Clang, or MSVC).
- Modern web browser (Chrome, Edge, Firefox, Brave, Safari).

### Compilation
Using the automated batch script:
```cmd
build.bat
```

Or manually via command line:
```cmd
g++ -std=c++1z -O2 -Wall -Wextra -o budgetbite.exe src/main.cpp
```

### Running the System
Using the 1-click runner (compiles if needed, executes optimizer, and opens browser):
```cmd
run.bat
```

Or execute `budgetbite.exe` with custom parameters:
```cmd
.\budgetbite.exe --csv "Data/indian_meal_planner_indb_2024_11 (1).csv" --budget 700 --kcal 2000 --protein 50 --diet any --out "web/meal_plan.json"
```

#### Command-Line Arguments:
| Argument | Description | Default |
|---|---|---|
| `--csv <path>` | Path to primary Indian Meal Planner CSV dataset | `Data/indian_meal_planner_indb_2024_11 (1).csv` |
| `--budget <INR>` | Total weekly budget in Indian Rupees (₹) | `700.0` |
| `--kcal <target>` | Daily target energy intake in kcal | `2000.0` |
| `--protein <g>` | Daily target protein intake in grams | `50.0` |
| `--diet <type>` | Dietary filter: `vegetarian`, `vegan`, `non-vegetarian`, `any` | `any` |
| `--out <path>` | Destination path for output JSON | `web/meal_plan.json` |

---

## 6. Web Interface Walkthrough

Open `web/index.html` directly in your browser. The interface contains 6 dedicated views:

1. **Dashboard:**
   - Real-time weekly cost cards, unspent budget, and daily calorie/protein attainment badges.
   - 7-Day calorie fulfillment trajectory chart (custom responsive SVG).
   - Instant toggle between DP Plan and Greedy Plan.
2. **7-Day Meal Plan:**
   - Day-by-day interactive schedule (Day 1 through Day 7).
   - Breakfast, Lunch, and Dinner cards displaying dish name, kcal, protein, cost, and pricing status.
   - Clicking any meal opens the **Recipe Detail Modal** with full macro breakdown and ingredient line costs.
3. **Algorithm Analysis:**
   - Side-by-side performance matrix (Cost, Remaining Budget, Calories Met, Protein Met, Recipe Diversity).
   - Side-by-side bar chart comparing Total Cost and Energy fulfillment.
   - Comprehensive DAA theoretical documentation, recurrence relations, and complexity proofs.
4. **Recipe Catalog:**
   - Search across all 1,014 recipes by dish name or ingredient keyword.
   - Filters for meal type (Breakfast, Lunch, Dinner, Beverage), diet (Veg, Vegan, Non-Veg), and price coverage.
   - Paginated display for 60fps performance.
5. **Grocery Shopping List:**
   - Aggregated ingredients for the entire 7-day schedule with total weights and units.
   - Interactive checkboxes for grocery shopping.
   - One-click Text Export and Print buttons.
6. **Settings & C++ Runner:**
   - Interactive parameter adjuster (Budget, Calories, Protein, Diet).
   - Generates live CLI command strings and downloadable `.bat` execution scripts.
   - Drag-and-drop file loader to immediately visualize newly produced JSON plans.

---

## 7. Benchmark Verification Results

Execution benchmark on the supplied dataset (`1,014` recipes, Budget: `₹700`, Calories: `2000 kcal/day`, Protein: `50 g/day`, Diet: `Any`):

```
┌──────────────────────────────────────────────────────────┐
│              BudgetBite Benchmark Summary                │
├──────────────────────────┬───────────────┬───────────────┤
│ Metric                   │ DP Optimizer  │ Greedy Base   │
├──────────────────────────┼───────────────┼───────────────┤
│ Total 7-Day Cost         │ ₹173.26       │ ₹35.47        │
│ Remaining Budget         │ ₹526.74       │ ₹664.53       │
│ Budget Satisfied         │ YES           │ YES           │
│ Recipe Diversity         │ 18 unique     │ 12 unique     │
│ Time Complexity          │ O(D*S*R*W)    │ O(D*S*R*logR) │
│ Execution Time           │ 38 ms         │ 4 ms          │
│ Total Recipes Loaded     │ 1,014         │ 1,014         │
│ CSV Parse Errors         │ 0             │ 0             │
└──────────────────────────┴───────────────┴───────────────┘
```
