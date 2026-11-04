# MASTER PROJECT PROMPT — SMARTMEAL OPTIMIZER

## 0. Your role and operating instructions

Act as a senior C++ software engineer, algorithms engineer, optimization specialist, and frontend developer.

Build a complete, functional university-level Design and Analysis of Algorithms project named **SmartMeal Optimizer** from scratch.

You are responsible for the entire application:

* Project architecture and setup.
* Excel and CSV input handling.
* Dataset validation.
* Recipe and nutritional calculations.
* Mathematical optimization formulation.
* Five algorithm implementations.
* Algorithm benchmarking.
* C++ backend and API.
* Interactive web dashboard.
* Excel output generation.
* Testing, documentation, and final integration.

### Mandatory instructions

1. Do not ask the user questions or request decisions.
2. Do not stop at decision points. Use the specifications and defaults in this document.
3. Do not merely provide code snippets or instructions. Create and modify the actual project files.
4. Implement features completely rather than leaving TODOs, placeholders, or pseudocode.
5. Do not silently change the mathematical formulation.
6. Do not claim that an algorithm is optimal unless its optimality has been established.
7. Do not use Python for application logic, optimization, backend processing, or data calculations.
8. All five algorithms must be implemented natively in C++.
9. Keep code modular, readable, maintainable, and appropriately commented.
10. Run builds and tests after implementation. Fix errors before proceeding.
11. Preserve working code when making changes. Do not unnecessarily rewrite completed components.
12. If a dependency or implementation approach causes problems, select a practical compatible alternative and document the decision.
13. Continue through all development phases automatically. Do not wait for user approval between phases.
14. At the end, provide the complete project structure, setup instructions, build commands, run commands, and test results.

The final result must be a working application, not just a prototype or design document.

---

# 1. Project overview

## 1.1 Project name

**SmartMeal Optimizer**

## 1.2 Problem statement

Planning meals for an entire week while satisfying nutritional requirements, dietary preferences, ingredient availability, and a limited budget is a combinatorial optimization problem.

Manually selecting recipes for every meal becomes difficult when several constraints must be considered simultaneously.

SmartMeal Optimizer solves this problem by generating optimized seven-day meal plans and comparing different combinatorial optimization algorithms on the same problem.

## 1.3 Main objectives

The application must:

1. Allow users to upload their own ingredient and recipe datasets.
2. Validate the uploaded data.
3. Calculate recipe costs and nutritional values.
4. Accept user-defined weekly and daily constraints.
5. Generate optimized seven-day meal plans.
6. Support three optimization modes:
   - Nutrition-focused.
   - Budget-focused.
   - Variety-focused.
7. Compare five algorithms:
   - Brute Force.
   - Greedy.
   - Dynamic Programming.
   - Backtracking.
   - Branch and Bound.
8. Display execution metrics and actual algorithm results.
9. Generate an Excel workbook containing the final meal plan, nutritional summary, and grocery list.

The application must work with custom user-uploaded datasets, not only hardcoded sample recipes.

---

# 2. Technology stack

Use the following technologies.

## Backend

* C++17 or later.
* CMake.
* OpenXLSX for Excel reading and writing.
* A lightweight CSV parser.
* nlohmann/json for JSON serialization.
* Crow for the HTTP server and REST API.
* Standard C++ containers and algorithms.

## Frontend

* React.
* JavaScript.
* HTML and CSS.
* Chart.js for graphs.
* Lucide icons or an equivalent icon library.

## Development environment

* VS Code.
* CMake Tools extension.
* Microsoft C/C++ extension.
* GCC through MSYS2 UCRT64 on Windows.

## Restrictions

* Do not use Python, pandas, NumPy, SciPy, or scikit-learn.
* Do not use external optimization libraries.
* Implement all five optimization algorithms manually in C++.
* Frontend libraries may be used for presentation and interaction.
* Do not implement the optimization logic in JavaScript.

The C++ backend must perform all data processing, validation, calculations, optimization, evaluation, and Excel generation.

---

# 3. Complete application architecture

Use a frontend/backend architecture.

```text
SmartMeal-Optimizer/
│
├── backend/
│   ├── include/
│   │   ├── models.h
│   │   ├── data_loader.h
│   │   ├── excel_reader.h
│   │   ├── csv_reader.h
│   │   ├── validator.h
│   │   ├── recipe_calculator.h
│   │   ├── optimization_config.h
│   │   ├── optimization_model.h
│   │   ├── plan_evaluator.h
│   │   ├── algorithm_interface.h
│   │   ├── brute_force.h
│   │   ├── greedy.h
│   │   ├── dynamic_programming.h
│   │   ├── backtracking.h
│   │   ├── branch_and_bound.h
│   │   ├── benchmark.h
│   │   ├── excel_exporter.h
│   │   └── api.h
│   │
│   ├── src/
│   │   ├── main.cpp
│   │   ├── data_loader.cpp
│   │   ├── excel_reader.cpp
│   │   ├── csv_reader.cpp
│   │   ├── validator.cpp
│   │   ├── recipe_calculator.cpp
│   │   ├── optimization_config.cpp
│   │   ├── optimization_model.cpp
│   │   ├── plan_evaluator.cpp
│   │   ├── brute_force.cpp
│   │   ├── greedy.cpp
│   │   ├── dynamic_programming.cpp
│   │   ├── backtracking.cpp
│   │   ├── branch_and_bound.cpp
│   │   ├── benchmark.cpp
│   │   ├── excel_exporter.cpp
│   │   └── api.cpp
│   │
│   ├── tests/
│   │   ├── test_parser.cpp
│   │   ├── test_validator.cpp
│   │   ├── test_calculator.cpp
│   │   ├── test_evaluator.cpp
│   │   ├── test_algorithms.cpp
│   │   └── test_exporter.cpp
│   │
│   ├── CMakeLists.txt
│   └── CMakePresets.json
│
├── frontend/
│   ├── src/
│   │   ├── components/
│   │   ├── pages/
│   │   ├── charts/
│   │   ├── services/
│   │   ├── styles/
│   │   ├── App.jsx
│   │   └── main.jsx
│   ├── public/
│   └── package.json
│
├── data/
│   └── SmartMeal_Input_Template.xlsx
│
├── docs/
│   ├── PROJECT_DOCUMENTATION.md
│   ├── MATHEMATICAL_FORMULATION.md
│   └── USER_GUIDE.md
│
├── CMakeLists.txt
└── README.md
```

