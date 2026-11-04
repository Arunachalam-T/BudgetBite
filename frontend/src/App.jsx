import { useState, useEffect } from 'react'
import { BrowserRouter, Routes, Route, NavLink } from 'react-router-dom'
import {
  LayoutDashboard, UtensilsCrossed, BarChart3,
  Database, Settings, Leaf, Download
} from 'lucide-react'
import { AppProvider, useApp } from './AppContext'

import Dashboard   from './pages/Dashboard'
import MealsPage   from './pages/MealsPage'
import AlgoPage    from './pages/AlgoPage'
import DatasetPage from './pages/DatasetPage'
import SettingsPage from './pages/SettingsPage'

const NAV = [
  { to: '/',          icon: LayoutDashboard, label: 'Dashboard'  },
  { to: '/meals',     icon: UtensilsCrossed, label: 'Meals'      },
  { to: '/algorithms',icon: BarChart3,       label: 'Algorithms' },
  { to: '/dataset',   icon: Database,        label: 'Dataset'    },
  { to: '/settings',  icon: Settings,        label: 'Settings'   },
]

function Sidebar() {
  return (
    <aside className="sidebar">
      <div className="sidebar-logo">
        <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
          <Leaf size={20} style={{ color: 'var(--color-green)' }} />
          <h1>SmartMeal</h1>
        </div>
        <p>Optimizer v1.0</p>
      </div>
      <nav className="sidebar-nav">
        {NAV.map(({ to, icon: Icon, label }) => (
          <NavLink key={to} to={to} end={to === '/'}>
            {({ isActive }) => (
              <button className={`nav-item ${isActive ? 'active' : ''}`}>
                <Icon className="nav-icon" />
                {label}
              </button>
            )}
          </NavLink>
        ))}
      </nav>
    </aside>
  )
}

function Header() {
  const { dataset, validation, results, exportExcel } = useApp()

  const currentResult = results?.plan || results?.nutrition || Object.values(results || {})[0]
  const hasResult = currentResult?.is_feasible

  return (
    <header className="header">
      <span className="header-dataset">
        {dataset ? (
          <>
            <strong>{dataset.source_name || 'Dataset'}</strong>
            &nbsp;·&nbsp;
            <span style={{ color: validation?.is_valid ? 'var(--color-green)' : 'var(--color-red)' }}>
              {validation?.is_valid ? '✓ Valid' : '✗ Errors'}
            </span>
          </>
        ) : (
          <span style={{ color: 'var(--color-text-muted)' }}>No dataset loaded</span>
        )}
      </span>
      <span className="header-spacer" />
      {hasResult && (
        <button
          className="btn btn-green btn-sm"
          onClick={() => exportExcel('Nutrition')}
        >
          <Download size={14} /> Export Plan
        </button>
      )}
    </header>
  )
}

function Shell() {
  const { refreshSummary } = useApp()
  useEffect(() => { refreshSummary() }, [])

  return (
    <BrowserRouter>
      <div className="app-shell">
        <Sidebar />
        <div className="main-area">
          <Header />
          <div className="page">
            <Routes>
              <Route path="/"           element={<Dashboard />} />
              <Route path="/meals"      element={<MealsPage />} />
              <Route path="/algorithms" element={<AlgoPage />} />
              <Route path="/dataset"    element={<DatasetPage />} />
              <Route path="/settings"   element={<SettingsPage />} />
            </Routes>
          </div>
        </div>
      </div>
    </BrowserRouter>
  )
}

export default function App() {
  return (
    <AppProvider>
      <Shell />
    </AppProvider>
  )
}

