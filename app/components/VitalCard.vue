<script setup lang="ts">
const props = defineProps<{
  label: string
  value: number | string
  unit: string
  alertLevel?: 'normal' | 'warning' | 'critical'
  icon?: string
}>()

const colorClasses = computed(() => {
  switch (props.alertLevel) {
    case 'critical':
      return {
        card: 'border-red-200 bg-red-50',
        value: 'text-red-700',
        badge: 'bg-red-100 text-red-700',
        icon: 'text-red-500',
      }
    case 'warning':
      return {
        card: 'border-yellow-200 bg-yellow-50',
        value: 'text-yellow-700',
        badge: 'bg-yellow-100 text-yellow-700',
        icon: 'text-yellow-500',
      }
    default:
      return {
        card: 'border-green-200 bg-white',
        value: 'text-green-700',
        badge: 'bg-green-100 text-green-700',
        icon: 'text-green-500',
      }
  }
})
</script>

<template>
  <div :class="['rounded-xl border-2 p-5 transition-all', colorClasses.card]">
    <div class="flex items-center justify-between mb-3">
      <span class="text-sm font-medium text-gray-500">{{ label }}</span>
      <span
        v-if="alertLevel && alertLevel !== 'normal'"
        :class="['text-xs font-semibold px-2 py-0.5 rounded-full uppercase', colorClasses.badge]"
      >
        {{ alertLevel }}
      </span>
    </div>
    <div class="flex items-baseline gap-1">
      <span :class="['text-3xl font-bold', colorClasses.value]">
        {{ typeof value === 'number' && value > 0 ? value.toFixed(0) : '--' }}
      </span>
      <span class="text-sm text-gray-400 ml-1">{{ unit }}</span>
    </div>
  </div>
</template>
