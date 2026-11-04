import { useState } from 'react'
import { useApp } from '../AppContext'
import { Save, AlertCircle, CheckCircle } from 'lucide-react'
import api from '../services/api'

function NumField({ label, name, value, onChange, hint, min, step = 1 }) {
  return (
    <div className="form-group">
      <label className="form-label">{label}</label>
      <input
        className="form-input"
        type="number"
        name={name}
        value={value}
        onChange={onChange}
        min={min}
        step={step}
      />
      {hint && <div className="form-hint">{hint}</div>}
    </div>
  )
}

export default function SettingsPage() {
  const { config, setConfig, dataset } = useApp()
  const [localCfg, setLocalCfg] = useState({ ...config })
  const [saved, setSaved] = useState(false)
  const [validationResult, setValidationResult] = useState(null)
  const [validating, setValidating] = useState(false)

  const handleChange = (e) => {
    const { name, value, type } = e.target
    const intFields = ['meals_per_day', 'max_recipe_repetitions']
    const floatFields = [
      'weekly_budget_inr', 'min_calories', 'max_calories',
      'min_protein', 'min_carbs', 'max_fat',
      'target_calories', 'target_protein', 'target_carbs', 'target_fat'
    ]
    let parsedValue = value
    if (intFields.includes(name)) {
      parsedValue = parseInt(value, 10) || 0
    } else if (type === 'number' || floatFields.includes(name)) {
      parsedValue = parseFloat(value) || 0
    }
    setLocalCfg(c => ({
      ...c,
      [name]: parsedValue,
    }))
    setSaved(false)
    setValidationResult(null)
  }

  const handleValidate = async () => {
    setValidating(true)
    try {
      const res = await api.validateConfig(localCfg)
      setValidationResult(res)
    } catch (e) {
      setValidationResult({ is_valid: false, messages: [{ severity: 'FATAL', worksheet: 'Config', row: -1, message: e.message }] })
    } finally {
      setValidating(false)
    }
  }

  const handleSave = async () => {
    await handleValidate()
    setConfig(localCfg)
    setSaved(true)
  }

  return (
    <div>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', marginBottom: 20 }}>
        <div>
          <h2 className="section-title">Settings</h2>
          <p className="section-sub">Configure optimization constraints and targets</p>
        </div>
        <div style={{ display: 'flex', gap: 10 }}>
          <button className="btn btn-outline" onClick={handleValidate} disabled={validating}>
            {validating ? 'Validating…' : 'Validate'}
          </button>
          <button className="btn btn-primary" onClick={handleSave}>
            <Save size={14} /> Save Configuration
          </button>
        </div>
      </div>

      {saved && (
        <div className="alert alert-success" style={{ marginBottom: 16 }}>
          <CheckCircle size={16} /> Configuration saved.
        </div>
      )}

      {validationResult && (
        <div className={`alert ${validationResult.is_valid ? 'alert-success' : 'alert-error'}`} style={{ marginBottom: 16 }}>
          {validationResult.is_valid
            ? <><CheckCircle size={16} /> Configuration is valid.</>
            : <><AlertCircle size={16} /> <div>
                <strong>Configuration errors:</strong>
                <ul style={{ paddingLeft: 16, marginTop: 4 }}>
                  {validationResult.messages?.filter(m=>m.severity==='FATAL').map((m,i) => <li key={i}>{m.message}</li>)}
                </ul>
              </div></>
          }
        </div>
      )}

      <div className="grid-2" style={{ gap: 24 }}>
        {/* Weekly settings */}
        <div className="card">
          <div className="card-title">Weekly Settings</div>
          <NumField label="Weekly Budget (₹)" name="weekly_budget_inr" value={localCfg.weekly_budget_inr} onChange={handleChange} min={0} step={100} hint="Maximum grocery purchase cost per week" />
          <div className="form-group">
            <label className="form-label">Meals per Day</label>
            <select className="form-input" name="meals_per_day" value={localCfg.meals_per_day} onChange={handleChange}>
              <option value={2}>2 — Breakfast &amp; Dinner</option>
              <option value={3}>3 — Breakfast, Lunch &amp; Dinner</option>
              <option value={4}>4 — Breakfast, Lunch, Snack &amp; Dinner</option>
            </select>
          </div>
          <NumField label="Max Recipe Repetitions per Week" name="max_recipe_repetitions" value={localCfg.max_recipe_repetitions} onChange={handleChange} min={1} hint="Maximum times the same recipe can appear across the week" />
        </div>

        {/* Diet settings */}
        <div className="card">
          <div className="card-title">Dietary Settings</div>
          <div className="form-group">
            <label className="form-label">Diet Type</label>
            <select className="form-input" name="diet_type" value={localCfg.diet_type} onChange={handleChange}>
              <option value="Unrestricted">Unrestricted (all foods)</option>
              <option value="Non-vegetarian">Non-vegetarian</option>
              <option value="Vegetarian">Vegetarian (plant-based, dairy, eggs)</option>
              <option value="Vegan">Vegan (plant-based only)</option>
            </select>
            <div className="form-hint">
              Filters are applied using the ingredient's Diet_Category field.
              Classifications are never inferred from names.
            </div>
          </div>
          <div className="form-group">
            <label className="form-label">Excluded Ingredient IDs</label>
            <input
              className="form-input"
              placeholder="I001, I002, …"
              value={localCfg.excluded_ingredient_ids?.join(', ') || ''}
              onChange={e => setLocalCfg(c => ({
                ...c,
                excluded_ingredient_ids: e.target.value.split(',').map(s => s.trim()).filter(Boolean)
              }))}
            />
            <div className="form-hint">Comma-separated ingredient IDs to always exclude</div>
          </div>
          <div className="form-group">
            <label className="form-label">Preferred Ingredient IDs</label>
            <input
              className="form-input"
              placeholder="I003, I004, …"
              value={localCfg.preferred_ingredient_ids?.join(', ') || ''}
              onChange={e => setLocalCfg(c => ({
                ...c,
                preferred_ingredient_ids: e.target.value.split(',').map(s => s.trim()).filter(Boolean)
              }))}
            />
            <div className="form-hint">Preferences only — never override hard constraints or primary objective</div>
          </div>
        </div>

        {/* Hard nutritional constraints */}
        <div className="card">
          <div className="card-title">Daily Nutritional Constraints (Hard Limits)</div>
          <p style={{ fontSize: 12, color: 'var(--color-text-muted)', marginBottom: 14 }}>
            Plans violating any of these are infeasible, regardless of the objective score.
          </p>
          <NumField label="Min Calories (kcal/day)" name="min_calories" value={localCfg.min_calories} onChange={handleChange} min={0} />
          <NumField label="Max Calories (kcal/day)" name="max_calories" value={localCfg.max_calories} onChange={handleChange} min={0} />
          <NumField label="Min Protein (g/day)"     name="min_protein"  value={localCfg.min_protein}  onChange={handleChange} min={0} step={0.1} />
          <NumField label="Min Carbohydrates (g/day)" name="min_carbs"  value={localCfg.min_carbs}    onChange={handleChange} min={0} step={0.1} />
          <NumField label="Max Fat (g/day)"          name="max_fat"     value={localCfg.max_fat}      onChange={handleChange} min={0} step={0.1} />
        </div>

        {/* Daily nutrition targets */}
        <div className="card">
          <div className="card-title">Explicit Daily Nutrition Targets</div>
          <p style={{ fontSize: 12, color: 'var(--color-text-muted)', marginBottom: 14 }}>
            Used by Nutrition mode to minimize deviation. Must be set independently from hard constraints.
            Target calories must lie within [min, max]. Target protein ≥ min protein. Target carbs ≥ min carbs. Target fat ≤ max fat.
          </p>
          <NumField label="Target Calories (kcal/day)" name="target_calories" value={localCfg.target_calories} onChange={handleChange} min={0} />
          <NumField label="Target Protein (g/day)"     name="target_protein"  value={localCfg.target_protein}  onChange={handleChange} min={0} step={0.1} />
          <NumField label="Target Carbohydrates (g/day)" name="target_carbs"  value={localCfg.target_carbs}    onChange={handleChange} min={0} step={0.1} />
          <NumField label="Target Fat (g/day)"          name="target_fat"     value={localCfg.target_fat}      onChange={handleChange} min={0} step={0.1} />
        </div>
      </div>
    </div>
  )
}

