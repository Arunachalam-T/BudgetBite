import { Bar } from 'react-chartjs-2'
import { Chart as ChartJS, registerables } from 'chart.js'

ChartJS.register(...registerables)

const DAY_LABELS = ['Mon','Tue','Wed','Thu','Fri','Sat','Sun']

export default function NutritionChart({ eval_, config }) {
  if (!eval_) return null

  const targetCal = config?.target_calories ?? 2000
  const targetProt = config?.target_protein ?? 75

  const opts = {
    responsive: true,
    maintainAspectRatio: true,
    plugins: {
      legend: { position: 'top' },
      title: { display: false }
    },
    scales: { y: { beginAtZero: true } }
  }

  const calData = {
    labels: DAY_LABELS,
    datasets: [
      {
        label: 'Actual Calories',
        data: eval_.daily_calories || [],
        backgroundColor: 'rgba(37,99,235,0.6)',
        borderColor: 'rgba(37,99,235,1)',
        borderWidth: 1,
      },
      {
        label: 'Target',
        data: Array(7).fill(targetCal),
        backgroundColor: 'rgba(22,163,74,0.3)',
        borderColor: 'rgba(22,163,74,1)',
        borderWidth: 2,
        type: 'line',
        fill: false,
        pointStyle: 'circle',
      },
    ],
  }

  const protData = {
    labels: DAY_LABELS,
    datasets: [
      {
        label: 'Actual Protein (g)',
        data: eval_.daily_protein || [],
        backgroundColor: 'rgba(124,58,237,0.6)',
        borderColor: 'rgba(124,58,237,1)',
        borderWidth: 1,
      },
      {
        label: 'Target',
        data: Array(7).fill(targetProt),
        backgroundColor: 'rgba(22,163,74,0.3)',
        borderColor: 'rgba(22,163,74,1)',
        borderWidth: 2,
        type: 'line',
        fill: false,
        pointStyle: 'circle',
      },
    ],
  }

  return (
    <div className="grid-2" style={{ marginBottom: 20 }}>
      <div className="card">
        <div className="card-title">Daily Calories vs Target</div>
        <Bar data={calData} options={opts} />
      </div>
      <div className="card">
        <div className="card-title">Daily Protein vs Target (g)</div>
        <Bar data={protData} options={opts} />
      </div>
    </div>
  )
}
