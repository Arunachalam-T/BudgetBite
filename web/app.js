// =============================================================================
// BudgetBite - Affordable Meal Planning System
// Client-Side Application Logic (Vanilla JavaScript - No Frameworks)
// =============================================================================

(function() {
    'use strict';

    // Application State
    let appData = null;
    let activeAlgo = 'dp';         // 'dp' | 'greedy'
    let activeGroceryAlgo = 'dp';  // 'dp' | 'greedy'
    let recipeCatalog = [];
    let recipeMap = new Map();
    let currentPage = 1;
    const PAGE_SIZE = 18;

    // DOM Elements
    const navTabs = document.querySelectorAll('.nav-btn');
    const tabPanels = document.querySelectorAll('.tab-panel');
    const recipeModal = document.getElementById('recipeModal');
    const modalCloseBtn = document.getElementById('modalCloseBtn');
    const modalBody = document.getElementById('modalBody');

    // Initialize App
    async function init() {
        setupNavigation();
        setupAlgoToggles();
        setupRecipeFilters();
        setupParameterForm();
        setupExports();
        setupModal();

        // Load JSON Data
        await loadMealPlanData();
    }

    // ── Navigation Tab Handling ───────────────────────────────────────────────
    function setupNavigation() {
        navTabs.forEach(btn => {
            btn.addEventListener('click', () => {
                const target = btn.dataset.tab;
                navTabs.forEach(b => b.classList.remove('active'));
                tabPanels.forEach(p => p.classList.remove('active'));
                btn.classList.add('active');
                const panel = document.getElementById(`tab-${target}`);
                if (panel) panel.classList.add('active');

                // Re-render chart if navigating to comparison or dashboard
                if (target === 'comparison') renderComparisonVisuals();
                if (target === 'dashboard') renderDashboardCharts();
            });
        });
    }

    // ── Algorithm Toggle Handling ─────────────────────────────────────────────
    function setupAlgoToggles() {
        // Dashboard / Planner toggles
        document.querySelectorAll('.algo-pill-btn[data-algo]').forEach(btn => {
            btn.addEventListener('click', (e) => {
                const algo = e.target.dataset.algo;
                activeAlgo = algo;
                document.querySelectorAll('.algo-pill-btn[data-algo]').forEach(b => {
                    b.classList.toggle('active', b.dataset.algo === algo);
                });
                renderDashboard();
                renderPlanner();
            });
        });

        // Grocery list toggle
        document.querySelectorAll('.algo-pill-btn[data-grocery-algo]').forEach(btn => {
            btn.addEventListener('click', (e) => {
                const algo = e.target.dataset.groceryAlgo;
                activeGroceryAlgo = algo;
                document.querySelectorAll('.algo-pill-btn[data-grocery-algo]').forEach(b => {
                    b.classList.toggle('active', b.dataset.groceryAlgo === algo);
                });
                renderGroceryList();
            });
        });
    }

    // ── Data Loading ──────────────────────────────────────────────────────────
    async function loadMealPlanData() {
        // Priority 1: Instant load from companion script (works under file:// protocol)
        if (window.BUDGETBITE_DATA) {
            handleDataLoaded(window.BUDGETBITE_DATA);
            return;
        }

        // Priority 2: Fetch JSON (works under http:// or local static servers)
        try {
            const resp = await fetch('meal_plan.json');
            if (!resp.ok) throw new Error(`HTTP error! status: ${resp.status}`);
            const data = await resp.json();
            handleDataLoaded(data);
        } catch (err) {
            console.warn('Auto-fetch of meal_plan.json failed (likely local file:// CORS). Prompting manual upload or fallback.', err);
            showFilePickerFallback();
        }
    }

    function handleDataLoaded(data) {
        appData = data;
        recipeCatalog = data.recipes || [];
        recipeMap.clear();
        recipeCatalog.forEach(r => recipeMap.set(r.id, r));

        renderDashboard();
        renderPlanner();
        renderComparison();
        renderRecipeCatalog();
        renderGroceryList();
    }

    function showFilePickerFallback() {
        const dashContainer = document.getElementById('tab-dashboard');
        const alertBox = document.createElement('div');
        alertBox.className = 'card';
        alertBox.style.borderColor = 'var(--warning)';
        alertBox.style.marginBottom = '2rem';
        alertBox.innerHTML = `
            <div style="display: flex; gap: 1rem; align-items: flex-start;">
                <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#F59E0B" stroke-width="2"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>
                <div style="flex: 1;">
                    <h3 style="color: #FCD34D; font-size: 1.1rem; margin-bottom: 0.35rem;">Load Generated Meal Plan JSON</h3>
                    <p style="color: var(--text-secondary); font-size: 0.88rem; margin-bottom: 1rem;">
                        If you opened this HTML page directly via <code style="color: #FCD34D;">file://</code>, browser security prevents automatic local file loading. Select the generated <strong>web/meal_plan.json</strong> file below to load the complete plan.
                    </p>
                    <input type="file" id="fallbackFileInput" accept=".json" class="btn btn-outline btn-sm">
                </div>
            </div>
        `;
        dashContainer.prepend(alertBox);

        document.getElementById('fallbackFileInput').addEventListener('change', (e) => {
            const file = e.target.files[0];
            if (file) {
                const reader = new FileReader();
                reader.onload = (event) => {
                    try {
                        const parsed = JSON.parse(event.target.result);
                        alertBox.remove();
                        handleDataLoaded(parsed);
                    } catch (ex) {
                        alert('Invalid JSON file format: ' + ex.message);
                    }
                };
                reader.readAsText(file);
            }
        });
    }

    // ── Dashboard Rendering ───────────────────────────────────────────────────
    function renderDashboard() {
        if (!appData) return;
        const plan = (activeAlgo === 'dp') ? appData.dp_plan : appData.greedy_plan;
        const budgetTarget = appData.budget_inr || 700;
        const kcalTarget = appData.daily_kcal_target || 2000;
        const protTarget = appData.daily_protein_g || 50;

        // Metric values
        document.getElementById('dashTotalCost').textContent = `₹${plan.total_cost_inr.toFixed(2)}`;
        document.getElementById('dashRemainingBudget').textContent = `₹${plan.budget_remaining.toFixed(2)}`;
        document.getElementById('dashBudgetTarget').textContent = budgetTarget.toFixed(0);
        document.getElementById('dashBudgetPercent').textContent = `${((plan.budget_remaining / budgetTarget) * 100).toFixed(1)}%`;

        // Badges
        const budgetBadge = document.getElementById('dashBudgetBadge');
        if (plan.budget_satisfied) {
            budgetBadge.className = 'metric-badge badge-success';
            budgetBadge.textContent = 'Within Budget';
        } else {
            budgetBadge.className = 'metric-badge badge-danger';
            budgetBadge.textContent = 'Budget Exceeded';
        }

        // Daily Averages
        const days = plan.days || [];
        let totalKcal = 0, totalProt = 0;
        days.forEach(d => {
            totalKcal += d.total_kcal;
            totalProt += d.total_protein_g;
        });
        const avgKcal = days.length ? (totalKcal / days.length) : 0;
        const avgProt = days.length ? (totalProt / days.length) : 0;

        document.getElementById('dashAvgCalories').textContent = `${avgKcal.toFixed(0)} kcal`;
        document.getElementById('dashKcalTarget').textContent = kcalTarget.toFixed(0);
        const kcalBadge = document.getElementById('dashKcalBadge');
        if (avgKcal >= kcalTarget * 0.8) {
            kcalBadge.className = 'metric-badge badge-success';
            kcalBadge.textContent = `${((avgKcal / kcalTarget) * 100).toFixed(0)}% Target`;
        } else {
            kcalBadge.className = 'metric-badge badge-warning';
            kcalBadge.textContent = `${((avgKcal / kcalTarget) * 100).toFixed(0)}% (Low)`;
        }

        document.getElementById('dashAvgProtein').textContent = `${avgProt.toFixed(1)} g`;
        document.getElementById('dashProtTarget').textContent = protTarget.toFixed(0);
        const protBadge = document.getElementById('dashProtBadge');
        if (avgProt >= protTarget * 0.8) {
            protBadge.className = 'metric-badge badge-success';
            protBadge.textContent = `${((avgProt / protTarget) * 100).toFixed(0)}% Target`;
        } else {
            protBadge.className = 'metric-badge badge-warning';
            protBadge.textContent = `${((avgProt / protTarget) * 100).toFixed(0)}% (Low)`;
        }

        // Highlights Card
        document.getElementById('dashAlgoBadge').textContent = (activeAlgo === 'dp') ? 'DP Plan Active' : 'Greedy Plan Active';
        document.getElementById('dashDietPref').textContent = appData.diet_pref || 'Any';
        document.getElementById('dashTotalRecipes').textContent = (appData.total_recipes || recipeCatalog.length).toLocaleString();

        renderDashboardCharts();
    }

    // ── Dashboard SVG Trajectory Chart ────────────────────────────────────────
    function renderDashboardCharts() {
        if (!appData) return;
        const plan = (activeAlgo === 'dp') ? appData.dp_plan : appData.greedy_plan;
        const container = document.getElementById('dashChartDaily');
        if (!container) return;

        const days = plan.days || [];
        if (!days.length) {
            container.innerHTML = '<div style="color:var(--text-muted); text-align:center; padding: 2rem;">No plan data available</div>';
            return;
        }

        const maxKcal = Math.max(...days.map(d => d.total_kcal), appData.daily_kcal_target || 2000) * 1.15;
        const width = container.clientWidth || 580;
        const height = 220;
        const padding = { top: 20, right: 30, bottom: 40, left: 55 };
        const plotW = width - padding.left - padding.right;
        const plotH = height - padding.top - padding.bottom;

        // Target line Y
        const targetY = padding.top + plotH - ((appData.daily_kcal_target || 2000) / maxKcal) * plotH;

        // Points
        const points = days.map((d, i) => {
            const x = padding.left + (i / (days.length - 1)) * plotW;
            const y = padding.top + plotH - (d.total_kcal / maxKcal) * plotH;
            return { x, y, kcal: d.total_kcal, day: d.day };
        });

        const linePath = points.map((p, i) => `${i === 0 ? 'M' : 'L'} ${p.x.toFixed(1)} ${p.y.toFixed(1)}`).join(' ');
        const areaPath = `${linePath} L ${points[points.length-1].x} ${padding.top + plotH} L ${points[0].x} ${padding.top + plotH} Z`;

        let svg = `
            <svg viewBox="0 0 ${width} ${height}" style="width: 100%; height: 100%; overflow: visible;">
                <defs>
                    <linearGradient id="areaGrad" x1="0" y1="0" x2="0" y2="1">
                        <stop offset="0%" stop-color="#6366F1" stop-opacity="0.35"/>
                        <stop offset="100%" stop-color="#6366F1" stop-opacity="0.0"/>
                    </linearGradient>
                </defs>
                <!-- Grid lines -->
                <line x1="${padding.left}" y1="${padding.top + plotH}" x2="${width - padding.right}" y2="${padding.top + plotH}" stroke="rgba(255,255,255,0.1)" stroke-width="1"/>
                <line x1="${padding.left}" y1="${targetY}" x2="${width - padding.right}" y2="${targetY}" stroke="#F59E0B" stroke-dasharray="4,4" stroke-width="1.5"/>
                <text x="${width - padding.right}" y="${targetY - 6}" fill="#FBBF24" font-size="11" text-anchor="end" font-weight="600">Target (${appData.daily_kcal_target} kcal)</text>

                <!-- Area & Line -->
                <path d="${areaPath}" fill="url(#areaGrad)"/>
                <path d="${linePath}" fill="none" stroke="#818CF8" stroke-width="3" stroke-linecap="round"/>

                <!-- Dots -->
                ${points.map(p => `
                    <circle cx="${p.x.toFixed(1)}" cy="${p.y.toFixed(1)}" r="5" fill="#6366F1" stroke="#FFF" stroke-width="2"/>
                    <text x="${p.x.toFixed(1)}" y="${padding.top + plotH + 20}" fill="#94A3B8" font-size="11" text-anchor="middle" font-weight="600">Day ${p.day}</text>
                    <text x="${p.x.toFixed(1)}" y="${p.y - 10}" fill="#F8FAFC" font-size="11" text-anchor="middle" font-weight="700">${p.kcal.toFixed(0)}</text>
                `).join('')}

                <!-- Y Axis Label -->
                <text x="${padding.left - 10}" y="${padding.top + plotH / 2}" fill="#64748B" font-size="10" text-anchor="middle" transform="rotate(-90 ${padding.left - 10} ${padding.top + plotH / 2})">Calories (kcal)</text>
            </svg>
        `;
        container.innerHTML = svg;
    }

    // ── 7-Day Meal Planner Rendering ──────────────────────────────────────────
    function renderPlanner() {
        if (!appData) return;
        const plan = (activeAlgo === 'dp') ? appData.dp_plan : appData.greedy_plan;
        const container = document.getElementById('plannerDaysContainer');
        if (!container) return;
        container.innerHTML = '';

        const days = plan.days || [];
        days.forEach(d => {
            const bfRecipe = recipeMap.get(d.breakfast_id) || { name: d.breakfast_id || 'Breakfast not set', kcal: 0, protein_g: 0, cost_inr: 0 };
            const luRecipe = recipeMap.get(d.lunch_id)     || { name: d.lunch_id || 'Lunch not set', kcal: 0, protein_g: 0, cost_inr: 0 };
            const diRecipe = recipeMap.get(d.dinner_id)    || { name: d.dinner_id || 'Dinner not set', kcal: 0, protein_g: 0, cost_inr: 0 };

            const dayCard = document.createElement('div');
            dayCard.className = 'day-card';
            dayCard.innerHTML = `
                <div class="day-header">
                    <div class="day-title-badge">
                        <span class="day-number-tag">Day ${d.day}</span>
                        <div class="day-summary-chips">
                            <span class="day-chip">Cost: <strong>₹${d.total_cost_inr.toFixed(2)}</strong></span>
                            <span class="day-chip">Energy: <strong>${d.total_kcal.toFixed(0)} kcal</strong></span>
                            <span class="day-chip">Protein: <strong>${d.total_protein_g.toFixed(1)} g</strong></span>
                            <span class="day-chip">Carbs: <strong>${d.total_carb_g.toFixed(1)} g</strong></span>
                        </div>
                    </div>
                    <div>
                        ${d.kcal_met ? '<span class="metric-badge badge-success">Kcal Met</span>' : '<span class="metric-badge badge-warning">Low Kcal</span>'}
                        ${d.protein_met ? '<span class="metric-badge badge-success">Protein Met</span>' : '<span class="metric-badge badge-warning">Low Protein</span>'}
                    </div>
                </div>

                <div class="day-meals-grid">
                    <!-- Breakfast -->
                    <div class="meal-slot-card" data-recipe-id="${d.breakfast_id}">
                        <div>
                            <div class="slot-tag">
                                <span>🌅 Breakfast</span>
                                <span>₹${(bfRecipe.cost_inr || 0).toFixed(1)}</span>
                            </div>
                            <div class="recipe-name">${bfRecipe.name}</div>
                        </div>
                        <div class="recipe-micro-stats">
                            <span>${(bfRecipe.kcal || 0).toFixed(0)} kcal</span>
                            <span>${(bfRecipe.protein_g || 0).toFixed(1)}g prot</span>
                            <span class="cost">${bfRecipe.cost_complete ? 'Complete Est.' : 'Partial Est.'}</span>
                        </div>
                    </div>

                    <!-- Lunch -->
                    <div class="meal-slot-card" data-recipe-id="${d.lunch_id}">
                        <div>
                            <div class="slot-tag" style="color: #38BDF8;">
                                <span>☀️ Lunch</span>
                                <span>₹${(luRecipe.cost_inr || 0).toFixed(1)}</span>
                            </div>
                            <div class="recipe-name">${luRecipe.name}</div>
                        </div>
                        <div class="recipe-micro-stats">
                            <span>${(luRecipe.kcal || 0).toFixed(0)} kcal</span>
                            <span>${(luRecipe.protein_g || 0).toFixed(1)}g prot</span>
                            <span class="cost">${luRecipe.cost_complete ? 'Complete Est.' : 'Partial Est.'}</span>
                        </div>
                    </div>

                    <!-- Dinner -->
                    <div class="meal-slot-card" data-recipe-id="${d.dinner_id}">
                        <div>
                            <div class="slot-tag" style="color: #C084FC;">
                                <span>🌙 Dinner</span>
                                <span>₹${(diRecipe.cost_inr || 0).toFixed(1)}</span>
                            </div>
                            <div class="recipe-name">${diRecipe.name}</div>
                        </div>
                        <div class="recipe-micro-stats">
                            <span>${(diRecipe.kcal || 0).toFixed(0)} kcal</span>
                            <span>${(diRecipe.protein_g || 0).toFixed(1)}g prot</span>
                            <span class="cost">${diRecipe.cost_complete ? 'Complete Est.' : 'Partial Est.'}</span>
                        </div>
                    </div>
                </div>
            `;
            container.appendChild(dayCard);
        });

        // Attach click listeners to cards to open modal
        container.querySelectorAll('.meal-slot-card').forEach(card => {
            card.addEventListener('click', () => {
                const rid = card.dataset.recipeId;
                if (rid) openRecipeModal(rid);
            });
        });
    }

    // ── Algorithm Analysis Tab Rendering ──────────────────────────────────────
    function renderComparison() {
        if (!appData) return;
        const dp = appData.dp_plan || {};
        const gr = appData.greedy_plan || {};

        // Calculate unique recipes for variety metric
        const dpIds = new Set();
        (dp.days || []).forEach(d => { dpIds.add(d.breakfast_id); dpIds.add(d.lunch_id); dpIds.add(d.dinner_id); });
        const grIds = new Set();
        (gr.days || []).forEach(d => { grIds.add(d.breakfast_id); grIds.add(d.lunch_id); grIds.add(d.dinner_id); });

        // Calculate avg kcal & protein
        const dpAvgKcal = (dp.days || []).reduce((acc, d) => acc + d.total_kcal, 0) / (dp.days || [1]).length;
        const grAvgKcal = (gr.days || []).reduce((acc, d) => acc + d.total_kcal, 0) / (gr.days || [1]).length;

        const dpAvgProt = (dp.days || []).reduce((acc, d) => acc + d.total_protein_g, 0) / (dp.days || [1]).length;
        const grAvgProt = (gr.days || []).reduce((acc, d) => acc + d.total_protein_g, 0) / (gr.days || [1]).length;

        const tableBody = document.getElementById('compareTableBody');
        tableBody.innerHTML = `
            <tr>
                <td><strong>Total Cost (7 Days)</strong></td>
                <td><strong style="color: #34D399;">₹${dp.total_cost_inr ? dp.total_cost_inr.toFixed(2) : '0.00'}</strong></td>
                <td><strong>₹${gr.total_cost_inr ? gr.total_cost_inr.toFixed(2) : '0.00'}</strong></td>
                <td>${dp.total_cost_inr <= (appData.budget_inr || 700) ? 'DP optimally allocates up to budget limit' : 'Greedy stops early'}</td>
            </tr>
            <tr>
                <td><strong>Remaining Budget</strong></td>
                <td>₹${dp.budget_remaining ? dp.budget_remaining.toFixed(2) : '0.00'}</td>
                <td>₹${gr.budget_remaining ? gr.budget_remaining.toFixed(2) : '0.00'}</td>
                <td>${gr.budget_remaining > dp.budget_remaining ? 'Greedy leaves high leftover due to rigid ratio stop' : 'DP balanced'}</td>
            </tr>
            <tr>
                <td><strong>Budget Satisfied?</strong></td>
                <td>${dp.budget_satisfied ? '<span class="metric-badge badge-success">YES</span>' : '<span class="metric-badge badge-danger">NO</span>'}</td>
                <td>${gr.budget_satisfied ? '<span class="metric-badge badge-success">YES</span>' : '<span class="metric-badge badge-danger">NO</span>'}</td>
                <td>Both satisfy hard ceiling constraint</td>
            </tr>
            <tr>
                <td><strong>Avg Daily Calories</strong></td>
                <td><strong>${dpAvgKcal.toFixed(0)} kcal</strong></td>
                <td><strong>${grAvgKcal.toFixed(0)} kcal</strong></td>
                <td>${dpAvgKcal >= grAvgKcal ? '<span class="metric-badge badge-success">DP Higher Energy</span>' : '<span class="metric-badge badge-info">Greedy Higher</span>'}</td>
            </tr>
            <tr>
                <td><strong>Avg Daily Protein</strong></td>
                <td><strong>${dpAvgProt.toFixed(1)} g</strong></td>
                <td><strong>${grAvgProt.toFixed(1)} g</strong></td>
                <td>${dpAvgProt >= grAvgProt ? '<span class="metric-badge badge-success">DP Higher Protein</span>' : '<span class="metric-badge badge-info">Greedy Higher</span>'}</td>
            </tr>
            <tr>
                <td><strong>Recipe Variety (Unique / 21)</strong></td>
                <td>${dpIds.size} unique meals</td>
                <td>${grIds.size} unique meals</td>
                <td>${dpIds.size >= grIds.size ? 'DP generates greater variety' : 'Greedy variety'}</td>
            </tr>
            <tr>
                <td><strong>Time Complexity</strong></td>
                <td><code>O(D × S × R × W)</code> (&approx; 7.3M ops)</td>
                <td><code>O(D × S × R log R)</code> (&approx; 40K ops)</td>
                <td>Greedy is faster; DP is global optimal</td>
            </tr>
            <tr>
                <td><strong>Space Complexity</strong></td>
                <td><code>O(W)</code> rolling state + choice table</td>
                <td><code>O(R)</code> candidate sorting buffer</td>
                <td>Both memory footprints are &lt; 2 MB</td>
            </tr>
        `;

        renderComparisonVisuals();
    }

    function renderComparisonVisuals() {
        if (!appData) return;
        const container = document.getElementById('compareChartContainer');
        if (!container) return;

        const dp = appData.dp_plan || {};
        const gr = appData.greedy_plan || {};

        const width = container.clientWidth || 700;
        const height = 260;
        const pad = { top: 30, right: 30, bottom: 40, left: 60 };

        // Metrics to compare: Cost, Avg Kcal, Avg Protein*20
        const dpCost = dp.total_cost_inr || 0;
        const grCost = gr.total_cost_inr || 0;

        const dpDays = dp.days || [];
        const grDays = gr.days || [];
        const dpKcal = dpDays.length ? (dpDays.reduce((a, b) => a + b.total_kcal, 0) / dpDays.length) : 0;
        const grKcal = grDays.length ? (grDays.reduce((a, b) => a + b.total_kcal, 0) / grDays.length) : 0;

        const maxVal = Math.max(dpCost, grCost, dpKcal, grKcal, 1000) * 1.15;
        const barW = 32;

        const groups = [
            { label: 'Weekly Cost (₹)', dp: dpCost, gr: grCost, format: v => `₹${v.toFixed(0)}` },
            { label: 'Daily Energy (kcal)', dp: dpKcal, gr: grKcal, format: v => `${v.toFixed(0)} kcal` }
        ];

        const groupSpacing = (width - pad.left - pad.right) / groups.length;

        let barsSvg = groups.map((g, idx) => {
            const centerX = pad.left + idx * groupSpacing + groupSpacing / 2;
            const dpX = centerX - barW - 6;
            const grX = centerX + 6;

            const dpH = (g.dp / maxVal) * (height - pad.top - pad.bottom);
            const grH = (g.gr / maxVal) * (height - pad.top - pad.bottom);

            const dpY = height - pad.bottom - dpH;
            const grY = height - pad.bottom - grH;

            return `
                <!-- DP Bar -->
                <rect x="${dpX}" y="${dpY}" width="${barW}" height="${dpH}" rx="6" fill="#6366F1"/>
                <text x="${dpX + barW/2}" y="${dpY - 8}" fill="#A5B4FC" font-size="11" font-weight="700" text-anchor="middle">${g.format(g.dp)}</text>
                <text x="${dpX + barW/2}" y="${height - pad.bottom + 16}" fill="#818CF8" font-size="10" font-weight="700" text-anchor="middle">DP</text>

                <!-- Greedy Bar -->
                <rect x="${grX}" y="${grY}" width="${barW}" height="${grH}" rx="6" fill="#10B981"/>
                <text x="${grX + barW/2}" y="${grY - 8}" fill="#6EE7B7" font-size="11" font-weight="700" text-anchor="middle">${g.format(g.gr)}</text>
                <text x="${grX + barW/2}" y="${height - pad.bottom + 16}" fill="#34D399" font-size="10" font-weight="700" text-anchor="middle">Greedy</text>

                <!-- Group Label -->
                <text x="${centerX}" y="${height - 6}" fill="#94A3B8" font-size="12" font-weight="600" text-anchor="middle">${g.label}</text>
            `;
        }).join('');

        container.innerHTML = `
            <svg viewBox="0 0 ${width} ${height}" style="width: 100%; height: 100%;">
                <!-- Axis line -->
                <line x1="${pad.left}" y1="${height - pad.bottom}" x2="${width - pad.right}" y2="${height - pad.bottom}" stroke="rgba(255,255,255,0.15)" stroke-width="1"/>
                ${barsSvg}
            </svg>
        `;
    }

    // ── Recipe Catalog Rendering ──────────────────────────────────────────────
    function setupRecipeFilters() {
        const searchInput = document.getElementById('recipeSearchInput');
        const mealSelect = document.getElementById('filterMealType');
        const dietSelect = document.getElementById('filterDiet');
        const costSelect = document.getElementById('filterCost');

        const applyFilters = () => {
            currentPage = 1;
            renderRecipeCatalog();
        };

        if (searchInput) searchInput.addEventListener('input', applyFilters);
        if (mealSelect) mealSelect.addEventListener('change', applyFilters);
        if (dietSelect) dietSelect.addEventListener('change', applyFilters);
        if (costSelect) costSelect.addEventListener('change', applyFilters);

        document.getElementById('btnPrevPage').addEventListener('click', () => {
            if (currentPage > 1) {
                currentPage--;
                renderRecipeCatalog();
            }
        });
        document.getElementById('btnNextPage').addEventListener('click', () => {
            const filtered = getFilteredRecipes();
            if (currentPage * PAGE_SIZE < filtered.length) {
                currentPage++;
                renderRecipeCatalog();
            }
        });
    }

    function getFilteredRecipes() {
        const query = (document.getElementById('recipeSearchInput').value || '').toLowerCase().trim();
        const mealType = document.getElementById('filterMealType').value;
        const diet = document.getElementById('filterDiet').value;
        const cost = document.getElementById('filterCost').value;

        return recipeCatalog.filter(r => {
            // Text Search
            if (query && !r.name.toLowerCase().includes(query) && !r.id.toLowerCase().includes(query)) {
                // Check if any ingredient matches
                const hasIng = (r.ingredients || []).some(i => i.name.toLowerCase().includes(query));
                if (!hasIng) return false;
            }

            // Meal Type
            if (mealType === 'breakfast' && !r.is_breakfast) return false;
            if (mealType === 'lunch' && !r.is_lunch) return false;
            if (mealType === 'dinner' && !r.is_dinner) return false;
            if (mealType === 'beverage' && !r.is_beverage) return false;

            // Diet
            if (diet === 'veg' && !r.is_vegetarian) return false;
            if (diet === 'vegan' && !r.is_vegan) return false;
            if (diet === 'nonveg' && !r.is_non_veg) return false;

            // Cost
            if (cost === 'priced' && (r.cost_inr <= 0 && r.cost_coverage_pct <= 0)) return false;
            if (cost === 'complete' && !r.cost_complete) return false;

            return true;
        });
    }

    function renderRecipeCatalog() {
        const grid = document.getElementById('recipesGrid');
        const countLabel = document.getElementById('recipeCatalogCount');
        if (!grid) return;

        const filtered = getFilteredRecipes();
        const total = filtered.length;
        const start = (currentPage - 1) * PAGE_SIZE;
        const end = Math.min(start + PAGE_SIZE, total);
        const pageItems = filtered.slice(start, end);

        countLabel.textContent = `Showing ${total ? start + 1 : 0}–${end} of ${total} recipes matching criteria`;
        document.getElementById('pageIndicator').textContent = `Page ${currentPage} of ${Math.max(1, Math.ceil(total / PAGE_SIZE))}`;
        document.getElementById('btnPrevPage').disabled = (currentPage <= 1);
        document.getElementById('btnNextPage').disabled = (end >= total);

        grid.innerHTML = '';
        pageItems.forEach(r => {
            const card = document.createElement('div');
            card.className = 'recipe-catalog-card';
            card.innerHTML = `
                <div>
                    <div class="recipe-badges">
                        ${r.is_vegetarian ? '<span class="tag-badge tag-veg">Veg</span>' : ''}
                        ${r.is_vegan ? '<span class="tag-badge tag-vegan">Vegan</span>' : ''}
                        ${r.is_non_veg ? '<span class="tag-badge tag-nonveg">Non-Veg</span>' : ''}
                        ${r.is_breakfast ? '<span class="tag-badge tag-bf">Breakfast</span>' : ''}
                        ${r.is_lunch ? '<span class="tag-badge tag-lunch">Lunch</span>' : ''}
                        ${r.is_dinner ? '<span class="tag-badge tag-dinner">Dinner</span>' : ''}
                    </div>
                    <h4 class="recipe-name" style="margin-bottom: 0.35rem;">${r.name}</h4>
                    <span style="font-size: 0.75rem; color: var(--text-muted);">${r.id} &middot; per ${r.serving_unit || 'serving'}</span>
                </div>
                <div style="margin-top: 1rem; padding-top: 0.75rem; border-top: 1px solid var(--border-subtle); display: flex; justify-content: space-between; align-items: center;">
                    <div>
                        <div style="font-size: 0.82rem; color: var(--text-muted);">${r.kcal ? r.kcal.toFixed(0) : '0'} kcal &middot; ${r.protein_g ? r.protein_g.toFixed(1) : '0'}g protein</div>
                    </div>
                    <div style="text-align: right;">
                        <span style="font-size: 1rem; font-weight: 800; color: #34D399;">₹${(r.cost_inr || 0).toFixed(1)}</span>
                        <div style="font-size: 0.68rem; color: var(--text-muted);">${r.cost_complete ? 'Complete Est.' : 'Partial (' + (r.cost_coverage_pct||0).toFixed(0) + '%)'}</div>
                    </div>
                </div>
            `;
            card.addEventListener('click', () => openRecipeModal(r.id));
            grid.appendChild(card);
        });
    }

    // ── Grocery Shopping List Rendering ───────────────────────────────────────
    function renderGroceryList() {
        if (!appData) return;
        const list = (activeGroceryAlgo === 'dp') ? (appData.grocery_list_dp || []) : (appData.grocery_list_greedy || []);
        const container = document.getElementById('groceryListContainer');
        if (!container) return;
        container.innerHTML = '';

        if (!list.length) {
            container.innerHTML = '<div style="color:var(--text-muted); grid-column: 1/-1; text-align:center; padding:2rem;">No grocery items found in plan</div>';
            return;
        }

        // Sort items alphabetically
        const sorted = [...list].sort((a, b) => a.name.localeCompare(b.name));

        sorted.forEach(item => {
            const div = document.createElement('div');
            div.className = 'grocery-item';
            div.innerHTML = `
                <label class="grocery-checkbox-label">
                    <input type="checkbox" class="grocery-checkbox">
                    <span>${item.name}</span>
                </label>
                <div style="display: flex; align-items: center; gap: 0.5rem;">
                    <span class="grocery-qty">${item.amount.toFixed(1)} ${item.unit || ''}</span>
                    ${item.all_priced ? '<span title="Priced via retail benchmark" style="color:#10B981; font-size:0.75rem;">✔</span>' : '<span title="Unpriced in official benchmark" style="color:#F59E0B; font-size:0.75rem;">⚠</span>'}
                </div>
            `;

            const chk = div.querySelector('.grocery-checkbox');
            chk.addEventListener('change', () => {
                div.classList.toggle('checked', chk.checked);
            });

            container.appendChild(div);
        });
    }

    // ── Recipe Detail Modal ───────────────────────────────────────────────────
    function setupModal() {
        modalCloseBtn.addEventListener('click', () => recipeModal.classList.remove('active'));
        recipeModal.addEventListener('click', (e) => {
            if (e.target === recipeModal) recipeModal.classList.remove('active');
        });
    }

    function openRecipeModal(recipeId) {
        const r = recipeMap.get(recipeId);
        if (!r) return;

        modalBody.innerHTML = `
            <div style="margin-bottom: 1.25rem;">
                <div class="recipe-badges" style="margin-bottom: 0.4rem;">
                    ${r.is_vegetarian ? '<span class="tag-badge tag-veg">Vegetarian</span>' : ''}
                    ${r.is_vegan ? '<span class="tag-badge tag-vegan">Vegan</span>' : ''}
                    ${r.is_non_veg ? '<span class="tag-badge tag-nonveg">Non-Vegetarian</span>' : ''}
                    ${r.cost_complete ? '<span class="metric-badge badge-success">100% Price Coverage</span>' : '<span class="metric-badge badge-warning">Partial Cost Coverage (' + (r.cost_coverage_pct||0).toFixed(0) + '%)</span>'}
                </div>
                <h2 style="font-size: 1.45rem; font-weight: 800; color: #FFF;">${r.name}</h2>
                <div style="font-size: 0.85rem; color: var(--text-muted);">${r.id} &middot; Serving Unit: <strong>${r.serving_unit || '1 serving'}</strong></div>
            </div>

            <!-- Nutritional Grid -->
            <div style="display: grid; grid-template-columns: repeat(4, 1fr); gap: 0.75rem; margin-bottom: 1.5rem;">
                <div style="background: rgba(255,255,255,0.03); padding: 0.75rem; border-radius: var(--radius-sm); text-align: center; border: 1px solid var(--border-subtle);">
                    <div style="font-size: 0.72rem; color: var(--text-muted); text-transform: uppercase;">Energy</div>
                    <div style="font-size: 1.15rem; font-weight: 800; color: #FFF;">${(r.kcal || 0).toFixed(0)}</div>
                    <div style="font-size: 0.7rem; color: var(--text-muted);">kcal</div>
                </div>
                <div style="background: rgba(255,255,255,0.03); padding: 0.75rem; border-radius: var(--radius-sm); text-align: center; border: 1px solid var(--border-subtle);">
                    <div style="font-size: 0.72rem; color: var(--text-muted); text-transform: uppercase;">Protein</div>
                    <div style="font-size: 1.15rem; font-weight: 800; color: #EC4899;">${(r.protein_g || 0).toFixed(1)}</div>
                    <div style="font-size: 0.7rem; color: var(--text-muted);">grams</div>
                </div>
                <div style="background: rgba(255,255,255,0.03); padding: 0.75rem; border-radius: var(--radius-sm); text-align: center; border: 1px solid var(--border-subtle);">
                    <div style="font-size: 0.72rem; color: var(--text-muted); text-transform: uppercase;">Carbs</div>
                    <div style="font-size: 1.15rem; font-weight: 800; color: #38BDF8;">${(r.carb_g || 0).toFixed(1)}</div>
                    <div style="font-size: 0.7rem; color: var(--text-muted);">grams</div>
                </div>
                <div style="background: rgba(255,255,255,0.03); padding: 0.75rem; border-radius: var(--radius-sm); text-align: center; border: 1px solid var(--border-subtle);">
                    <div style="font-size: 0.72rem; color: var(--text-muted); text-transform: uppercase;">Fat</div>
                    <div style="font-size: 1.15rem; font-weight: 800; color: #FBBF24;">${(r.fat_g || 0).toFixed(1)}</div>
                    <div style="font-size: 0.7rem; color: var(--text-muted);">grams</div>
                </div>
            </div>

            <!-- Ingredients Table -->
            <h4 style="font-size: 1rem; font-weight: 700; color: #FFF; margin-bottom: 0.5rem;">Ingredient Cost & Quantity Breakdown</h4>
            <div style="overflow-x: auto; max-height: 260px;">
                <table class="ing-table">
                    <thead>
                        <tr>
                            <th>Ingredient</th>
                            <th>Quantity</th>
                            <th>Commodity Benchmark</th>
                            <th>Line Cost</th>
                        </tr>
                    </thead>
                    <tbody>
                        ${(r.ingredients || []).map(ing => `
                            <tr>
                                <td><strong>${ing.name}</strong></td>
                                <td>${ing.amount.toFixed(1)} ${ing.unit || ''}</td>
                                <td>${ing.commodity ? `<span style="color:#A5B4FC;">${ing.commodity}</span>` : '<span style="color:var(--text-muted); font-size:0.75rem;">No official match</span>'}</td>
                                <td>${ing.priced ? `<span style="color:#34D399; font-weight:700;">₹${ing.cost_line.toFixed(2)}</span>` : '<span style="color:#F59E0B; font-size:0.75rem;">Unpriced</span>'}</td>
                            </tr>
                        `).join('')}
                    </tbody>
                </table>
            </div>

            <div style="margin-top: 1.25rem; text-align: right; font-size: 1.1rem;">
                Estimated Cost per Serving: <strong style="color: #34D399;">₹${(r.cost_inr || 0).toFixed(2)}</strong>
            </div>
        `;

        recipeModal.classList.add('active');
    }

    // ── Parameter Form & C++ Runner ───────────────────────────────────────────
    function setupParameterForm() {
        const inBudget = document.getElementById('inputBudget');
        const inKcal = document.getElementById('inputKcal');
        const inProt = document.getElementById('inputProtein');
        const inDiet = document.getElementById('inputDiet');
        const cliBox = document.getElementById('cliCommandBox');

        const updateCmd = () => {
            const b = inBudget.value || 700;
            const k = inKcal.value || 2000;
            const p = inProt.value || 50;
            const d = inDiet.value || 'any';
            cliBox.textContent = `.\\budgetbite.exe --budget ${b} --kcal ${k} --protein ${p} --diet ${d} --out "web/meal_plan.json"`;
        };

        [inBudget, inKcal, inProt, inDiet].forEach(el => el.addEventListener('input', updateCmd));

        document.getElementById('btnCopyCli').addEventListener('click', () => {
            navigator.clipboard.writeText(cliBox.textContent.trim());
            alert('Command copied to clipboard!\nRun it in PowerShell or Command Prompt in the project folder.');
        });

        document.getElementById('btnDownloadBat').addEventListener('click', () => {
            const content = `@echo off\necho Running BudgetBite Optimizer...\n${cliBox.textContent.trim()}\necho Done!\npause\n`;
            const blob = new Blob([content], { type: 'text/plain' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = 'run_budgetbite.bat';
            a.click();
            URL.revokeObjectURL(url);
        });

        document.getElementById('jsonFileInput').addEventListener('change', (e) => {
            const file = e.target.files[0];
            if (file) {
                const reader = new FileReader();
                reader.onload = (event) => {
                    try {
                        const parsed = JSON.parse(event.target.result);
                        handleDataLoaded(parsed);
                        alert('Custom meal_plan.json loaded successfully!');
                    } catch (ex) {
                        alert('Error parsing JSON: ' + ex.message);
                    }
                };
                reader.readAsText(file);
            }
        });
    }

    // ── Exports (JSON, Text Grocery) ──────────────────────────────────────────
    function setupExports() {
        // Export Plan JSON
        document.getElementById('btnExportJson').addEventListener('click', () => {
            if (!appData) return;
            const blob = new Blob([JSON.stringify(appData, null, 2)], { type: 'application/json' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = `budgetbite_${activeAlgo}_plan.json`;
            a.click();
            URL.revokeObjectURL(url);
        });

        // Export Grocery Text
        document.getElementById('btnExportGroceryTxt').addEventListener('click', () => {
            if (!appData) return;
            const list = (activeGroceryAlgo === 'dp') ? (appData.grocery_list_dp || []) : (appData.grocery_list_greedy || []);
            let txt = `BudgetBite Grocery Shopping List (${activeGroceryAlgo.toUpperCase()} Plan)\n`;
            txt += `Generated: ${new Date().toLocaleDateString()}\n`;
            txt += `====================================================\n\n`;
            list.forEach((item, idx) => {
                txt += `[ ] ${idx + 1}. ${item.name} - ${item.amount.toFixed(1)} ${item.unit || ''} (${item.all_priced ? 'Priced' : 'Unpriced'})\n`;
            });
            const blob = new Blob([txt], { type: 'text/plain' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = `budgetbite_grocery_${activeGroceryAlgo}.txt`;
            a.click();
            URL.revokeObjectURL(url);
        });
    }

    // Start on DOM ready
    window.addEventListener('DOMContentLoaded', init);

})();
