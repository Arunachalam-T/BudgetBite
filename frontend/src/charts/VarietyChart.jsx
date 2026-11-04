import { Bar } from 'react-chartjs-2'
import { Chart as ChartJS, registerables } from 'chart.js'

ChartJS.register(...registerables)

export default function VarietyChart({ eval_ }) {
  if (!eval_) return null

  const reps = eval_.recipe_repetitions || {}
  const entries = Object.entries(reps).sort((a, b) => b[1] - a[1])

  const barData = {
    labels: entries.map(([id]) => id),
    datasets: [{
      label: 'Times Used',
      data: entries.map(([, cnt]) => cnt),
      backgroundColor: 'rgba(124,58,237,0.65)',
      borderColor: 'rgba(124,58,237,1)',
      borderWidth: 1,
    }],
  }

  const distinctCount = eval_.distinct_recipe_count ?? entries.length
  const totalSlots = Object.values(reps).reduce((a, b) => a + b, 0)
  const varietyScore = eval_.variety_score ?? 0

  return (
    <div className="grid-2" style={{ marginBottom: 20 }}>
      <div className="card">
        <div className="card-title">Recipe Usage Frequency</div>
        {entries.length > 0 ? (
          <Bar data={barData} options={{ responsive: true, plugins: { legend: { display: false } }, scales: { y: { beginAtZero: true, ticks: { stepSize: 1 } } } }} />
        ) : (
          <p style={{ color: 'var(--color-text-muted)' }}>No recipe repetition data available.</p>
        )}
      </div>
      <div className="card">
        <div className="card-title">Variety Summary</div>
        <div style={{ fontSize: 13, lineHeight: 2 }}>
          <div>Distinct recipes: <strong style={{ color: 'var(--color-purple)' }}>{distinctCount}</strong></div>
          <div>Total meal slots: <strong>{totalSlots}</strong></div>
          <div>Variety score: <strong>{varietyScore.toFixed(1)}%</strong></div>
        </div>
        <div style={{ marginTop: 16 }}>
          <div className="progress-bar" style={{ height: 14 }}>
            <div className="progress-fill" style={{ width: `${Math.min(100, Math.max(0, varietyScore))}%`, background: 'var(--color-purple)' }} />
          </div>
          <div style={{ fontSize: 11, color: 'var(--color-text-muted)', marginTop: 4 }}>
            {varietyScore.toFixed(1)}% variety
          </div>
        </div>
        {entries.length > 0 && (
          <div style={{ marginTop: 16 }}>
            <div className="table-wrap" style={{ maxHeight: 200, overflowY: 'auto' }}>
              <table>
                <thead><tr><th>Recipe ID</th><th>Times Used</th></tr></thead>
                <tbody>
                  {entries.map(([id, cnt], i) => (
                    <tr key={i}><td><code style={{ fontSize: 11 }}>{id}</code></td><td>{cnt}</td></tr>
                  ))}
                </tbody>
              </table>
            </div>
          </div>
        )}
      </div>
    </div>
  )
}
