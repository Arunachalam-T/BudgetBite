# Mathematical Formulation — SmartMeal Optimizer

## Decision Variables

$$x_{d,m,r} \in \{0, 1\}$$

Where:
- $d$: day index, $d \in \{0, \ldots, 6\}$
- $m$: meal slot (Breakfast, Lunch, Snack, Dinner)
- $r$: recipe ID

$x_{d,m,r} = 1$ means recipe $r$ is assigned to slot $m$ on day $d$.

---

## Constraints

### C1. Meal Assignment (Hard)
Every meal slot has exactly one recipe:

$$\sum_r x_{d,m,r} = 1 \quad \forall d, m$$

### C2. Calorie Range (Hard)
$$\text{MinCalories} \le C_d \le \text{MaxCalories} \quad \forall d$$

where $C_d = \sum_{m,r} x_{d,m,r} \cdot \text{Cal}_r$

### C3. Minimum Protein (Hard)
$$\text{Protein}_d \ge \text{MinProtein} \quad \forall d$$

### C4. Minimum Carbohydrates (Hard)
$$\text{Carbs}_d \ge \text{MinCarbs} \quad \forall d$$

### C5. Maximum Fat (Hard)
$$\text{Fat}_d \le \text{MaxFat} \quad \forall d$$

### C6. Weekly Budget (Hard)
$$\text{WeeklyPurchaseCost} \le \text{WeeklyBudget}$$

$$\text{WeeklyPurchaseCost} = \sum_i \frac{\max(0,\, \text{TotalRequired}_i - \text{Inventory}_i)}{1000} \cdot \text{Price}_i$$

### C7. Recipe Repetition (Hard)
$$\sum_{d,m} x_{d,m,r} \le \text{MaxRepetitions} \quad \forall r$$

### C8. Diet Compatibility (Hard)
No selected recipe contains an ingredient with a diet category incompatible with the chosen diet type.

### C9. Ingredient Exclusion (Hard)
No selected recipe contains an excluded ingredient.

---

## Objective Functions

### Mode 1: Nutrition (Minimize deviation)

Normalized deviation for nutrient $n$ on day $d$:

$$D_{d,n} = \frac{|\text{Actual}_{d,n} - \text{Target}_n|}{\max(\text{Target}_n, \varepsilon)}$$

Aggregate score (lower = better):

$$D = \frac{1}{7} \sum_{d=1}^{7} \frac{1}{4} \sum_{n \in \{\text{Cal, Prot, Carb, Fat}\}} D_{d,n}$$

$$\boxed{\min D}$$

### Mode 2: Budget (Minimize purchase cost)

$$\boxed{\min \text{WeeklyPurchaseCost}}$$

### Mode 3: Variety (Maximize distinct recipes)

$$\boxed{\max |\{r : \exists d,m \text{ s.t. } x_{d,m,r}=1\}|}$$

Display score: $\text{VarietyScore} = 100 \times \frac{\text{DistinctRecipes}}{\text{TotalSlots}}$

---

## Tie-Breaking (Lexicographic)

When two plans have equal primary objective scores:
1. Prefer lower weekly purchase cost
2. Prefer lower nutrition deviation score
3. Prefer lexicographically smaller recipe-ID assignment

---

## Algorithm Approaches

### Brute Force
Enumerates all $\prod_{d,m} |C_{m}|$ assignments exhaustively. Proven optimal if exhausted.

### Greedy
At each slot, selects the locally best candidate under the active objective. $O(7m \cdot |R|)$.

### Dynamic Programming
State: (slot\_index, repetition\_counts, per-day nutrition rounded to nearest unit).
Recurrence: $V(\text{state}) = \text{best}_{r} V(\text{state'})$ where state' is the next state after assigning $r$.
Memoized via a `std::map`. Exact when all states explored.

### Backtracking
Depth-first search with pruning:
- Repetition limit exceeded → prune
- Per-day fat already > MaxFat → prune
- Proven optimal if exhausted

### Branch and Bound
Best-first search on a priority queue ordered by optimistic objective bound:
- Budget mode: lower bound = current gross cost (admissible since purchase ≤ gross)
- Variety mode: upper bound = current distinct + remaining slots
- Nutrition mode: lower bound = 0 (admissible)

Optimality gap when interrupted:
$$\text{Gap} = \frac{|\text{Incumbent} - \text{Bound}|}{\max(|\text{Incumbent}|, \varepsilon)} \times 100$$

