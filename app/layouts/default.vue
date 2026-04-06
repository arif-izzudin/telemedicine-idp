<script setup lang="ts">
const { userName, userRole, logout } = useAuth()
const { unacknowledgedCount, requestNotificationPermission } = useAlerts()

const sidebarOpen = ref(false)

onMounted(() => {
  requestNotificationPermission()
})

async function handleLogout() {
  await logout()
  await navigateTo('/login')
}

const navItems = computed(() => {
  const items = [
    { to: '/dashboard', label: 'Dashboard', icon: 'home' },
  ]
  return items
})
</script>

<template>
  <div class="min-h-screen bg-gray-50">
    <!-- Mobile sidebar overlay -->
    <div v-if="sidebarOpen" class="fixed inset-0 z-40 lg:hidden">
      <div class="fixed inset-0 bg-gray-600/75" @click="sidebarOpen = false" />
      <div class="fixed inset-y-0 left-0 w-64 bg-white shadow-xl">
        <div class="p-4">
          <div class="flex items-center justify-between mb-6">
            <h2 class="text-lg font-bold text-blue-600">TeleMed</h2>
            <button @click="sidebarOpen = false" class="text-gray-400 hover:text-gray-600">
              <svg class="w-6 h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12" />
              </svg>
            </button>
          </div>
          <nav class="space-y-1">
            <NuxtLink
              v-for="item in navItems"
              :key="item.to"
              :to="item.to"
              class="block px-3 py-2 rounded-lg text-gray-700 hover:bg-blue-50 hover:text-blue-700 font-medium"
              @click="sidebarOpen = false"
            >
              {{ item.label }}
            </NuxtLink>
          </nav>
        </div>
      </div>
    </div>

    <!-- Desktop sidebar -->
    <aside class="hidden lg:fixed lg:inset-y-0 lg:flex lg:w-64 lg:flex-col">
      <div class="flex flex-col flex-grow bg-white border-r border-gray-200 pt-5 pb-4 overflow-y-auto">
        <!-- Logo -->
        <div class="flex items-center px-6 mb-8">
          <div class="w-8 h-8 rounded-lg bg-blue-600 flex items-center justify-center mr-3">
            <svg class="w-5 h-5 text-white" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                d="M4.318 6.318a4.5 4.5 0 000 6.364L12 20.364l7.682-7.682a4.5 4.5 0 00-6.364-6.364L12 7.636l-1.318-1.318a4.5 4.5 0 00-6.364 0z" />
            </svg>
          </div>
          <span class="text-xl font-bold text-gray-900">TeleMed</span>
        </div>

        <!-- Navigation -->
        <nav class="flex-1 px-4 space-y-1">
          <NuxtLink
            v-for="item in navItems"
            :key="item.to"
            :to="item.to"
            class="flex items-center px-3 py-2.5 rounded-lg text-gray-700 hover:bg-blue-50 hover:text-blue-700 font-medium transition"
            active-class="bg-blue-50 text-blue-700"
          >
            {{ item.label }}
          </NuxtLink>
        </nav>

        <!-- User info -->
        <div class="px-4 pt-4 border-t border-gray-200">
          <div class="flex items-center px-3 py-2">
            <div class="w-8 h-8 rounded-full bg-blue-100 flex items-center justify-center mr-3">
              <span class="text-sm font-medium text-blue-600">{{ userName?.charAt(0)?.toUpperCase() || '?' }}</span>
            </div>
            <div class="flex-1 min-w-0">
              <p class="text-sm font-medium text-gray-900 truncate">{{ userName }}</p>
              <p class="text-xs text-gray-500 capitalize">{{ userRole }}</p>
            </div>
          </div>
          <button
            @click="handleLogout"
            class="w-full mt-2 px-3 py-2 text-sm text-gray-600 hover:text-red-600 hover:bg-red-50 rounded-lg transition text-left"
          >
            Sign Out
          </button>
        </div>
      </div>
    </aside>

    <!-- Main content -->
    <div class="lg:pl-64">
      <!-- Top bar -->
      <header class="sticky top-0 z-30 bg-white border-b border-gray-200">
        <div class="flex items-center justify-between px-4 py-3 sm:px-6">
          <!-- Mobile menu button -->
          <button @click="sidebarOpen = true" class="lg:hidden text-gray-600 hover:text-gray-900">
            <svg class="w-6 h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 6h16M4 12h16M4 18h16" />
            </svg>
          </button>

          <div class="flex-1" />

          <!-- Alert badge -->
          <div v-if="unacknowledgedCount > 0" class="relative mr-4">
            <div class="flex items-center text-red-600">
              <svg class="w-6 h-6 animate-pulse" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                  d="M15 17h5l-1.405-1.405A2.032 2.032 0 0118 14.158V11a6.002 6.002 0 00-4-5.659V5a2 2 0 10-4 0v.341C7.67 6.165 6 8.388 6 11v3.159c0 .538-.214 1.055-.595 1.436L4 17h5m6 0v1a3 3 0 11-6 0v-1m6 0H9" />
              </svg>
              <span class="absolute -top-1 -right-1 bg-red-500 text-white text-xs rounded-full w-5 h-5 flex items-center justify-center">
                {{ unacknowledgedCount }}
              </span>
            </div>
          </div>

          <!-- Mobile user name -->
          <span class="lg:hidden text-sm font-medium text-gray-700">{{ userName }}</span>
        </div>
      </header>

      <!-- Alert Banner -->
      <AlertBanner />

      <!-- Page content -->
      <main class="p-4 sm:p-6">
        <slot />
      </main>
    </div>
  </div>
</template>
