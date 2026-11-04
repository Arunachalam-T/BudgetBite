import { useState, useEffect } from 'react'
import { useApp } from '../AppContext'
import { Play, Download, AlertCircle, CheckCircle, Sparkles } from 'lucide-react'
import NutritionChart from '../charts/NutritionChart'
import BudgetChart    from '../charts/BudgetChart'
import ErrorBoundary  from '../components/ErrorBoundary'
import api            from '../services/api'

const DAY_NAMES = ['Monday','Tuesday','Wednesday','Thursday','Friday','Saturday','Sunday']

function MealDayCard({ day, assignments = [], calcRecipes }) {
  return (
    <div className="day-card">
      <div className="day-header">{DAY_NAMES[day]}</div>
      {assignments.map((a, i) => {
        const cr = calcRecipes?.[a.recipe_id]
        return (
          <div key={i} className="meal-slot">
            <div className="meal-slot-type">{a.meal_type}</div>
            <div className="meal-slot-name">{cr?.recipe_name || a.recipe_id}</div>
            {cr && cr.calories_per_serving !== undefined && (
              <div className="meal-slot-meta">
                {Math.round(cr.calories_per_serving)} kcal · ₹{cr.gross_cost_per_serving?.toFixed(1)}
              </div>
            )}
          </div>
        )
      })}
    </div>
  )
}

function ModeComparisonBanner({ results, activeMode, onSelectMode }) {
  const modes = [
    { key: 'nutrition', label: 'Nutrition Mode', color: 'var(--color-green)', target: 'Min Nutrition Deviation' },
    { key: 'budget',    label: 'Budget Mode',    color: 'var(--color-primary)', target: 'Min Purchase Cost' },
  ]

  const hasAny = modes.some(m => results?.[m.key]?.is_feasible)
  if (!hasAny) return null

  return (
    <div className="card" style={{ marginBottom: 20, background: 'var(--color-surface)' }}>
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: 10 }}>
        <div style={{ fontSize: 13, fontWeight: 600, color: 'var(--color-text)' }}>
          Mode Comparison — Click to switch view:
        </div>
        <span style={{ fontSize: 11, color: 'var(--color-text-muted)' }}>
          Values differ based on each mode's optimization objective
        </span>
      </div>
      <div className="grid-2" style={{ gap: 12 }}>
        {modes.map(({ key, label, color }) => {
          const res = results?.[key]
          const isCurrent = activeMode.toLowerCase() === key
          const eval_ = res?.evaluation

          return (
            <div
              key={key}
              onClick={() => onSelectMode(label.split(' ')[0])}
              style={{
                cursor: 'pointer',
                padding: '12px 14px',
                borderRadius: 8,
                background: isCurrent ? 'var(--color-surface-2)' : 'var(--color-surface)',
                border: isCurrent ? `2px solid ${color}` : '1px solid var(--color-border)',
                transition: 'all 0.2s',
                boxShadow: isCurrent ? '0 2px 8px rgba(0,0,0,0.06)' : 'none'
              }}
            >
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 6 }}>
                <strong style={{ fontSize: 13, color }}>{label}</strong>
                {isCurrent && <span className="badge badge-blue" style={{ fontSize: 10 }}>Active</span>}
              </div>
              {eval_ ? (
                <div style={{ fontSize: 12, lineHeight: 1.6, color: 'var(--color-text)' }}>
                  <div>💰 Purchase Cost: <strong>₹{Math.round(eval_.weekly_purchase_cost || 0)}</strong></div>
                  <div>🎯 Nutrition Deviation: <strong>{((eval_.nutrition_deviation_score || 0) * 100).toFixed(1)}%</strong></div>
                  <div>🍽️ Distinct Recipes: <strong>{eval_.distinct_recipe_count || 0}</strong></div>
                </div>
              ) : (
                <div style={{ fontSize: 12, color: 'var(--color-text-muted)', fontStyle: 'italic', marginTop: 4 }}>
                  Not generated yet (click to generate)
                </div>
              )}
            </div>
          )
        })}
      </div>
    </div>
  )
}

