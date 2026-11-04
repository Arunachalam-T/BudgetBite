import React from 'react'
import ReactDOM from 'react-dom/client'
import App from './App.jsx'
import ErrorBoundary from './components/ErrorBoundary.jsx'
import './styles/global.css'

// Globally register all Chart.js components (controllers, elements, scales, plugins)
import { Chart as ChartJS, registerables } from 'chart.js'
ChartJS.register(...registerables)

ReactDOM.createRoot(document.getElementById('root')).render(
  <React.StrictMode>
    <ErrorBoundary>
      <App />
    </ErrorBoundary>
  </React.StrictMode>
)
