// https://nuxt.com/docs/api/configuration/nuxt-config
export default defineNuxtConfig({
  compatibilityDate: '2025-07-15',
  devtools: { enabled: true },
  ssr: false, // SPA mode — Firestore real-time listeners are client-side

  modules: [
    '@nuxtjs/tailwindcss',
    'nuxt-vuefire',
  ],

  vuefire: {
    config: {
      apiKey: 'AIzaSyBjdQ67Naew1jNhzlYZcye7NL5shiCCkfQ',
      authDomain: 'telemedicine-idp.firebaseapp.com',
      projectId: 'telemedicine-idp',
      storageBucket: 'telemedicine-idp.firebasestorage.app',
      messagingSenderId: '950612160792',
      appId: '1:950612160792:web:33c62fabebc4fd21d7ee6d',
      measurementId: 'G-441E47BVB6',
    },
    auth: {
      enabled: true,
    },
  },

  tailwindcss: {
    cssPath: '~/assets/css/tailwind.css',
  },

  nitro: {
    prerender: {
      crawlLinks: true,
      routes: ['/sitemap.xml', '/robots.txt'],
      ignore: [],
    },
  },
})