You may introduce additional files when needed, but maintain this separation of responsibilities.

---

# 4. Input dataset and Excel structure

The application must accept:

1. An Excel workbook (`.xlsx`) containing the required worksheets.
2. Four separate CSV files representing the same data.

A CSV upload must support:

* `ingredients.csv`
* `recipes.csv`
* `recipe_ingredients.csv`
* `inventory.csv`

Inventory CSV is optional.

## 4.1 Ingredients worksheet

Required columns:

| Column | Description |
|---|---|
| Ingredient_ID | Unique ingredient identifier |
| Name | Ingredient name |
| Price_per_kg_INR | Price per kilogram |
| Calories_per_100g | Calories per 100 g |
| Protein_g_per_100g | Protein per 100 g |
| Carbs_g_per_100g | Carbohydrates per 100 g |
| Fat_g_per_100g | Fat per 100 g |
| Diet_Category | Ingredient dietary classification |

### Diet_Category values

Allow exactly these categories:

* Plant-based
* Dairy
* Eggs
* Meat
* Fish/Seafood
* Animal-derived

Use a dropdown in the generated input template.

Every ingredient must have a valid dietary classification. Do not infer dietary compatibility from ingredient names.

Examples:

| Ingredient | Diet_Category |
|---|---|
| Rice | Plant-based |
| Toor Dal | Plant-based |
| Paneer | Dairy |
| Milk | Dairy |
| Egg | Eggs |
| Chicken | Meat |
| Fish | Fish/Seafood |
| Honey | Animal-derived |

For an unknown or unclassified ingredient, report a validation error. Never assume it is vegetarian or vegan.

## 4.2 Recipes worksheet

Required columns:

| Column | Description |
|---|---|
| Recipe_ID | Unique recipe identifier |
| Recipe_Name | Recipe name |
| Meal_Type | Meal category |

Allowed Meal_Type values:

* Breakfast
* Lunch
* Snack
* Dinner

## 4.3 Recipe_Ingredients worksheet

Required columns:

| Column | Description |
|---|---|
| Recipe_ID | Reference to Recipes |
| Ingredient_ID | Reference to Ingredients |
| Quantity_g_per_serving | Ingredient quantity in grams for one serving |

Each row represents one ingredient used by one recipe.

A recipe may contain multiple ingredients.

## 4.4 Inventory worksheet

Required columns:

| Column | Description |
|---|---|
| Ingredient_ID | Ingredient identifier |
| Available_Quantity_g | Existing inventory in grams |

This worksheet is optional.

If it is absent, treat the available inventory of every ingredient as zero.

## 4.5 Input template creation

Create a properly formatted Excel template containing the four worksheets.

Include:

* Clear column headings.
* Excel tables.
* Suitable column widths.
* Freeze panes.
* Data validation for dietary classifications and meal types.
* Sample Indian ingredients and recipes.
* A few inventory examples.
* Clear instructions for users.

Use the sample ingredient and recipe data described in the project examples, and add enough sample recipes to cover Breakfast, Lunch, Snack, and Dinner.

All sample prices and nutritional values must be labelled as illustrative demonstration values, not verified market data.

---

# 5. Data loading and validation

Implement a complete data-loading pipeline.

## 5.1 Data structures

Create suitable C++ structures:

```cpp
struct Ingredient {
    string ingredient_id;
    string name;

    double price_per_kg_inr;
    double calories_per_100g;
    double protein_g_per_100g;
    double carbs_g_per_100g;
    double fat_g_per_100g;

    string diet_category;
};

struct Recipe {
    string recipe_id;
    string recipe_name;
    string meal_type;
};

struct RecipeIngredient {
    string recipe_id;
    string ingredient_id;
    double quantity_g_per_serving;
};

struct InventoryItem {
    string ingredient_id;
    double available_quantity_g;
};
```

Add suitable structures for calculated recipes, datasets, validation results, and optimization results.

Use `std::vector`, `std::unordered_map`, and other appropriate standard containers.

## 5.2 Validation rules

Validate:

* Missing required worksheets.
* Missing required columns.
* Duplicate ingredient IDs.
* Duplicate recipe IDs.
* Empty IDs or names.
* Missing required values.
* Invalid numeric values.
* Negative prices.
* Negative quantities.
* Negative nutritional values.
* Invalid dietary classifications.
* Invalid meal types.
* Unknown ingredient references.
* Unknown recipe references.
* Recipes without ingredients.
* Duplicate recipe-ingredient mappings.
* Duplicate inventory entries.
* Invalid inventory quantities.

Provide meaningful error messages including worksheet and row information wherever possible.

Distinguish between:

* Fatal validation errors.
* Non-fatal warnings.

Do not proceed to optimization when fatal validation errors exist.

