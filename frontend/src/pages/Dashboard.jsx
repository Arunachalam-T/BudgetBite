import { useNavigate } from 'react-router-dom'
import { useApp } from '../AppContext'
import { Database, UtensilsCrossed, BarChart3, CheckCircle, XCircle, AlertCircle } from 'lucide-react'

const DAY_NAMES = ['Mon','Tue','Wed','Thu','Fri','Sat','Sun']

export default function Dashboard() {
  const { dataset, validation, config, results } = useApp()
  const nav = useNavigate()

  const currentPlan = results?.plan || results?.nutrition || Object.values(results || {})[0]

  return (
    <div>
      <h2 className="section-title">Dashboard</h2>
      <p className="section-sub">Overview of your SmartMeal Optimizer session</p>

      {/* Dataset status */}
      {!dataset ? (
        <div className="card" style={{ marginBottom: 20 }}>
          <div className="empty-state">
            <Database size={40} style={{ color: 'var(--color-text-muted)' }} />
            <h3>No dataset loaded</h3>
            <p>Upload an Excel workbook or CSV files to get started.</p>
            <button className="btn btn-primary" style={{ marginTop: 16 }} onClick={() => nav('/dataset')}>
              Go to Dataset
            </button>
          </div>
        </div>
      ) : (
        <>
          {/* Stats row */}
          <div className="stats-row">
            <div className="stat-card">
              <div className="stat-label">Ingredients</div>
              <div className="stat-value" style={{ color: 'var(--color-primary)' }}>{dataset.ingredients_count ?? 0}</div>
              <div className="stat-sub">loaded</div>
            </div>
            <div className="stat-card">
              <div className="stat-label">Recipes</div>
              <div className="stat-value" style={{ color: 'var(--color-green)' }}>{dataset.recipes_count ?? 0}</div>
              <div className="stat-sub">loaded</div>
            </div>
            <div className="stat-card">
              <div className="stat-label">Weekly Budget</div>
              <div className="stat-value" style={{ color: 'var(--color-purple)' }}>₹{config.weekly_budget_inr}</div>
              <div className="stat-sub">configured</div>
            </div>
            <div className="stat-card">
              <div className="stat-label">Diet</div>
              <div className="stat-value" style={{ fontSize: 18 }}>{config.diet_type}</div>
              <div className="stat-sub">{config.meals_per_day} meals/day</div>
            </div>
            <div className="stat-card">
              <div className="stat-label">Validation</div>
              <div className="stat-value" style={{ fontSize: 18 }}>
                {validation?.is_valid ? (
                  <span style={{ color: 'var(--color-green)' }}>✓ Valid</span>
                ) : (
                  <span style={{ color: 'var(--color-red)' }}>✗ Errors</span>
                )}
              </div>
              <div className="stat-sub">{validation?.messages?.length ?? 0} messages</div>
            </div>
          </div>

          {/* Validation errors */}
          {validation && !validation.is_valid && (
            <div className="alert alert-error" style={{ marginBottom: 20 }}>
              <XCircle size={16} />
              <div>
                <strong>Dataset has validation errors.</strong>
                <ul style={{ marginTop: 6, paddingLeft: 16 }}>
                  {validation.messages.filter(m => m.severity === 'FATAL').slice(0, 5).map((m, i) => (
                    <li key={i}>{m.worksheet}{m.row > 0 ? ` row ${m.row}` : ''}: {m.message}</li>
                  ))}
                </ul>
              </div>
            </div>
          )}

          {/* Plan summary */}
          <div className="grid-2" style={{ marginBottom: 20 }}>
            {[
              { mode: 'nutrition', label: 'Nutrition Plan', color: 'var(--color-green)', res: results?.nutrition },
              { mode: 'budget',    label: 'Budget Plan',    color: 'var(--color-primary)', res: results?.budget },
            ].map(({ mode, label, color, res }) => (
              <div key={mode} className="card">
                <div className="card-title" style={{ color, display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                  <span>{label}</span>
                  {res?.is_feasible && (
                    <span className="badge badge-green" style={{ fontSize: 10 }}>
                      <CheckCircle size={11} /> Ready
                    </span>
                  )}
                </div>
                {res?.is_feasible ? (
                  <div style={{ fontSize: 13, lineHeight: 1.8, marginTop: 6 }}>
                    <div>💰 Purchase: <strong>₹{Math.round(res.evaluation?.weekly_purchase_cost || 0)}</strong></div>
                    <div>🎯 Deviation: <strong>{((res.evaluation?.nutrition_deviation_score || 0) * 100).toFixed(1)}%</strong></div>
                    <div>🍽️ Recipes: <strong>{res.evaluation?.distinct_recipe_count || 0} distinct</strong></div>
                    <div style={{ marginTop: 8 }}>
                      <button className="btn btn-outline btn-sm" onClick={() => nav('/meals')}>
                        View {label} →
                      </button>
                    </div>
                  </div>
                ) : (
                  <div style={{ color: 'var(--color-text-muted)', fontSize: 13, padding: '6px 0' }}>
                    No plan generated yet.
                    <div style={{ marginTop: 8 }}>
                      <button className="btn btn-primary btn-sm" onClick={() => nav('/meals')}>
                        <UtensilsCrossed size={13} /> Generate Plan
                      </button>
                    </div>
                  </div>
                )}
              </div>
            ))}
          </div>

          {/* Quick actions */}
          <div className="grid-2">
            <div className="card">
              <div className="card-title">Quick Actions</div>
              <div style={{ display: 'flex', flexDirection: 'column', gap: 10 }}>
                <button className="btn btn-primary" onClick={() => nav('/meals')}>
                  <UtensilsCrossed size={15} /> Generate Meal Plans
                </button>
                <button className="btn btn-outline" onClick={() => nav('/algorithms')}>
                  <BarChart3 size={15} /> Compare Algorithms
                </button>
                <button className="btn btn-outline" onClick={() => nav('/settings')}>
                  Configure Constraints
                </button>
              </div>
            </div>
            <div className="card">
              <div className="card-title">Configuration Summary</div>
              <table style={{ fontSize: 13 }}>
                <tbody>
                  {[
                    ['Meals/day', config.meals_per_day],
                    ['Max repetitions', config.max_recipe_repetitions],
                    ['Min calories', config.min_calories + ' kcal'],
                    ['Max calories', config.max_calories + ' kcal'],
                    ['Min protein', config.min_protein + ' g'],
                    ['Max fat', config.max_fat + ' g'],
                  ].map(([k,v]) => (
                    <tr key={k}>
                      <td style={{ color: 'var(--color-text-muted)', fontWeight: 600, width: '55%' }}>{k}</td>
                      <td>{v}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </div>
        </>
      )}
    </div>
  )
}

