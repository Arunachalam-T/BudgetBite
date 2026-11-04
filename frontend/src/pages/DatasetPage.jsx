import { useState, useEffect } from 'react'
import { useApp } from '../AppContext'
import { Upload, AlertCircle, CheckCircle, Search, FileSpreadsheet, FileText, Sparkles, Folder } from 'lucide-react'
import api from '../services/api'

// Helper to sanitize paths (trim spaces, remove quotes from Windows "Copy as path")
function cleanPath(p) {
  if (!p) return ''
  let s = p.trim()
  if ((s.startsWith('"') && s.endsWith('"')) || (s.startsWith("'") && s.endsWith("'"))) {
    s = s.slice(1, -1).trim()
  }
  return s
}

function ValidationMessages({ messages }) {
  if (!messages?.length) return null
  const fatals   = messages.filter(m => m.severity === 'FATAL')
  const warnings = messages.filter(m => m.severity === 'WARNING')
  return (
    <div style={{ marginTop: 14 }}>
      {fatals.map((m, i) => (
        <div key={i} className="alert alert-error" style={{ marginBottom: 6 }}>
          <AlertCircle size={14} />
          <span><strong>{m.worksheet}{m.row > 0 ? ` row ${m.row}` : ''}: </strong>{m.message}</span>
        </div>
      ))}
      {warnings.map((m, i) => (
        <div key={i} className="alert alert-warn" style={{ marginBottom: 6 }}>
          <AlertCircle size={14} />
          <span><strong>{m.worksheet}{m.row > 0 ? ` row ${m.row}` : ''}: </strong>{m.message}</span>
        </div>
      ))}
    </div>
  )
}

function IngredientsTable({ ingredients }) {
  const [search, setSearch] = useState('')
  const filtered = ingredients.filter(i =>
    i.name.toLowerCase().includes(search.toLowerCase()) ||
    i.ingredient_id.toLowerCase().includes(search.toLowerCase())
  )
  return (
    <div>
      <div style={{ display: 'flex', gap: 10, marginBottom: 12 }}>
        <div style={{ position: 'relative', flex: 1 }}>
          <Search size={14} style={{ position: 'absolute', left: 10, top: '50%', transform: 'translateY(-50%)', color: 'var(--color-text-muted)' }} />
          <input
            className="form-input"
            style={{ paddingLeft: 30 }}
            placeholder="Search ingredients…"
            value={search}
            onChange={e => setSearch(e.target.value)}
          />
        </div>
        <span style={{ fontSize: 13, color: 'var(--color-text-muted)', lineHeight: '36px' }}>{filtered.length} results</span>
      </div>
      <div className="table-wrap">
        <table>
          <thead><tr>
            <th>ID</th><th>Name</th><th>Price/kg (₹)</th>
            <th>Cal/100g</th><th>Prot/100g</th><th>Carbs/100g</th><th>Fat/100g</th><th>Diet</th>
          </tr></thead>
          <tbody>
            {filtered.slice(0, 100).map((i, idx) => (
              <tr key={idx}>
                <td><code style={{ fontSize: 11 }}>{i.ingredient_id}</code></td>
                <td>{i.name}</td>
                <td>₹{i.price_per_kg_inr}</td>
                <td>{i.calories_per_100g}</td>
                <td>{i.protein_g_per_100g}</td>
                <td>{i.carbs_g_per_100g}</td>
                <td>{i.fat_g_per_100g}</td>
                <td><span className="badge badge-green" style={{ fontSize: 10 }}>{i.diet_category}</span></td>
              </tr>
            ))}
          </tbody>
        </table>
        {filtered.length > 100 && <p style={{ fontSize: 12, color: 'var(--color-text-muted)', padding: 8 }}>Showing first 100 of {filtered.length}</p>}
      </div>
    </div>
  )
}