---

# 6. Recipe cost and nutrition calculations

Calculate nutritional values and costs per serving using the ingredient quantities.

For an ingredient used in a recipe:

### Cost

\[
Cost_i =
\frac{Quantity_i(g)}{1000}
\times Price_i(₹/kg)
\]

### Calories

\[
Calories_i =
\frac{Quantity_i(g)}{100}
\times Calories_i(100g)
\]

Use the equivalent formula for protein, carbohydrates, and fat.

For each recipe, sum the contributions of all its ingredients.

Store:

* Recipe ID.
* Calories per serving.
* Protein per serving.
* Carbohydrates per serving.
* Fat per serving.
* Gross ingredient cost per serving.

Do not repeatedly recalculate recipe nutrition inside every algorithm. Calculate it once after data validation and reuse it.

---

# 7. User configuration

Create an `OptimizationConfig` structure.

It must contain all user-defined constraints and targets.

## 7.1 Weekly configuration

* Weekly budget in INR.
* Meals per day: 2, 3, or 4.
* Maximum repetitions of any recipe per week.
* Diet type.
* Excluded ingredients.
* Preferred ingredients.

## 7.2 Daily hard nutritional constraints

The user must provide:

| Nutrient | Constraint |
|---|---|
| Calories | Minimum and maximum |
| Protein | Minimum |
| Carbohydrates | Minimum |
| Fat | Maximum |

## 7.3 Explicit daily nutritional targets

The user must also provide four separate preferred targets:

* Target calories.
* Target protein.
* Target carbohydrates.
* Target fat.

These targets are mandatory for Nutrition mode.

Do not derive the targets automatically from hard bounds.

Keep hard constraints and preferred targets as separate fields.

Validate that:

* All targets are positive.
* Calorie target lies within the calorie minimum and maximum.
* Protein target is at least the protein minimum.
* Carbohydrate target is at least the carbohydrate minimum.
* Fat target does not exceed the maximum fat limit.

Use equal weights for the four nutritional components by default. Do not introduce additional user-configurable weights.

## 7.4 Meal schedule

Use these fixed schedules:

| Meals per day | Meal slots |
|---|---|
| 2 | Breakfast, Dinner |
| 3 | Breakfast, Lunch, Dinner |
| 4 | Breakfast, Lunch, Snack, Dinner |

A recipe may only be assigned to a compatible meal slot.

If the uploaded dataset has no recipes for a required slot, report the issue clearly.

## 7.5 Diet compatibility

Implement compatibility using `Diet_Category`.

**Vegetarian:**

Allow:
* Plant-based.
* Dairy.
* Eggs.

Reject:
* Meat.
* Fish/Seafood.
* Animal-derived.

**Vegan:**

Allow only:
* Plant-based.

Reject all other categories.

**Non-vegetarian:**

Allow all categories.

**Unrestricted:**

Allow all categories.

A recipe is compatible only when every ingredient in that recipe is compatible with the selected diet.

Excluded ingredients always override diet compatibility.

Apply dietary filtering before optimization.

---

# 8. Inventory and grocery cost rules

Treat inventory as existing stock that reduces the amount the user needs to purchase.

Do not treat existing stock as a hard limit that makes a recipe unavailable.

For each ingredient:

\[
PurchaseQuantity_i =
\max(0, RequiredQuantity_i - AvailableInventory_i)
\]

The actual weekly grocery expenditure is:

\[
WeeklyPurchaseCost =
\sum_i
\left(
\frac{PurchaseQuantity_i(g)}{1000}
\times Price_i(₹/kg)
\right)
\]

The weekly budget applies to this purchase cost.

Also calculate and retain the gross recipe ingredient cost separately:

\[
GrossRecipeCost =
\sum_{\text{selected servings}} RecipeCost
\]

Display both values clearly.

This ensures that the grocery list, inventory usage, and weekly budget remain consistent.

---

# 9. Mathematical optimization formulation

All five algorithms must solve the same mathematical problem.

Never implement separate constraint definitions or separate objective formulas for different algorithms.

## 9.1 Decision variables

Define:

\[
x_{d,m,r} \in \{0,1\}
\]

Where:

* \(d\): day, from 0 to 6.
* \(m\): meal slot.
* \(r\): recipe.

The variable is 1 when recipe \(r\) is assigned to that meal slot.

Use exactly one serving per selected recipe initially.

## 9.2 Meal assignment constraint

Every meal slot must have exactly one recipe:

\[
\sum_r x_{d,m,r}=1
\]

Only recipes compatible with the meal type, diet, and excluded-ingredient restrictions are candidates.

## 9.3 Daily nutrition calculations

For each day, calculate:

\[
C_d = \sum_{m,r}x_{d,m,r}Calories_r
\]

Calculate protein, carbohydrates, and fat similarly.

## 9.4 Hard constraints

Every completed plan must satisfy:

**Calories**

\[
MinCalories \le C_d \le MaxCalories
\]

**Protein**

\[
Protein_d \ge MinProtein
\]

**Carbohydrates**

\[
Carbs_d \ge MinCarbs
\]

**Fat**

\[
Fat_d \le MaxFat
\]

Apply these constraints to every day.

**Weekly budget**

\[
WeeklyPurchaseCost \le WeeklyBudget
\]

**Recipe repetition**

\[
Count_r \le MaxRecipeRepetitions
\]

**Excluded ingredients**

No selected recipe may contain an excluded ingredient.

**Diet compatibility**

No selected recipe may violate the selected diet type.

There must be exactly one selected recipe per meal slot.

