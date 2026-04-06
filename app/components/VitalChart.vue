<script setup lang="ts">
import { Line } from 'vue-chartjs'
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler,
} from 'chart.js'

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend, Filler)

const props = defineProps<{
  readings: Array<{
    heartRate: number
    spo2: number
    timestamp: any
  }>
}>()

const chartData = computed(() => {
  const sorted = [...props.readings].reverse() // oldest first for chart

  const labels = sorted.map((r) => {
    if (!r.timestamp) return ''
    const date = r.timestamp?.toDate ? r.timestamp.toDate() : new Date(r.timestamp)
    return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
  })

  return {
    labels,
    datasets: [
      {
        label: 'Heart Rate (BPM)',
        data: sorted.map((r) => (r.heartRate > 0 ? r.heartRate : null)),
        borderColor: '#ef4444',
        backgroundColor: 'rgba(239, 68, 68, 0.1)',
        fill: true,
        tension: 0.3,
        pointRadius: 2,
      },
      {
        label: 'SpO2 (%)',
        data: sorted.map((r) => (r.spo2 > 0 ? r.spo2 : null)),
        borderColor: '#3b82f6',
        backgroundColor: 'rgba(59, 130, 246, 0.1)',
        fill: true,
        tension: 0.3,
        pointRadius: 2,
      },
    ],
  }
})

const chartOptions = {
  responsive: true,
  maintainAspectRatio: false,
  interaction: {
    intersect: false,
    mode: 'index' as const,
  },
  plugins: {
    legend: {
      position: 'top' as const,
    },
    tooltip: {
      backgroundColor: 'rgba(0,0,0,0.8)',
    },
  },
  scales: {
    y: {
      beginAtZero: false,
      grid: {
        color: 'rgba(0,0,0,0.05)',
      },
    },
    x: {
      grid: {
        display: false,
      },
    },
  },
}
</script>

<template>
  <div class="bg-white rounded-xl border border-gray-200 p-5">
    <h3 class="text-sm font-medium text-gray-500 mb-4">Vital Signs Over Time</h3>
    <div class="h-64">
      <Line v-if="readings.length > 0" :data="chartData" :options="chartOptions" />
      <div v-else class="h-full flex items-center justify-center text-gray-400">
        No readings available yet
      </div>
    </div>
  </div>
</template>
