import React from 'react'
import { AlertTriangle, RefreshCw } from 'lucide-react'

export class ErrorBoundary extends React.Component {
  constructor(props) {
    super(props)
    this.state = { hasError: false, error: null }
  }

  static getDerivedStateFromError(error) {
    return { hasError: true, error }
  }

  componentDidCatch(error, errorInfo) {
    console.error('ErrorBoundary caught an error:', error, errorInfo)
  }

  render() {
    if (this.state.hasError) {
      return (
        <div className="card" style={{ margin: 20, borderLeft: '4px solid var(--color-red)' }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: 10, color: 'var(--color-red)', marginBottom: 12 }}>
            <AlertTriangle size={24} />
            <h3 style={{ margin: 0, fontSize: 16 }}>Rendering Error</h3>
          </div>
          <p style={{ fontSize: 13, color: 'var(--color-text-muted)', marginBottom: 12 }}>
            An unexpected error occurred while displaying this view:
          </p>
          <pre style={{
            background: 'var(--color-surface-2)',
            padding: 12,
            borderRadius: 6,
            fontSize: 12,
            overflowX: 'auto',
            marginBottom: 16,
            color: 'var(--color-red)'
          }}>
            {this.state.error?.message || String(this.state.error)}
          </pre>
          <button
            className="btn btn-primary btn-sm"
            onClick={() => {
              this.setState({ hasError: false, error: null })
              window.location.reload()
            }}
          >
            <RefreshCw size={14} /> Reload Page
          </button>
        </div>
      )
    }

    return this.props.children
  }
}

export default ErrorBoundary