A plan violating any hard constraint is infeasible.

## 9.5 Inventory

Calculate total ingredient consumption over all seven days.

Calculate purchase quantities after subtracting available inventory.

Inventory affects purchase expenditure, not recipe eligibility.

## 9.6 Objective mode 1: Nutrition-focused

Minimize the deviation from the four explicit daily targets.

For nutrient \(n\):

\[
D_{d,n} =
\frac{|Actual_{d,n}-Target_n|}
{\max(Target_n,\epsilon)}
\]

Use a small positive \(\epsilon\) to avoid division by zero.

Calculate:

\[
D =
\frac{1}{28}
\sum_{d=1}^{7}
\sum_{n=1}^{4}D_{d,n}
\]

The nutrition objective is:

\[
\boxed{\min D}
\]

The hard constraints must be checked independently before this score is considered.

A plan outside any hard nutritional bound is infeasible, even if its deviation score is excellent.

## 9.7 Objective mode 2: Budget-focused

Minimize actual weekly grocery expenditure:

\[
\boxed{\min WeeklyPurchaseCost}
\]

All hard constraints remain active.

## 9.8 Objective mode 3: Variety-focused

Maximize the number of distinct recipes used during the week.

Let \(DistinctRecipes\) be the number of unique recipe IDs selected.

\[
\boxed{\max DistinctRecipes}
\]

Use this normalized display score:

\[
VarietyScore =
100 \times
\frac{DistinctRecipes}{TotalMealSlots}
\]

All hard constraints remain active.

## 9.9 Objective comparison and tie-breaking

Use a consistent lexicographic comparison:

1. Compare the selected mode's primary objective.
2. If primary values are equal within a small numerical tolerance, prefer lower weekly purchase cost.
3. If still tied, prefer lower nutritional deviation.
4. If still tied, choose the lexicographically smaller recipe-ID assignment.

For minimization modes, lower primary scores are better.

For variety mode, higher primary scores are better.

Document this comparison and implement it centrally so every algorithm uses identical rules.

---

# 10. Shared evaluator

This is one of the most important components.

Implement a single `PlanEvaluator`.

It must accept:

* Dataset.
* OptimizationConfig.
* Optimization mode.
* Candidate MealPlan.

It must return an `EvaluationResult`.

Include:

* Feasibility status.
* Specific constraint violation messages.
* Weekly purchase cost.
* Gross recipe cost.
* Daily calories.
* Daily protein.
* Daily carbohydrates.
* Daily fat.
* Weekly nutrition totals.
* Ingredient consumption.
* Existing inventory usage.
* Ingredient purchase quantities.
* Grocery list.
* Recipe repetition counts.
* Number of distinct recipes.
* Nutrition deviation score.
* Variety score.
* Primary objective score.

The evaluator must be deterministic.

All five algorithms must call this same evaluator when checking completed plans.

Also create reusable partial-state validation functions for search pruning. These functions must implement the same constraints and must never reject a partial state that could still lead to a feasible completed plan.

Do not duplicate or modify hard constraint definitions inside individual algorithms.

---

# 11. Algorithm implementations

Implement all five algorithms natively in C++.

All algorithms must receive the same:

* Dataset.
* OptimizationConfig.
* Optimization mode.
* Candidate recipe list.
* Hard constraints.
* Objective comparison function.
* Evaluator.

Each algorithm must return a common result structure.

## 11.1 Common algorithm result

Include:

* Algorithm name.
* Best meal plan found.
* Feasibility.
* Primary objective score.
* Weekly purchase cost.
* Nutritional deviation.
* Distinct recipe count.
* Execution time in milliseconds.
* Nodes/states explored.
* Branches pruned.
* Peak memory estimate.
* Whether optimality was proven.
* Termination reason.
* Lower or upper bound where applicable.
* Optimality gap where available.

Use a common interface, such as:

```cpp
class IOptimizationAlgorithm {
public:
    virtual AlgorithmResult solve(
        const Dataset& dataset,
        const OptimizationConfig& config,
        OptimizationMode mode
    ) = 0;

    virtual ~IOptimizationAlgorithm() = default;
};
```

Use an appropriate alternative if a simpler interface is more practical, but all implementations must remain interchangeable.

## 11.2 Brute Force

Implement exhaustive enumeration.

Process:

1. Generate compatible candidate recipes for each meal slot.
2. Enumerate every possible assignment.
3. Evaluate every completed assignment.
4. Retain the best feasible assignment.
5. Continue until the entire search space is exhausted or a configured execution limit is reached.

Do not use pruning in Brute Force.

This is the baseline algorithm.

If the search is exhaustive, the optimality of the best feasible solution is proven.

If interrupted, mark it as not proven optimal.

## 11.3 Greedy

Implement a deterministic greedy heuristic.

At each meal slot:

1. Identify currently eligible recipes.
2. Remove recipes violating immediate restrictions.
3. Calculate the marginal effect of each candidate.
4. Select the candidate with the best local score for the active optimization mode.
5. Update accumulated cost, nutrition, inventory requirements, and repetition counts.
6. Continue to the next meal slot.

Use these local selection priorities:

**Nutrition mode:** Choose the recipe that most improves the current day's normalized deviation from prorated nutritional targets.

**Budget mode:** Prefer the recipe with the lowest incremental grocery purchase cost.

**Variety mode:** Prefer recipes not yet selected, then recipes with lower repetition counts.

Use deterministic tie-breaking.

At completion, pass the plan through the common evaluator.

Greedy must never claim optimality.

If it fails to produce a feasible plan, report that no feasible plan was found by Greedy. Do not claim that the entire problem is infeasible.

