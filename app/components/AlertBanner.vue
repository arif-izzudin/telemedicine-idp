<script setup lang="ts">
const { alerts } = useAlerts()

const latestCritical = computed(() =>
  alerts.value.find((a) => a.alertLevel === 'critical' && !a.acknowledged)
)

const dismissed = ref(false)

watch(latestCritical, () => {
  dismissed.value = false
})
</script>

<template>
  <div
    v-if="latestCritical && !dismissed"
    class="bg-red-600 text-white px-4 py-3 flex items-center justify-between"
  >
    <div class="flex items-center gap-3">
      <svg class="w-5 h-5 animate-pulse flex-shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24">
        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
          d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-2.5L13.732 4c-.77-.833-1.964-.833-2.732 0L4.082 16.5c-.77.833.192 2.5 1.732 2.5z" />
      </svg>
      <div>
        <span class="font-semibold">CRITICAL ALERT</span>
        <span class="ml-2 text-red-100">{{ latestCritical.message }}</span>
        <span class="ml-2 text-red-200 text-sm">— {{ latestCritical.patientName }}</span>
      </div>
    </div>
    <button @click="dismissed = true" class="text-red-200 hover:text-white ml-4 flex-shrink-0">
      <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12" />
      </svg>
    </button>
  </div>
</template>
