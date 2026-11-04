# User Guide — SmartMeal Optimizer

## 1. Preparing Your Dataset

### 1.1 Ingredients CSV

Create a file `ingredients.csv` with these columns:

```
Ingredient_ID, Name, Price_per_kg_INR, Calories_per_100g, Protein_g_per_100g, Carbs_g_per_100g, Fat_g_per_100g, Diet_Category
```

**Example:**
```csv
Ingredient_ID,Name,Price_per_kg_INR,Calories_per_100g,Protein_g_per_100g,Carbs_g_per_100g,Fat_g_per_100g,Diet_Category
I001,Basmati Rice,60,350,7,78,0.5,Plant-based
I002,Toor Dal,130,340,22,60,1,Plant-based
I003,Paneer,320,265,18,3,20,Dairy
```

> **Diet_Category must be one of:** Plant-based · Dairy · Eggs · Meat · Fish/Seafood · Animal-derived

### 1.2 Recipes CSV

```
Recipe_ID, Recipe_Name, Meal_Type
```

Meal_Type must be: `Breakfast`, `Lunch`, `Snack`, or `Dinner`.

```csv
Recipe_ID,Recipe_Name,Meal_Type
R001,Dal Chawal,Lunch
R002,Upma,Breakfast
```

### 1.3 Recipe Ingredients CSV

```
Recipe_ID, Ingredient_ID, Quantity_g_per_serving
```

```csv
Recipe_ID,Ingredient_ID,Quantity_g_per_serving
R001,I001,150
R001,I002,80
```

### 1.4 Inventory CSV (Optional)

```
Ingredient_ID, Available_Quantity_g
```

```csv
Ingredient_ID,Available_Quantity_g
I001,500
```

Inventory reduces the amount you need to purchase; it does not make recipes unavailable.

---

## 2. Uploading Your Dataset

1. Open SmartMeal Optimizer in your browser (http://localhost:5173)
2. Navigate to **Dataset** in the left sidebar
3. On the **Upload** tab, enter the full paths to your CSV files
4. Click **Load Dataset**
5. Check the **Validation** tab for any errors or warnings
6. Fix fatal errors before proceeding to optimization

---

## 3. Configuring Optimization

Navigate to **Settings**:

### Weekly Settings
- **Weekly Budget (₹):** Maximum grocery purchase cost for the week
- **Meals per Day:** 2, 3, or 4 (determines which meal slots are planned)
- **Max Recipe Repetitions:** How many times the same recipe can appear across the week

### Nutritional Constraints (Hard Limits)
These are non-negotiable. Plans outside these ranges are infeasible.

| Field | Meaning |
|-------|---------|
| Min Calories | Minimum kcal per day |
| Max Calories | Maximum kcal per day |
| Min Protein | Minimum grams of protein per day |
| Min Carbohydrates | Minimum grams of carbs per day |
| Max Fat | Maximum grams of fat per day |

### Explicit Daily Targets
Used by Nutrition mode to minimize deviation. Must satisfy:
- Target calories ∈ [min calories, max calories]
- Target protein ≥ min protein
- Target carbs ≥ min carbs
- Target fat ≤ max fat

### Diet Settings
- **Vegetarian:** Allows Plant-based, Dairy, Eggs
- **Vegan:** Allows only Plant-based
- **Non-vegetarian / Unrestricted:** All categories allowed

Diet is enforced using the `Diet_Category` column — never by ingredient name.

---

## 4. Generating Meal Plans

1. Go to **Meals**
2. Select the optimization mode (Nutrition / Budget / Variety)
3. Select an algorithm (Backtracking recommended for best results within the time limit)
4. Click **Generate Plan**
5. View the 7-day calendar and charts
6. Switch tabs to compare all three modes

---

## 5. Comparing Algorithms

1. Go to **Algorithms**
2. Select the optimization mode
3. Click **Run All Algorithms**
4. All five algorithms run with identical configuration and limits
5. Review the comparison table:
   - **Execution Time:** Wall-clock time in milliseconds
   - **Nodes/States:** How many states were explored
   - **Pruned:** Branches eliminated without exploring
   - **Memory:** Algorithm-owned state memory estimate
   - **Objective:** Primary objective score (lower = better for Nutrition/Budget; higher = better for Variety)
   - **Status:** Optimal / Feasible / No Feasible Found / Infeasible / Interrupted

---

## 6. Interpreting Results

### Status badges

| Badge | Meaning |
|-------|---------|
| **Optimal** | An exhaustive search proved this is the best possible plan |
| **Feasible (not proven optimal)** | A valid plan was found, but better plans may exist |
| **No feasible solution found** | No valid plan found before the time/node limit |
| **Infeasible** | An exhaustive proof that no valid plan exists for this configuration |
| **Interrupted** | Search was cut off by the execution limit |

> **Important:** "No feasible solution found" does NOT mean "infeasible." It means the algorithm did not find one within its limits.

---

## 7. Exporting to Excel

1. After generating a plan, click the **Export** button in the top-right
2. The exported workbook contains exactly three sheets:
   - **Weekly_Meal_Plan** — recipe assignments, nutrition, and cost per day
   - **Nutritional_Summary** — daily actual vs. target vs. constraint nutrition
   - **Grocery_List** — ingredients, inventory, purchase quantities, and cost

---

## 8. Tips

- Start with generous constraints (wide calorie range, high budget) to verify your dataset produces feasible plans before tightening them
- Use Greedy first for a quick result, then run Backtracking or Branch & Bound for a higher-quality solution
- If no feasible plan is found, check whether your nutritional constraints are achievable with the recipes in your dataset
- Inventory quantities reduce what you need to buy but do not limit recipe selection