Do not silently violate hard constraints to obtain a result.

## 11.4 Dynamic Programming

Implement exact state-based dynamic programming with memoization.

Use a state that contains all information required to determine future decisions and feasibility, including:

* Current meal-slot index.
* Accumulated daily nutrition.
* Accumulated ingredient consumption.
* Recipe repetition counts.
* Used recipe set.
* Accumulated purchase cost or sufficient information to derive it.

Use a fixed slot ordering.

Memoize states that have identical relevant accumulated values.

Do not discretize nutrition or cost values into arbitrary buckets and then claim exact optimality.

Use deterministic representations of accumulated numeric state for memoization. If exact state matching is required, preserve the original floating-point values and use a stable representation; do not merge states merely because they are approximately equal.

Use the common objective comparator when selecting the best continuation.

If all relevant states are explored, the result is exact.

If execution stops early, report that optimality was not proven.

Clearly document the DP state and recurrence.

## 11.5 Backtracking

Implement recursive backtracking.

Process:

1. Assign one recipe to the current meal slot.
2. Update partial state.
3. Check whether the partial assignment can still lead to a feasible solution.
4. If it cannot, undo the assignment and backtrack.
5. Otherwise, continue recursively.
6. At complete assignments, call the common evaluator.
7. Retain the best feasible plan.

Implement safe pruning for:

* Budget already exceeded.
* Maximum recipe repetitions exceeded.
* Excluded ingredient or dietary violations.
* Daily upper nutritional limits already exceeded.
* Minimum nutritional constraints that can no longer be reached using remaining meal slots.
* Ingredient consumption and other monotonic constraints where applicable.

Any lower-bound or upper-bound feasibility test must be mathematically safe.

Do not prune a branch based on a heuristic assumption that could remove a valid solution.

If exhaustive search finishes, the best feasible plan is proven optimal.

## 11.6 Branch and Bound

Implement a best-bound search using a priority queue.

Each search node must contain:

* Current meal-slot index.
* Partial meal plan.
* Accumulated cost.
* Daily nutritional totals.
* Ingredient consumption.
* Recipe repetition counts.
* Used recipe set.
* Valid optimistic objective bound.

Use valid optimistic bounds.

**Budget mode:**

* Use current purchase cost plus an admissible lower bound on the minimum possible additional purchase cost.
* Never use a lower bound that is greater than the actual best possible completion cost.

**Nutrition mode:**

* Estimate attainable intervals for each daily nutrient using the remaining compatible meal slots.
* Calculate a lower bound on possible normalized nutritional deviation from those intervals.
* Ensure the estimate is optimistic, not pessimistic.

**Variety mode:**

* Calculate an optimistic upper bound on the number of distinct recipes that could still be selected.
* Do not underestimate the attainable variety.

Use safe feasibility pruning as in Backtracking.

Expand the most promising node first.

Keep an incumbent best feasible solution.

Prune nodes only when their valid optimistic bounds cannot beat the incumbent under the common lexicographic comparison.

If the queue is exhausted, optimality is proven.

If interrupted, report the best solution, remaining bound, and optimality gap where possible.

---

# 12. Execution limits and result status

Exact algorithms can have enormous search spaces.

Use these default limits:

* Maximum execution time: 15 seconds per algorithm.
* Maximum explored states/nodes: 1,000,000 per algorithm.

Make these limits configurable internally.

Use the same limits for all algorithms in a benchmark.

Do not allow the Greedy algorithm to consume the entire benchmark duration.

Every algorithm must return one of these result statuses:

* `OPTIMAL`: A feasible solution has been proven optimal.
* `FEASIBLE_NOT_PROVEN`: A feasible solution exists, but optimality has not been established.
* `NO_FEASIBLE_FOUND`: No feasible plan was found before termination; infeasibility is not proven.
* `INFEASIBLE`: An exhaustive exact search proves no feasible solution exists.
* `INTERRUPTED`: Execution stopped before a definitive result was obtained.

Do not confuse `NO_FEASIBLE_FOUND` with `INFEASIBLE`.

For minimization:

\[
Gap =
\frac{Incumbent-Bound}
{\max(|Incumbent|,\epsilon)}
\times 100
\]

For maximization, use the corresponding valid upper bound and incumbent.

Do not display a fabricated optimality gap when no valid bound is available.

---

# 13. Benchmarking framework

Build a common benchmarking component.

When the user requests an algorithm comparison:

1. Use the currently uploaded dataset.
2. Use exactly the same optimization configuration.
3. Use the same optimization mode.
4. Use the same compatible recipe candidates.
5. Use the same execution limits.
6. Run all five algorithms.
7. Evaluate every returned feasible plan with the common evaluator.
8. Collect and display the metrics.

Measure execution time using `std::chrono::steady_clock`.

Measure memory consistently. Document whether the measurement is an estimate of algorithm-owned state memory or actual process peak memory. Do not mix the two definitions across algorithms.

Record:

* Execution time.
* States/nodes explored.
* Branches pruned.
* Memory.
* Feasibility.
* Objective value.
* Optimality status.
* Optimality gap when available.

Do not compare algorithms using different constraints or optimization modes.

Do not label Greedy's answer optimal.

Do not compare a completed exact result against an interrupted result as if both had the same proof status.

---

# 14. Frontend UI and complete design

Build a modern, clean, responsive dashboard.

The UI should look like a polished productivity and analytics application, not a basic university form.

Use a light theme with:

* White and light-grey surfaces.
* Dark navy or charcoal text.
* A restrained green accent for nutrition and successful results.
* Blue accents for general analytics.
* Subtle borders and shadows.
* Rounded cards.
* Consistent spacing.
* Clear typography.
* Responsive layouts.

Avoid excessive gradients, excessive animations, and unnecessary decorative elements.

## 14.1 Main layout

Use a fixed or collapsible left sidebar.

Sidebar items:

1. Dashboard.
2. Meals.
3. Algorithms.
4. Dataset.
5. Settings.

Include a top header with:

* SmartMeal Optimizer logo/name.
* Current dataset name.
* Dataset validation status.
* User's current optimization mode.
* Export button when a valid plan exists.

## 14.2 Dashboard page

Show an overview of the application.

Include:

* Dataset upload status.
* Number of ingredients.
* Number of recipes.
* Last validation result.
* Current weekly budget.
* Selected diet.
* Last generated plan summary.
* Quick navigation to Meals and Algorithms.

When no dataset has been uploaded, show a clear upload call-to-action.

Do not show fabricated results.

## 14.3 Dataset page

Allow users to:

* Upload an `.xlsx` workbook.
* Upload the four CSV files.
* Download the input template.
* View validation errors and warnings.
* Inspect ingredients.
* Inspect recipes.
* Inspect recipe-ingredient mappings.
* Inspect inventory.

Display tables with search, sorting, and clear error indicators.

If validation fails, identify the worksheet, row, and problem.

Do not permit optimization until fatal validation errors are resolved.

## 14.4 Settings page

Create a clean configuration form with the following fields.

### Weekly settings

* Weekly budget (₹).
* Meals per day: 2, 3, or 4.
* Maximum recipe repetitions per week.

### Nutrition constraints

* Minimum daily calories.
* Maximum daily calories.
* Minimum daily protein.
* Minimum daily carbohydrates.
* Maximum daily fat.

### Preferred daily nutrition targets

* Target calories.
* Target protein.
* Target carbohydrates.
* Target fat.

These fields are mandatory for Nutrition mode.

### Dietary settings

* Vegetarian.
* Vegan.
* Non-vegetarian.
* Unrestricted.

### Ingredient settings

* Multi-select excluded ingredients.
* Multi-select preferred ingredients.

Preferred ingredients must act only as a secondary preference. They must never override the primary objective or any hard constraint.

### Configuration validation

Display inline errors for invalid configurations.

Do not submit invalid configurations to the backend.

Provide a clear Save Configuration button.

## 14.5 Meals page

This is one of the main pages.

Display three tabs:

1. Nutrition.
2. Budget.
3. Variety.

Each tab must display the meal plan generated for that optimization mode.

The user should be able to switch between modes without losing their configuration.

Provide a prominent **Generate Meal Plan** button.

### Weekly meal plan display

Display a seven-day calendar or grid.

Each day should show:

* Day name.
* Meal type.
* Recipe name.
* Serving count.
* Cost.
* Calories.
* Protein.
* Carbohydrates.
* Fat.

Use visually distinct recipe cards.

Provide a daily summary below or beside each day.

### Nutrition mode

Show:

* Daily target versus actual calories.
* Daily target versus actual protein.
* Daily target versus actual carbohydrates.
* Daily target versus actual fat.
* Nutritional deviation score.
* Weekly nutrition summary.

Use bar charts or line charts for daily nutritional comparisons.

### Budget mode

Show:

* Weekly grocery expenditure.
* Weekly budget.
* Remaining budget.
* Gross recipe ingredient cost.
* Existing inventory value used, if calculated.
* Daily expenditure.
* Grocery purchase breakdown.

Use suitable charts for budget allocation.

### Variety mode

Show:

* Number of distinct recipes.
* Total meal slots.
* Variety score.
* Recipe repetition counts.
* Distribution of recipes across the week.

Use a bar chart or doughnut chart to show recipe distribution.

### Common requirements

All three modes must respect the same hard constraints.

Display infeasibility reasons clearly.

Never display an invalid plan as a successful result.

Provide a link or button to export the generated plan to Excel.

## 14.6 Algorithms page

This page is dedicated to comparing the five algorithms.

At the top, display:

* Current dataset.
* Active optimization mode.
* Current configuration summary.
* Run All Algorithms button.

All algorithms must use the same dataset and configuration.

### Algorithm comparison table

Display:

| Algorithm | Execution Time | Nodes/States | Pruned | Memory | Objective | Status |
|---|---:|---:|---:|---:|---:|---|
| Brute Force | | | | | | |
| Greedy | | | | | | |
| Dynamic Programming | | | | | | |
| Backtracking | | | | | | |
| Branch and Bound | | | | | | |

Include:

* Optimality status.
* Optimality gap when available.
* Termination reason.

Use appropriate badges for:

* Optimal.
* Feasible, not proven optimal.
* No feasible plan found.
* Infeasible.
* Interrupted.

Do not use misleading colors or labels.

### Benchmark charts

Include separate visualizations for:

1. Execution time comparison.
2. States/nodes explored.
3. Branches pruned.
4. Memory consumption.
5. Objective score comparison.

Ensure objective values are interpreted correctly for minimization versus maximization.

Do not compare objective values from different optimization modes.

### Backtracking meal plan

Create a dedicated subsection titled:

**Meal Plan Generated by Backtracking**

Display the actual seven-day meal plan returned by Backtracking.

Show:

* Recipe assignments.
* Daily nutritional totals.
* Weekly grocery expenditure.
* Nutritional deviation.
* Feasibility status.

If Backtracking did not find a feasible solution, show its actual status and reason instead of inventing a plan.

### Algorithm explanation

