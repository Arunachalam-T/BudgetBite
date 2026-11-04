// api.js — Frontend API service layer

const BASE = '/api'

async function request(method, path, body = null) {
  const opts = {
    method,
    headers: { 'Content-Type': 'application/json' },
  }
  if (body) opts.body = JSON.stringify(body)
  const res = await fetch(`${BASE}${path}`, opts)
  const data = await res.json().catch(() => ({}))
  if (!res.ok) throw new Error(data.error || `HTTP ${res.status}`)
  return data
}

export const api = {
  health:           ()      => request('GET', '/health'),
  datasetSummary:   ()      => request('GET', '/dataset/summary'),
  ingredients:      ()      => request('GET', '/dataset/ingredients'),
  recipes:          ()      => request('GET', '/dataset/recipes'),

  uploadCsv: (ing, rec, ri, inv) =>
    request('POST', '/dataset/upload', {
      type: 'csv',
      ingredients: ing,
      recipes: rec,
      recipe_ingredients: ri,
      inventory: inv || '',
    }),

  uploadExcel: (filepath) =>
    request('POST', '/dataset/upload', { type: 'excel', filepath }),

  validateConfig: (config) =>
    request('POST', '/optimization/validate-config', config),

  generatePlan: (config, mode, algorithm = 'Backtracking', limits = {}) =>
    request('POST', '/optimization/generate-plan', {
      ...config, mode, algorithm, ...limits,
    }),

  benchmark: (config, mode, limits = {}) =>
    request('POST', '/algorithms/benchmark', { ...config, mode, ...limits }),

  latestResults: () => request('GET', '/results/latest'),

  exportExcel: (mode, filepath) =>
    request('POST', '/export/excel', { mode, filepath }),
}

export default api