function PlanDisplay({ result, mode, config, recipesMap }) {
  if (!result) return (
    <div className="empty-state">
      <AlertCircle size={36} style={{ color: 'var(--color-text-muted)' }} />
      <h3>No plan generated for {mode} mode</h3>
      <p>Click "Generate Plan" above or click the mode card to create an optimized plan.</p>
    </div>
  )

  const eval_ = result.evaluation || {}

  // Map recipe IDs to recipe details
  const crMap = { ...recipesMap }
  if (result.plan) {
    result.plan.forEach(a => {
      if (!crMap[a.recipe_id]) crMap[a.recipe_id] = { recipe_name: a.recipe_id }
    })
  }

  // Group assignments by day (0..6)
  const byDay = {}
  for (let d = 0; d < 7; ++d) byDay[d] = []
  ;(result.plan || []).forEach(a => {
    if (byDay[a.day] !== undefined) byDay[a.day].push(a)
  })

  if (!result.is_feasible) {
    return (
      <div className="alert alert-error">
        <AlertCircle size={16} />
        <div>
          <strong>No feasible plan found: {result.status}</strong>
          <p style={{ marginTop: 4 }}>{result.termination_reason}</p>
          {eval_.violations?.length > 0 && (
            <ul style={{ paddingLeft: 16, marginTop: 6 }}>
              {eval_.violations.slice(0, 5).map((v, i) => <li key={i}>{v}</li>)}
            </ul>
          )}
        </div>
      </div>
    )
  }

  const purchaseCost = eval_.weekly_purchase_cost ?? 0
  const devScore = (eval_.nutrition_deviation_score ?? 0) * 100
  const distinctCount = eval_.distinct_recipe_count ?? 0

  const isNutritionMode = mode === 'Nutrition'
  const isBudgetMode    = mode === 'Budget'

  return (
    <div>
      {/* Stats row with Primary Objective highlights */}
      <div className="stats-row" style={{ marginBottom: 20 }}>
        {/* Cost Card */}
        <div className="stat-card" style={isBudgetMode ? { border: '2px solid var(--color-primary)', background: 'rgba(37,99,235,0.03)' } : {}}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <div className="stat-label">Purchase Cost</div>
            {isBudgetMode && <span className="badge badge-blue" style={{ fontSize: 9 }}>★ Primary Target</span>}
          </div>
          <div className="stat-value" style={{ fontSize: 20, color: isBudgetMode ? 'var(--color-primary)' : 'inherit' }}>
            ₹{Math.round(purchaseCost)}
          </div>
          <div className="stat-sub">of ₹{config?.weekly_budget_inr ?? 2000} budget</div>
        </div>

        {/* Nutrition Card */}
        <div className="stat-card" style={isNutritionMode ? { border: '2px solid var(--color-green)', background: 'rgba(22,163,74,0.03)' } : {}}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <div className="stat-label">Nutrition Deviation</div>
            {isNutritionMode && <span className="badge badge-green" style={{ fontSize: 9 }}>★ Primary Target</span>}
          </div>
          <div className="stat-value" style={{ fontSize: 20, color: isNutritionMode ? 'var(--color-green)' : 'inherit' }}>
            {devScore.toFixed(1)}%
          </div>
          <div className="stat-sub">avg daily deviation</div>
        </div>

        {/* Distinct Recipes Card */}
        <div className="stat-card">
          <div className="stat-label">Distinct Recipes</div>
          <div className="stat-value" style={{ fontSize: 20 }}>
            {distinctCount}
          </div>
          <div className="stat-sub">recipes used across 7 days</div>
        </div>

        {/* Status Card */}
        <div className="stat-card">
          <div className="stat-label">Algorithm Status</div>
          <div className="stat-value" style={{ fontSize: 14 }}>
            <span className={`badge ${result.optimality_proven ? 'badge-green' : 'badge-yellow'}`}>
              {result.optimality_proven ? '✓ Optimal' : 'Feasible'}
            </span>
          </div>
          <div className="stat-sub">{result.algorithm_name} ({result.execution_time_ms?.toFixed(1) ?? 0} ms)</div>
        </div>
      </div>

      {/* Day grid */}
      <div className="card" style={{ marginBottom: 20 }}>
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 12 }}>
          <div className="card-title" style={{ margin: 0 }}>
            7-Day Meal Plan ({mode} Optimized)
          </div>
          <span style={{ fontSize: 12, color: 'var(--color-text-muted)' }}>
            Showing 21 meal slots
          </span>
        </div>
        <div className="day-grid">
          {Array.from({ length: 7 }, (_, d) => (
            <MealDayCard key={d} day={d} assignments={byDay[d]} calcRecipes={crMap} />
          ))}
        </div>
      </div>

      {/* Mode-specific charts */}
      <ErrorBoundary>
        {mode === 'Nutrition' && <NutritionChart eval_={eval_} config={config} />}
        {mode === 'Budget'    && <BudgetChart    eval_={eval_} config={config} />}
      </ErrorBoundary>
    </div>
  )
}