Provide an expandable explanation for each algorithm:

* How it works.
* How it explores the search space.
* Its pruning strategy, if applicable.
* Its time and space complexity.
* Why its performance may differ from the other algorithms.

Keep explanations understandable to a university DAA student.

---

# 15. Backend REST API

Implement the backend using Crow.

Provide endpoints similar to the following.

| Method | Endpoint | Purpose |
|---|---|---|
| GET | `/api/health` | Server health |
| POST | `/api/dataset/upload` | Upload Excel or CSV dataset |
| GET | `/api/dataset/summary` | Get dataset summary |
| GET | `/api/dataset/ingredients` | Get ingredients |
| GET | `/api/dataset/recipes` | Get recipes |
| POST | `/api/optimization/validate-config` | Validate configuration |
| POST | `/api/optimization/generate-plan` | Generate a plan for one mode |
| POST | `/api/algorithms/benchmark` | Run all five algorithms |
| GET | `/api/results/latest` | Retrieve latest results |
| POST | `/api/export/excel` | Generate output workbook |

Use JSON requests and responses except for file uploads and downloads.

Return meaningful HTTP status codes and error messages.

The backend must:

* Retain the active validated dataset during the application session.
* Validate all requests.
* Avoid global mutable state where it is unnecessary.
* Handle failed uploads safely.
* Avoid crashes on invalid input.
* Return structured JSON.
* Prevent simultaneous requests from corrupting shared state.

Use localhost-only development CORS configuration.

The frontend must communicate with the C++ backend through these endpoints.

Do not move optimization logic into frontend code.

---

# 16. Excel export

Excel export is a required final feature.

Generate a workbook containing **exactly three worksheets**.

## 16.1 Sheet 1: Weekly_Meal_Plan

Include:

* Day.
* Meal type.
* Recipe ID.
* Recipe name.
* Servings.
* Calories.
* Protein.
* Carbohydrates.
* Fat.
* Gross recipe cost.
* Estimated purchase cost contribution, if allocated.

Include a weekly summary.

## 16.2 Sheet 2: Nutritional_Summary

Include:

* Day.
* Actual calories.
* Target calories.
* Minimum calories.
* Maximum calories.
* Actual protein.
* Target protein.
* Minimum protein.
* Actual carbohydrates.
* Target carbohydrates.
* Minimum carbohydrates.
* Actual fat.
* Target fat.
* Maximum fat.
* Daily nutrition deviation.

Include weekly totals or averages where appropriate.

## 16.3 Sheet 3: Grocery_List

Include:

* Ingredient ID.
* Ingredient name.
* Total required quantity in grams.
* Available inventory in grams.
* Purchase quantity in grams.
* Price per kg.
* Estimated purchase cost.

Only purchase quantities greater than zero need to be listed, but all ingredients used in the plan must be accounted for.

Include the total estimated grocery expenditure.

## 16.4 Export requirements

* Use OpenXLSX.
* Format headers clearly.
* Apply appropriate column widths.
* Freeze header rows.
* Use currency and numeric formatting.
* Avoid unnecessary decimal places in presentation.
* Ensure the workbook can be opened in Microsoft Excel.
* Verify all three worksheets exist and contain the expected data.
* Do not add unrelated worksheets.

The export must reflect the selected optimization mode and its actual generated plan.

---

# 17. Testing requirements

Write automated tests throughout development.

## 17.1 Parser tests

Test:

* Valid Excel workbook.
* Valid CSV dataset.
* Missing worksheet.
* Missing column.
* Invalid numeric values.
* Duplicate IDs.
* Invalid references.
* Missing inventory.
* Missing dietary classification.

## 17.2 Calculator tests

Manually verify the cost and nutrition calculations of at least two recipes.

Test quantities and prices with fractional values.

## 17.3 Evaluator tests

Test:

* A fully feasible plan.
* Weekly budget violation.
* Minimum calorie violation.
* Maximum calorie violation.
* Protein violation.
* Carbohydrate violation.
* Fat violation.
* Vegetarian incompatibility.
* Vegan incompatibility.
* Excluded ingredient.
* Maximum repetition violation.
* Incorrect meal type.
* Missing required meal slot.
* Inventory deduction from grocery purchases.
* Target deviation calculation.

## 17.4 Algorithm tests

Use small datasets where exhaustive enumeration can establish the true optimum.

For each optimization mode:

1. Run Brute Force to establish the optimum.
2. Run Backtracking.
3. Run Branch and Bound.
4. Run Dynamic Programming.
5. Verify that every completed exact algorithm obtains the same primary optimum.
6. Check Greedy feasibility and objective quality without expecting optimality.
7. Verify that all returned feasible plans pass the same evaluator.

Test cases must include both feasible and infeasible configurations.

## 17.5 Integration tests

Test the complete workflow:

1. Upload workbook.
2. Validate dataset.
3. Save configuration.
4. Generate Nutrition plan.
5. Generate Budget plan.
6. Generate Variety plan.
7. Run algorithm benchmark.
8. View Backtracking result.
9. Export Excel workbook.
10. Reopen and validate the exported workbook.

---

# 18. Performance and reliability

The application should remain responsive.

Requirements:

* Do not block the frontend while algorithms run.
* Use asynchronous frontend requests.
* Display a loading indicator during optimization.
* Display algorithm execution status.
* Handle timeout and interrupted results.
* Avoid unnecessary recalculation of recipe nutrition.
* Precompute compatible recipe candidates.
* Use efficient data structures.
* Avoid repeated linear searches when hash maps are appropriate.
* Do not store unnecessary copies of large datasets.
* Use safe memory management.
* Use clear error handling.

