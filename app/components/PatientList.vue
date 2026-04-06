<script setup lang="ts">
defineProps<{
  patients: Array<{
    id: string
    name: string
    latestReading?: {
      heartRate: number
      spo2: number
      alertLevel: string
    }
  }>
}>()

function getStatusColor(alertLevel?: string) {
  switch (alertLevel) {
    case 'critical': return 'bg-red-500'
    case 'warning': return 'bg-yellow-500'
    default: return 'bg-green-500'
  }
}

function getStatusBg(alertLevel?: string) {
  switch (alertLevel) {
    case 'critical': return 'bg-red-50 border-red-200'
    case 'warning': return 'bg-yellow-50 border-yellow-200'
    default: return 'bg-white border-gray-200'
  }
}
</script>

<template>
  <div class="space-y-3">
    <NuxtLink
      v-for="patient in patients"
      :key="patient.id"
      :to="`/patient/${patient.id}`"
      :class="['block rounded-xl border p-4 hover:shadow-md transition', getStatusBg(patient.latestReading?.alertLevel)]"
    >
      <div class="flex items-center justify-between">
        <div class="flex items-center gap-3">
          <!-- Status indicator -->
          <div :class="['w-3 h-3 rounded-full', getStatusColor(patient.latestReading?.alertLevel)]" />
          <div>
            <h3 class="font-medium text-gray-900">{{ patient.name }}</h3>
            <p class="text-sm text-gray-500">ID: {{ patient.id.slice(0, 8) }}...</p>
          </div>
        </div>
        <div v-if="patient.latestReading" class="text-right text-sm">
          <div class="text-gray-600">
            <span class="font-medium">{{ patient.latestReading.heartRate > 0 ? patient.latestReading.heartRate.toFixed(0) : '--' }}</span> BPM
          </div>
          <div class="text-gray-600">
            <span class="font-medium">{{ patient.latestReading.spo2 > 0 ? patient.latestReading.spo2.toFixed(0) : '--' }}</span>% SpO2
          </div>
        </div>
        <div v-else class="text-sm text-gray-400">No data</div>
      </div>
    </NuxtLink>

    <div v-if="patients.length === 0" class="text-center py-8 text-gray-400">
      <p>No linked patients</p>
      <p class="text-sm mt-1">Patients need to be linked to your account</p>
    </div>
  </div>
</template>