export default function MealsPage() {
  const {
    dataset, config, results,
    activeMode, setActiveMode,
    generatePlan, generateBothPlans,
    exportExcel, loading, error
  } = useApp()

  const [algorithm, setAlgorithm] = useState('Backtracking')
  const [recipesMap, setRecipesMap] = useState({})

  // Only Nutrition and Budget modes (No Variety)
  const modes = ['Nutrition', 'Budget']
  const algorithms = ['Brute Force','Greedy','Dynamic Programming','Backtracking','Branch and Bound']

  useEffect(() => {
    if (dataset?.is_loaded || dataset?.is_valid) {
      api.recipes()
        .then(list => {
          if (Array.isArray(list)) {
            const map = {}
            list.forEach(r => { map[r.recipe_id] = r })
            setRecipesMap(map)
          }
        })
        .catch(() => {})
    }
  }, [dataset])

  const canGenerate = dataset?.is_valid
  const currentModeResult = results?.[activeMode?.toLowerCase()]

  const isLoadingCurrent = loading?.[`plan_${activeMode}`]
  const isLoadingAll     = loading?.plan_all
  const planError        = error?.[`plan_${activeMode}`] || error?.plan_all

  // Switch mode and auto-generate if not yet computed
  const handleSelectMode = (newMode) => {
    setActiveMode(newMode)
    const key = newMode.toLowerCase()
    if (!results?.[key] && canGenerate && !isLoadingCurrent && !isLoadingAll) {
      generatePlan(newMode, algorithm)
    }
  }

  return (
    <div>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', marginBottom: 20 }}>
        <div>
          <h2 className="section-title">Meal Plans</h2>
          <p className="section-sub">Generate and compare Nutrition and Budget optimized plans</p>
        </div>
        <div style={{ display: 'flex', gap: 10, alignItems: 'center' }}>
          <select
            className="form-input"
            style={{ width: 170 }}
            value={algorithm}
            onChange={e => setAlgorithm(e.target.value)}
          >
            {algorithms.map(a => <option key={a}>{a}</option>)}
          </select>

          {/* Generate single plan button */}
          <button
            className="btn btn-primary"
            disabled={!canGenerate || isLoadingCurrent || isLoadingAll}
            onClick={() => generatePlan(activeMode, algorithm)}
          >
            {isLoadingCurrent ? (
              <><span className="spinner" style={{ width: 14, height: 14 }} /> Optimizing {activeMode}…</>
            ) : (
              <><Play size={14} /> Generate {activeMode} Plan</>
            )}
          </button>

          {/* Generate both modes button */}
          <button
            className="btn btn-outline"
            disabled={!canGenerate || isLoadingCurrent || isLoadingAll}
            onClick={() => generateBothPlans(algorithm)}
            title="Optimizes Nutrition and Budget back-to-back so you can compare both instantly"
          >
            {isLoadingAll ? (
              <><span className="spinner" style={{ width: 14, height: 14 }} /> Solving Both Modes…</>
            ) : (
              <><Sparkles size={14} /> Generate Both Modes</>
            )}
          </button>

          {currentModeResult?.is_feasible && (
            <button className="btn btn-green btn-sm" onClick={() => exportExcel(activeMode)}>
              <Download size={14} /> Export Plan
            </button>
          )}
        </div>
      </div>

      {!canGenerate && (
        <div className="alert alert-warn" style={{ marginBottom: 20 }}>
          <AlertCircle size={16} />
          Upload and validate a dataset before generating plans.
        </div>
      )}

      {planError && (
        <div className="alert alert-error" style={{ marginBottom: 16 }}>
          <AlertCircle size={16} /> {planError}
        </div>
      )}

      {/* Mode tabs: Nutrition and Budget only */}
      <div className="tabs">
        {modes.map(m => (
          <button
            key={m}
            className={`tab-btn ${activeMode === m ? 'active' : ''}`}
            onClick={() => handleSelectMode(m)}
          >
            {m} Mode
          </button>
        ))}
      </div>

      {/* Side-by-side mode comparison strip */}
      <ModeComparisonBanner
        results={results}
        activeMode={activeMode}
        onSelectMode={handleSelectMode}
      />

      {/* Main plan & chart display */}
      <ErrorBoundary>
        <PlanDisplay
          result={currentModeResult}
          mode={activeMode}
          config={config}
          recipesMap={recipesMap}
        />
      </ErrorBoundary>
    </div>
  )
}
