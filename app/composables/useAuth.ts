import { ref, computed, onMounted } from 'vue'
import {
  getAuth,
  onAuthStateChanged,
  signInWithEmailAndPassword,
  createUserWithEmailAndPassword,
  signInWithPopup,
  GoogleAuthProvider,
  signOut,
  type User
} from 'firebase/auth'
import { doc, getDoc, setDoc, serverTimestamp } from 'firebase/firestore'
import { useFirestore } from 'vuefire'

const currentUser = ref<User | null>(null)
const userProfile = ref<Record<string, any> | null>(null)
const loading = ref(true)
const error = ref<string | null>(null)

let initialized = false

export function useAuth() {
  const db = useFirestore()

  const isAuthenticated = computed(() => !!currentUser.value)
  const userRole = computed(() => userProfile.value?.role || null)
  const userName = computed(() => userProfile.value?.name || '')
  const userId = computed(() => currentUser.value?.uid || null)

  function init() {
    if (initialized) return
    initialized = true

    const auth = getAuth()
    onAuthStateChanged(auth, async (user) => {
      currentUser.value = user
      if (user) {
        await fetchProfile(user.uid)
      } else {
        userProfile.value = null
      }
      loading.value = false
    })
  }

  async function fetchProfile(uid: string) {
    try {
      const docSnap = await getDoc(doc(db, 'users', uid))
      if (docSnap.exists()) {
        userProfile.value = { id: docSnap.id, ...docSnap.data() }
      } else {
        userProfile.value = null
      }
    } catch (e: any) {
      console.error('Error fetching profile:', e)
      userProfile.value = null
    }
  }

  async function login(email: string, password: string) {
    error.value = null
    try {
      const auth = getAuth()
      await signInWithEmailAndPassword(auth, email, password)
    } catch (e: any) {
      error.value = e.message
      throw e
    }
  }

  async function register(email: string, password: string, name: string, role: string) {
    error.value = null
    try {
      const auth = getAuth()
      const cred = await createUserWithEmailAndPassword(auth, email, password)

      // Create user profile in Firestore
      await setDoc(doc(db, 'users', cred.user.uid), {
        name,
        email,
        role,
        linkedPatients: [],
        createdAt: serverTimestamp(),
      })

      await fetchProfile(cred.user.uid)
    } catch (e: any) {
      error.value = e.message
      throw e
    }
  }

  async function loginWithGoogle() {
    error.value = null
    try {
      const auth = getAuth()
      const provider = new GoogleAuthProvider()
      const cred = await signInWithPopup(auth, provider)

      // Check if profile exists; if not, create one (default role: patient)
      const docSnap = await getDoc(doc(db, 'users', cred.user.uid))
      if (!docSnap.exists()) {
        await setDoc(doc(db, 'users', cred.user.uid), {
          name: cred.user.displayName || 'User',
          email: cred.user.email || '',
          role: 'patient',
          linkedPatients: [],
          createdAt: serverTimestamp(),
        })
      }
      await fetchProfile(cred.user.uid)
    } catch (e: any) {
      error.value = e.message
      throw e
    }
  }

  async function logout() {
    const auth = getAuth()
    await signOut(auth)
    currentUser.value = null
    userProfile.value = null
  }

  // Auto-init on first use
  if (import.meta.client) {
    init()
  }

  return {
    currentUser,
    userProfile,
    loading,
    error,
    isAuthenticated,
    userRole,
    userName,
    userId,
    login,
    loginWithGoogle,
    register,
    logout,
    fetchProfile,
  }
}
