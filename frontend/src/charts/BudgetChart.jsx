import { Doughnut } from 'react-chartjs-2'
import { Chart as ChartJS, registerables } from 'chart.js'

ChartJS.register(...registerables)

export default function BudgetChart({ eval_, config }) {
  if (!eval_) return null

  const budget = config?.weekly_budget_inr ?? 2000
  const cost = eval_.weekly_purchase_cost || 0
  const remaining = budget - cost

  const doughData = {
    labels: ['Purchase Cost', 'Remaining Budget'],
    datasets: [{
      data: [
        Math.max(0, cost),
        Math.max(0, remaining),
      ],
      backgroundColor: ['rgba(37,99,235,0.7)', 'rgba(22,163,74,0.5)'],
      borderColor: ['rgba(37,99,235,1)', 'rgba(22,163,74,1)'],
      borderWidth: 1,
    }],
  }

  return (
    <div className="grid-2" style={{ marginBottom: 20 }}>
      <div className="card">
        <div className="card-title">Weekly Budget Allocation</div>
        <div style={{ maxWidth: 280, margin: '0 auto' }}>
          <Doughnut data={doughData} options={{ responsive: true, maintainAspectRatio: true }} />
        </div>
        <div style={{ marginTop: 16, fontSize: 13 }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: 6 }}>
            <span>Purchase cost</span><strong>₹{cost.toFixed(2)}</strong>
          </div>
          <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: 6 }}>
            <span>Gross recipe cost</span><strong>₹{(eval_.gross_recipe_cost || 0).toFixed(2)}</strong>
          </div>
          <div style={{ display: 'flex', justifyContent: 'space-between', borderTop: '1px solid var(--color-border)', paddingTop: 6 }}>
            <span>Budget remaining</span>
            <strong style={{ color: remaining >= 0 ? 'var(--color-green)' : 'var(--color-red)' }}>
              ₹{remaining.toFixed(2)}
            </strong>
          </div>
        </div>
      </div>
      <div className="card">
        <div className="card-title">Grocery Purchases</div>
        <div className="table-wrap" style={{ maxHeight: 280, overflowY: 'auto' }}>
          <table>
            <thead><tr><th>Ingredient</th><th>Purchase (g)</th><th>Cost (₹)</th></tr></thead>
            <tbody>
              {(eval_.grocery_list || []).filter(g => g.purchase_g > 0).map((g, i) => (
                <tr key={i}>
                  <td>{g.ingredient_name || g.ingredient_id}</td>
                  <td>{g.purchase_g?.toFixed(0)}</td>
                  <td>₹{g.purchase_cost_inr?.toFixed(2)}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  )
}
