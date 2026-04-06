<script setup lang="ts">
definePageMeta({ layout: false })

const { isAuthenticated, loading } = useAuth()

watchEffect(() => {
  if (!loading.value) {
    if (isAuthenticated.value) {
      navigateTo('/dashboard')
    } else {
      navigateTo('/login')
    }
  }
})
</script>

<template>
  <div class="min-h-screen flex items-center justify-center bg-gray-50">
    <div class="text-center text-gray-400">
      <div class="animate-spin w-8 h-8 border-4 border-blue-500 border-t-transparent rounded-full mx-auto mb-3" />
      <p>Loading...</p>
    </div>
  </div>
</template>
