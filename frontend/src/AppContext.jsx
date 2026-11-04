import { createContext, useContext, useState, useCallback } from 'react'
import api from './services/api'

const AppContext = createContext(null)

export function AppProvider({ children }) {
  const [dataset, setDataset]     = useState(null)       // dataset summary
  const [validation, setValidation] = useState(null)
  const [config, setConfig]       = useState({
    weekly_budget_inr: 2000,
    meals_per_day: 3,
    max_recipe_repetitions: 3,
    diet_type: 'Unrestricted',
    min_calories: 1500,
    max_calories: 2500,
    min_protein: 50,
    min_carbs: 150,
    max_fat: 80,
    target_calories: 2000,
    target_protein: 75,
    target_carbs: 250,
    target_fat: 60,
    excluded_ingredient_ids: [],
    preferred_ingredient_ids: [],
  })
  const [results, setResults]     = useState({})         // {plan, nutrition}
  const [benchmark, setBenchmark] = useState(null)
  const [loading, setLoading]     = useState({})         // {key: bool}
  const [error, setError]         = useState({})         // {key: string}

  const setLoad = (key, val) => setLoading(l => ({ ...l, [key]: val }))
  const setErr  = (key, val) => setError(e => ({ ...e, [key]: val }))
  const clearErr = (key)     => setError(e => ({ ...e, [key]: null }))

  const refreshSummary = useCallback(async () => {
    try {
      const s = await api.datasetSummary()
      setDataset(s)
      setValidation(s.validation)
    } catch (e) {
      setDataset(null)
    }
  }, [])

  const uploadCsv = useCallback(async (ing, rec, ri, inv) => {
    setLoad('upload', true); setErr('upload', null)
    try {
      const res = await api.uploadCsv(ing, rec, ri, inv)
      setValidation(res.validation)
      await refreshSummary()
      return res
    } catch (e) {
      setErr('upload', e.message)
      throw e
    } finally {
      setLoad('upload', false)
    }
  }, [refreshSummary])

  const uploadExcel = useCallback(async (filepath) => {
    setLoad('upload', true); setErr('upload', null)
    try {
      const res = await api.uploadExcel(filepath)
      setValidation(res.validation)
      await refreshSummary()
      return res
    } catch (e) {
      setErr('upload', e.message)
      throw e
    } finally {
      setLoad('upload', false)
    }
  }, [refreshSummary])

  const generatePlan = useCallback(async (algorithm = 'Backtracking') => {
    setLoad('plan', true); setErr('plan', null)
    try {
      const res = await api.generatePlan(config, 'Nutrition', algorithm)
      setResults(r => ({ ...r, plan: res, nutrition: res }))
      return res
    } catch (e) {
      setErr('plan', e.message)
      throw e
    } finally {
      setLoad('plan', false)
    }
  }, [config])

  const runBenchmark = useCallback(async () => {
    setLoad('benchmark', true); setErr('benchmark', null)
    try {
      const res = await api.benchmark(config, 'Nutrition')
      setBenchmark(res)
      return res
    } catch (e) {
      setErr('benchmark', e.message)
      throw e
    } finally {
      setLoad('benchmark', false)
    }
  }, [config])

  const exportExcel = useCallback(async (customPath) => {
    const filepath = customPath || `SmartMeal_Plan_${Date.now()}.xlsx`
    return api.exportExcel('Nutrition', filepath)
  }, [])

  return (
    <AppContext.Provider value={{
      dataset, validation, config, setConfig,
      results, benchmark,
      loading, error,
      uploadCsv, uploadExcel, generatePlan, runBenchmark, exportExcel,
      refreshSummary,
    }}>
      {children}
    </AppContext.Provider>
  )
}

export const useApp = () => useContext(AppContext)