function RecipesTable({ recipes }) {
  const [search, setSearch] = useState('')
  const filtered = recipes.filter(r =>
    r.recipe_name.toLowerCase().includes(search.toLowerCase()) ||
    r.recipe_id.toLowerCase().includes(search.toLowerCase())
  )
  return (
    <div>
      <div style={{ display: 'flex', gap: 10, marginBottom: 12 }}>
        <div style={{ position: 'relative', flex: 1 }}>
          <Search size={14} style={{ position: 'absolute', left: 10, top: '50%', transform: 'translateY(-50%)', color: 'var(--color-text-muted)' }} />
          <input className="form-input" style={{ paddingLeft: 30 }} placeholder="Search recipes…" value={search} onChange={e => setSearch(e.target.value)} />
        </div>
        <span style={{ fontSize: 13, color: 'var(--color-text-muted)', lineHeight: '36px' }}>{filtered.length} results</span>
      </div>
      <div className="table-wrap">
        <table>
          <thead><tr>
            <th>ID</th><th>Name</th><th>Meal Type</th>
            <th>Cal/serving</th><th>Prot(g)</th><th>Carbs(g)</th><th>Fat(g)</th><th>Cost(₹)</th>
          </tr></thead>
          <tbody>
            {filtered.slice(0, 100).map((r, idx) => (
              <tr key={idx}>
                <td><code style={{ fontSize: 11 }}>{r.recipe_id}</code></td>
                <td>{r.recipe_name}</td>
                <td><span className="badge badge-blue" style={{ fontSize: 10 }}>{r.meal_type}</span></td>
                <td>{r.calories_per_serving?.toFixed(0)}</td>
                <td>{r.protein_per_serving?.toFixed(1)}</td>
                <td>{r.carbs_per_serving?.toFixed(1)}</td>
                <td>{r.fat_per_serving?.toFixed(1)}</td>
                <td>₹{r.gross_cost_per_serving?.toFixed(2)}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  )
}

export default function DatasetPage() {
  const { dataset, validation, uploadCsv, uploadExcel, loading, error, refreshSummary } = useApp()
  const [tab, setTab] = useState('upload')
  const [uploadMode, setUploadMode] = useState('excel') // 'excel' | 'csv'
  const [ingredients, setIngredients] = useState([])
  const [recipes, setRecipes] = useState([])

  // File path inputs
  const [excelPath, setExcelPath] = useState('data/SmartMeal_Input_Template.xlsx')
  const [paths, setPaths] = useState({
    ing: 'data/ingredients.csv',
    rec: 'data/recipes.csv',
    ri:  'data/recipe_ingredients.csv',
    inv: 'data/inventory.csv'
  })

  const isLoading = loading?.upload
  const uploadError = error?.upload

  useEffect(() => {
    if (tab === 'ingredients' && ingredients.length === 0) {
      api.ingredients().then(setIngredients).catch(() => {})
    }
    if (tab === 'recipes' && recipes.length === 0) {
      api.recipes().then(setRecipes).catch(() => {})
    }
  }, [tab])

  const handleUploadCsv = async () => {
    try {
      await uploadCsv(
        cleanPath(paths.ing),
        cleanPath(paths.rec),
        cleanPath(paths.ri),
        cleanPath(paths.inv)
      )
      setTab('ingredients')
      api.ingredients().then(setIngredients).catch(() => {})
      api.recipes().then(setRecipes).catch(() => {})
    } catch {}
  }

  const handleUploadExcel = async () => {
    try {
      await uploadExcel(cleanPath(excelPath))
      setTab('ingredients')
      api.ingredients().then(setIngredients).catch(() => {})
      api.recipes().then(setRecipes).catch(() => {})
    } catch {}
  }

  const fillSampleCsv = (absolute = false) => {
    if (absolute) {
      setPaths({
        ing: 'C:\\Users\\Arunachalam\\OneDrive\\Desktop\\Meal Manager\\data\\ingredients.csv',
        rec: 'C:\\Users\\Arunachalam\\OneDrive\\Desktop\\Meal Manager\\data\\recipes.csv',
        ri:  'C:\\Users\\Arunachalam\\OneDrive\\Desktop\\Meal Manager\\data\\recipe_ingredients.csv',
        inv: 'C:\\Users\\Arunachalam\\OneDrive\\Desktop\\Meal Manager\\data\\inventory.csv'
      })
    } else {
      setPaths({
        ing: 'data/ingredients.csv',
        rec: 'data/recipes.csv',
        ri:  'data/recipe_ingredients.csv',
        inv: 'data/inventory.csv'
      })
    }
  }

  const fillSampleExcel = (absolute = false) => {
    if (absolute) {
      setExcelPath('C:\\Users\\Arunachalam\\OneDrive\\Desktop\\Meal Manager\\data\\SmartMeal_Input_Template.xlsx')
    } else {
      setExcelPath('data/SmartMeal_Input_Template.xlsx')
    }
  }

  return (
    <div>
      <h2 className="section-title">Dataset</h2>
      <p className="section-sub">Load datasets directly by entering file paths (Excel or CSV), inspect ingredients and recipes</p>

      {/* Dataset Guidance Banner */}
      <div className="card" style={{ marginBottom: 20, background: 'linear-gradient(to right, rgba(99, 102, 241, 0.05), rgba(16, 185, 129, 0.05))', borderLeft: '4px solid var(--color-primary)' }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginBottom: 6 }}>
          <Sparkles size={18} style={{ color: 'var(--color-primary)' }} />
          <strong style={{ fontSize: 14 }}>Which Datasets To Upload for Testing:</strong>
        </div>
        <p style={{ fontSize: 13, color: 'var(--color-text)', lineHeight: 1.6, margin: 0 }}>
          To verify system functionality, you can write the path to either:
        </p>
        <ul style={{ fontSize: 13, marginTop: 6, marginBottom: 8, paddingLeft: 20, lineHeight: 1.6 }}>
          <li><strong>Option 1 (Excel All-in-One):</strong> <code>data/SmartMeal_Input_Template.xlsx</code> (contains Ingredients, Recipes, Recipe_Ingredients, Inventory sheets)</li>
          <li><strong>Option 2 (Four CSV Files):</strong> <code>data/ingredients.csv</code>, <code>data/recipes.csv</code>, <code>data/recipe_ingredients.csv</code>, and <code>data/inventory.csv</code></li>
        </ul>
        <span style={{ fontSize: 12, color: 'var(--color-text-muted)' }}>
          Tip: You can enter relative paths (e.g. <code>data/SmartMeal_Input_Template.xlsx</code>) or full Windows paths (e.g. <code>C:\Users\...\data\...</code>). Quotes around copied paths are automatically stripped.
        </span>
      </div>

      <div className="tabs">
        {[
          { key: 'upload',      label: 'Upload by Path' },
          { key: 'validation',  label: 'Validation' },
          { key: 'ingredients', label: `Ingredients (${ingredients.length})` },
          { key: 'recipes',     label: `Recipes (${recipes.length})` },
        ].map(({ key, label }) => (
          <button key={key} className={`tab-btn ${tab === key ? 'active' : ''}`} onClick={() => setTab(key)}>
            {label}
          </button>
        ))}
      </div>

      {/* Upload tab */}
      {tab === 'upload' && (
        <div>
          {/* Format Selection Toggle */}
          <div style={{ display: 'flex', gap: 12, marginBottom: 16 }}>
            <button
              className={`btn ${uploadMode === 'excel' ? 'btn-primary' : 'btn-ghost'}`}
              onClick={() => setUploadMode('excel')}
              style={{ display: 'flex', alignItems: 'center', gap: 8 }}
            >
              <FileSpreadsheet size={16} /> Excel Workbook (.xlsx)
            </button>
            <button
              className={`btn ${uploadMode === 'csv' ? 'btn-primary' : 'btn-ghost'}`}
              onClick={() => setUploadMode('csv')}
              style={{ display: 'flex', alignItems: 'center', gap: 8 }}
            >
              <FileText size={16} /> Four CSV Files
            </button>
          </div>

          {/* EXCEL PATH INPUT */}
          {uploadMode === 'excel' && (
            <div className="card" style={{ marginBottom: 20 }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 12 }}>
                <div className="card-title" style={{ margin: 0, display: 'flex', alignItems: 'center', gap: 8 }}>
                  <FileSpreadsheet size={18} style={{ color: 'var(--color-green)' }} />
                  Enter Excel Workbook Path (.xlsx)
                </div>
                <div style={{ display: 'flex', gap: 8 }}>
                  <button className="btn btn-ghost btn-sm" onClick={() => fillSampleExcel(false)}>
                    Quick Fill Relative
                  </button>
                  <button className="btn btn-ghost btn-sm" onClick={() => fillSampleExcel(true)}>
                    Quick Fill Absolute
                  </button>
                </div>
              </div>

              <div className="form-group">
                <label className="form-label">Excel File Path</label>
                <input
                  className="form-input"
                  style={{ fontFamily: 'monospace', fontSize: 13 }}
                  placeholder="e.g. data/SmartMeal_Input_Template.xlsx or C:\path\to\template.xlsx"
                  value={excelPath}
                  onChange={e => setExcelPath(e.target.value)}
                />
                <div className="form-hint">
                  Path to an Excel file with sheets: <strong>Ingredients</strong>, <strong>Recipes</strong>, <strong>Recipe_Ingredients</strong>, and <strong>Inventory</strong>.
                </div>
              </div>

              {uploadError && <div className="alert alert-error" style={{ marginBottom: 12 }}><AlertCircle size={14} /> {uploadError}</div>}

              <button
                className="btn btn-primary"
                disabled={isLoading || !cleanPath(excelPath)}
                onClick={handleUploadExcel}
              >
                {isLoading ? <><span className="spinner" style={{ width: 14, height: 14 }} /> Loading & Validating…</> : <><Upload size={14} /> Load Excel Dataset</>}
              </button>
            </div>
          )}

          {/* CSV PATHS INPUT */}
          {uploadMode === 'csv' && (
            <div className="card" style={{ marginBottom: 20 }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 12 }}>
                <div className="card-title" style={{ margin: 0, display: 'flex', alignItems: 'center', gap: 8 }}>
                  <FileText size={18} style={{ color: 'var(--color-primary)' }} />
                  Enter CSV File Paths
                </div>
                <div style={{ display: 'flex', gap: 8 }}>
                  <button className="btn btn-ghost btn-sm" onClick={() => fillSampleCsv(false)}>
                    Quick Fill Relative
                  </button>
                  <button className="btn btn-ghost btn-sm" onClick={() => fillSampleCsv(true)}>
                    Quick Fill Absolute
                  </button>
                </div>
              </div>

              <p style={{ fontSize: 13, color: 'var(--color-text-muted)', marginBottom: 16 }}>
                Type or paste the paths to your four CSV files. Surrounding quotes are automatically handled.
              </p>

              {[
                { key: 'ing', label: '1. Ingredients CSV Path', hint: 'Required — Ingredient_ID, Name, Price_per_kg_INR, Calories_per_100g, Protein_g_per_100g, Carbs_g_per_100g, Fat_g_per_100g, Diet_Category' },
                { key: 'rec', label: '2. Recipes CSV Path',      hint: 'Required — Recipe_ID, Recipe_Name, Meal_Type' },
                { key: 'ri',  label: '3. Recipe Ingredients CSV Path', hint: 'Required — Recipe_ID, Ingredient_ID, Quantity_g_per_serving' },
                { key: 'inv', label: '4. Inventory CSV Path',   hint: 'Optional — Ingredient_ID, Available_Quantity_g' },
              ].map(({ key, label, hint }) => (
                <div key={key} className="form-group">
                  <label className="form-label">{label}</label>
                  <input
                    className="form-input"
                    style={{ fontFamily: 'monospace', fontSize: 13 }}
                    placeholder="e.g. data/ingredients.csv or C:\path\to\file.csv"
                    value={paths[key]}
                    onChange={e => setPaths(p => ({ ...p, [key]: e.target.value }))}
                  />
                  <div className="form-hint">{hint}</div>
                </div>
              ))}

              {uploadError && <div className="alert alert-error" style={{ marginBottom: 12 }}><AlertCircle size={14} /> {uploadError}</div>}

              <button
                className="btn btn-primary"
                disabled={isLoading || !cleanPath(paths.ing) || !cleanPath(paths.rec) || !cleanPath(paths.ri)}
                onClick={handleUploadCsv}
              >
                {isLoading ? <><span className="spinner" style={{ width: 14, height: 14 }} /> Loading & Validating…</> : <><Upload size={14} /> Load CSV Dataset</>}
              </button>
            </div>
          )}

          {/* Status Banner */}
          {dataset && (
            <div className={`alert ${dataset.is_valid ? 'alert-success' : 'alert-warn'}`}>
              {dataset.is_valid ? <CheckCircle size={18} /> : <AlertCircle size={18} />}
              <div>
                <strong>{dataset.is_valid ? 'Dataset loaded and validated successfully!' : 'Dataset loaded with validation issues'}</strong>
                <div style={{ marginTop: 4, fontSize: 13 }}>
                  Source: <code>{dataset.source_name}</code> · {dataset.ingredients_count} ingredients · {dataset.recipes_count} recipes
                </div>
              </div>
            </div>
          )}
        </div>
      )}

      {/* Validation tab */}
      {tab === 'validation' && (
        <div className="card">
          <div className="card-title">Validation Results</div>
          {!validation ? (
            <p style={{ color: 'var(--color-text-muted)' }}>No dataset loaded yet. Enter paths in the Upload tab and click Load.</p>
          ) : validation.is_valid ? (
            <div className="alert alert-success">
              <CheckCircle size={16} /> Dataset passed validation with {validation.messages?.filter(m => m.severity === 'WARNING').length || 0} warning(s). All integrity checks satisfied.
            </div>
          ) : (
            <>
              <div className="alert alert-error">
                <AlertCircle size={16} />
                Validation failed with {validation.messages?.filter(m => m.severity === 'FATAL').length} fatal error(s).
                Fix these errors before optimizing.
              </div>
            </>
          )}
          <ValidationMessages messages={validation?.messages} />
        </div>
      )}

      {/* Ingredients tab */}
      {tab === 'ingredients' && (
        <div className="card">
          <div className="card-title">Ingredients ({ingredients.length})</div>
          {ingredients.length === 0 ? (
            <p style={{ color: 'var(--color-text-muted)' }}>No ingredients loaded yet. Please load a dataset first.</p>
          ) : (
            <IngredientsTable ingredients={ingredients} />
          )}
        </div>
      )}

      {/* Recipes tab */}
      {tab === 'recipes' && (
        <div className="card">
          <div className="card-title">Recipes ({recipes.length})</div>
          {recipes.length === 0 ? (
            <p style={{ color: 'var(--color-text-muted)' }}>No recipes loaded yet. Please load a dataset first.</p>
          ) : (
            <RecipesTable recipes={recipes} />
          )}
        </div>
      )}
    </div>
  )
}
