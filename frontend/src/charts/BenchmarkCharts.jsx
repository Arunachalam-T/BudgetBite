import { Bar } from 'react-chartjs-2'
import { Chart as ChartJS, registerables } from 'chart.js'

ChartJS.register(...registerables)

const ALGO_COLORS = [
  'rgba(239,68,68,0.7)',    // red    — Brute Force
  'rgba(37,99,235,0.7)',    // blue   — Greedy
  'rgba(124,58,237,0.7)',   // purple — DP
  'rgba(22,163,74,0.7)',    // green  — Backtracking
  'rgba(217,119,6,0.7)',    // amber  — B&B
]
const ALGO_BORDER = ALGO_COLORS.map(c => c.replace('0.7', '1'))

function makeBarData(results, valueKey, label) {
  return {
    labels: results.map(r => r.algorithm_name),
    datasets: [{
      label,
      data: results.map(r => r[valueKey] ?? 0),
      backgroundColor: ALGO_COLORS,
      borderColor: ALGO_BORDER,
      borderWidth: 1,
    }],
  }
}

const baseOpts = (title) => ({
  responsive: true,
  plugins: {
    legend: { display: false },
    title: { display: true, text: title },
  },
  scales: { y: { beginAtZero: true } },
})

export default function BenchmarkCharts({ results, mode }) {
  if (!results?.length) return null

  const timeData = makeBarData(results, 'execution_time_ms', 'Execution Time (ms)')
  const nodeData = makeBarData(results, 'nodes_explored',    'Nodes/States Explored')
  const pruneData= makeBarData(results, 'branches_pruned',   'Branches Pruned')
  const memData  = {
    labels: results.map(r => r.algorithm_name),
    datasets: [{
      label: 'Memory (KB)',
      data: results.map(r => (r.peak_memory_bytes || 0) / 1024),
      backgroundColor: ALGO_COLORS,
      borderColor: ALGO_BORDER,
      borderWidth: 1,
    }],
  }
  const objData  = {
    labels: results.map(r => r.algorithm_name),
    datasets: [{
      label: 'Objective Score',
      data: results.map(r => r.is_feasible ? r.primary_objective : 0),
      backgroundColor: ALGO_COLORS,
      borderColor: ALGO_BORDER,
      borderWidth: 1,
    }],
  }

  return (
    <div>
      <div className="grid-2" style={{ marginBottom: 20 }}>
        <div className="card">
          <div className="card-title">Execution Time (ms)</div>
          <Bar data={timeData} options={baseOpts('Execution Time')} />
        </div>
        <div className="card">
          <div className="card-title">Nodes / States Explored</div>
          <Bar data={nodeData} options={baseOpts('Nodes Explored')} />
        </div>
        <div className="card">
          <div className="card-title">Branches Pruned</div>
          <Bar data={pruneData} options={baseOpts('Pruned Branches')} />
        </div>
        <div className="card">
          <div className="card-title">Memory Usage (KB)</div>
          <Bar data={memData} options={baseOpts('Peak Memory')} />
        </div>
      </div>
      <div className="card" style={{ marginBottom: 20 }}>
        <div className="card-title">Objective Score Comparison (lower is better)</div>
        <Bar data={objData} options={{
          ...baseOpts('Objective Score'),
          plugins: { ...baseOpts('').plugins, title: { display: false } },
        }} />
      </div>
    </div>
  )
}