For exact algorithms, report execution limits and termination reasons.

Never hide the fact that an algorithm was interrupted.

---

# 19. Documentation

Create the following documentation.

## README.md

Include:

* Project overview.
* Features.
* Technology stack.
* Requirements.
* Installation instructions.
* Build instructions.
* Run instructions.
* Frontend instructions.
* Backend instructions.
* Dataset format.
* API overview.

## MATHEMATICAL_FORMULATION.md

Include:

* Decision variables.
* Meal assignment constraints.
* Nutritional constraints.
* Budget constraint.
* Recipe repetition constraint.
* Dietary constraints.
* Ingredient exclusion constraints.
* Nutrition objective.
* Budget objective.
* Variety objective.
* Normalization formula.
* Common evaluator.
* Each algorithm's approach.
* Bound calculations.
* Optimality and gap definitions.

## USER_GUIDE.md

Explain:

* How to prepare the Excel input.
* How to upload the dataset.
* How to configure daily nutritional constraints.
* How to enter explicit daily targets.
* How to select diet preferences.
* How to generate meal plans.
* How to compare algorithms.
* How to interpret benchmark results.
* How to export the final workbook.

---

# 20. Development order

Complete the project in this exact order.

### Phase 1 — Project setup and input handling

* Create project structure.
* Configure CMake.
* Configure C++17.
* Install and configure dependencies.
* Create the input Excel template.
* Implement Excel and CSV reading.
* Implement dataset validation.
* Implement recipe cost and nutrition calculations.
* Test all input functionality.

### Phase 2 — Optimization formulation

* Implement OptimizationConfig.
* Implement decision model.
* Implement daily nutritional calculations.
* Implement dietary compatibility.
* Implement hard constraints.
* Implement explicit nutritional targets.
* Implement all three objective functions.
* Implement shared PlanEvaluator.
* Write mathematical documentation.
* Test feasibility and objective calculations.

### Phase 3 — Five algorithms

Implement in this order:

1. Brute Force.
2. Greedy.
3. Dynamic Programming.
4. Backtracking.
5. Branch and Bound.

Then implement:

* Common result structure.
* Execution limits.
* Benchmarking.
* Optimality status.
* Optimality gap.
* Algorithm tests.

### Phase 4 — Backend and dashboard

* Implement Crow server.
* Implement REST endpoints.
* Implement React application.
* Implement Dataset page.
* Implement Settings page.
* Implement Dashboard page.
* Implement Meals page.
* Implement Algorithms page.
* Integrate charts.
* Connect every page to the backend.
* Test the complete workflow.

### Phase 5 — Excel export and final integration

* Implement OpenXLSX exporter.
* Generate exactly three output worksheets.
* Test exported data.
* Verify all application workflows.
* Fix bugs.
* Update documentation.
* Prepare final build and run instructions.

Do not skip a phase or leave it partially implemented before moving on.

---

# 21. Final acceptance criteria

The project is complete only when all of the following work:

* [ ] User can download the input Excel template.
* [ ] User can upload a valid Excel workbook.
* [ ] User can upload four CSV files.
* [ ] Dataset validation works.
* [ ] Recipe nutrition is calculated correctly.
* [ ] Recipe cost is calculated correctly.
* [ ] Ingredient diet classifications are respected.
* [ ] Vegetarian and vegan restrictions work correctly.
* [ ] User can enter all hard nutritional constraints.
* [ ] User can enter all four explicit daily nutrition targets.
* [ ] User can enter the weekly budget.
* [ ] User can select 2, 3, or 4 meals per day.
* [ ] User can exclude ingredients.
* [ ] User can specify preferred ingredients.
* [ ] User can configure maximum recipe repetitions.
* [ ] Nutrition mode generates a feasible plan when one is found.
* [ ] Budget mode generates a feasible plan when one is found.
* [ ] Variety mode generates a feasible plan when one is found.
* [ ] All five algorithms are implemented in C++.
* [ ] All five algorithms use the same evaluator.
* [ ] Algorithm comparison uses identical configurations.
* [ ] Execution time is measured.
* [ ] States/nodes explored are measured.
* [ ] Branches pruned are measured.
* [ ] Memory is measured consistently.
* [ ] Optimality status is reported correctly.
* [ ] Backtracking's actual meal plan is displayed.
* [ ] Infeasible and interrupted cases are handled correctly.
* [ ] Excel export generates exactly three worksheets.
* [ ] Exported grocery quantities account for inventory.
* [ ] All automated tests pass.
* [ ] README and technical documentation are complete.
* [ ] The application can be built and run from a clean setup.

---

# 22. Final instructions

Start by inspecting the current workspace.

If it is empty, create the entire project.

If some files already exist, inspect them and reuse working components where appropriate.

Make every technical decision using the specifications above. Do not ask the user to choose between alternative approaches.

Do not stop after writing the architecture or mathematical formulation. Implement the actual application.

After every major phase:

1. Compile the C++ project.
2. Run relevant tests.
3. Fix errors.
4. Verify the functionality.
5. Continue to the next phase.

At the end, provide:

1. A concise summary of implemented features.
2. Final project directory structure.
3. Build and run instructions for Windows VS Code with MSYS2 UCRT64.
4. Frontend installation and run instructions.
5. Test results.
6. Any genuine limitations that remain.

**The goal is a complete, functioning SmartMeal Optimizer application with a correct optimization model, five genuinely implemented algorithms, a polished interactive dashboard, and working Excel input/output. Do not substitute mock data, simulated algorithm results, or placeholder implementations for actual functionality.**