import { useState, useEffect } from 'react'
import { useApp } from '../AppContext'
import { Play, Download, AlertCircle, CheckCircle } from 'lucide-react'
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

function PlanDisplay({ result, config, recipesMap }) {
  if (!result) return (
    <div className="empty-state">
      <AlertCircle size={36} style={{ color: 'var(--color-text-muted)' }} />
      <h3>No plan generated yet</h3>
      <p>Select an algorithm above and click "Generate Plan" to optimize your 7-day meal plan.</p>
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

  return (
    <div>
      {/* Stats summary row */}
      <div className="stats-row" style={{ marginBottom: 20 }}>
        <div className="stat-card">
          <div className="stat-label">Purchase Cost</div>
          <div className="stat-value" style={{ fontSize: 20, color: 'var(--color-primary)' }}>
            ₹{Math.round(purchaseCost)}
          </div>
          <div className="stat-sub">of ₹{config?.weekly_budget_inr ?? 2000} budget</div>
        </div>

        <div className="stat-card">
          <div className="stat-label">Nutrition Deviation</div>
          <div className="stat-value" style={{ fontSize: 20, color: 'var(--color-green)' }}>
            {devScore.toFixed(1)}%
          </div>
          <div className="stat-sub">average daily deviation</div>
        </div>

        <div className="stat-card">
          <div className="stat-label">Distinct Recipes</div>
          <div className="stat-value" style={{ fontSize: 20, color: 'var(--color-purple)' }}>
            {distinctCount}
          </div>
          <div className="stat-sub">unique recipes across 7 days</div>
        </div>

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

      {/* 7-Day Meal Plan grid */}
      <div className="card" style={{ marginBottom: 20 }}>
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 12 }}>
          <div className="card-title" style={{ margin: 0 }}>
            7-Day Optimized Meal Plan
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

      {/* Visual Analytics */}
      <ErrorBoundary>
        <div style={{ display: 'flex', flexDirection: 'column', gap: 20 }}>
          <NutritionChart eval_={eval_} config={config} />
          <BudgetChart eval_={eval_} config={config} />
        </div>
      </ErrorBoundary>
    </div>
  )
}

export default function MealsPage() {
  const {
    dataset, config, results,
    generatePlan, exportExcel, loading, error
  } = useApp()

  const [algorithm, setAlgorithm] = useState('Backtracking')
  const [recipesMap, setRecipesMap] = useState({})

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
  const currentPlan = results?.plan || results?.nutrition || Object.values(results || {})[0]

  const isLoading = loading?.plan
  const planError = error?.plan

  return (
    <div>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', marginBottom: 20 }}>
        <div>
          <h2 className="section-title">Meal Plans</h2>
          <p className="section-sub">Generate and view your optimized 7-day meal plan</p>
        </div>
        <div style={{ display: 'flex', gap: 10, alignItems: 'center' }}>
          <select
            className="form-input"
            style={{ width: 180 }}
            value={algorithm}
            onChange={e => setAlgorithm(e.target.value)}
          >
            {algorithms.map(a => <option key={a}>{a}</option>)}
          </select>

          <button
            className="btn btn-primary"
            disabled={!canGenerate || isLoading}
            onClick={() => generatePlan(algorithm)}
          >
            {isLoading ? (
              <><span className="spinner" style={{ width: 14, height: 14 }} /> Optimizing Plan…</>
            ) : (
              <><Play size={14} /> Generate Plan</>
            )}
          </button>

          {currentPlan?.is_feasible && (
            <button className="btn btn-green btn-sm" onClick={() => exportExcel()}>
              <Download size={14} /> Export Plan
            </button>
          )}
        </div>
      </div>

      {!canGenerate && (
        <div className="alert alert-warn" style={{ marginBottom: 20 }}>
          <AlertCircle size={16} />
          Upload and validate a dataset before generating meal plans.
        </div>
      )}

      {planError && (
        <div className="alert alert-error" style={{ marginBottom: 16 }}>
          <AlertCircle size={16} /> {planError}
        </div>
      )}

      {/* Main plan & chart display */}
      <ErrorBoundary>
        <PlanDisplay
          result={currentPlan}
          config={config}
          recipesMap={recipesMap}
        />
      </ErrorBoundary>
    </div>
  )
}
