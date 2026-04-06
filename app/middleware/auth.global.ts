export default defineNuxtRouteMiddleware((to) => {
  const { isAuthenticated, loading } = useAuth()

  // Don't redirect while auth is still loading
  if (loading.value) return

  const publicPages = ['/login', '/register']
  const isPublicPage = publicPages.includes(to.path)

  if (!isAuthenticated.value && !isPublicPage) {
    return navigateTo('/login')
  }

  if (isAuthenticated.value && isPublicPage) {
    return navigateTo('/dashboard')
  }
})
