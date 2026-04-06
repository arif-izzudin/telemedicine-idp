<script setup lang="ts">
import {
  collection,
  query,
  orderBy,
  limit,
  onSnapshot,
  doc,
  getDoc,
  type Unsubscribe,
} from 'firebase/firestore'
import { useFirestore } from 'vuefire'

const db = useFirestore()
const { userId, userRole, userProfile, userName } = useAuth()
const { subscribeToAlerts, alerts } = useAlerts()

// Patient's own readings (for patient role)
const readings = ref<any[]>([])
const latestReading = ref<any>(null)
let readingsUnsubscribe: Unsubscribe | null = null

// Linked patients (for caregiver/doctor role)
const linkedPatients = ref<any[]>([])

// Watch for auth to be ready before subscribing
watch(
  [userId, userRole],
  ([uid, role]) => {
    // Clean up previous subscription
    if (readingsUnsubscribe) {
      readingsUnsubscribe()
      readingsUnsubscribe = null
    }

    if (!uid || !role) return

    if (role === 'patient') {
      subscribeToOwnReadings()
    } else {
      loadLinkedPatients()
    }
    subscribeToAlerts()
  },
  { immediate: true }
)

onUnmounted(() => {
  if (readingsUnsubscribe) readingsUnsubscribe()
})

function subscribeToOwnReadings() {
  if (!userId.value) return

  const readingsRef = collection(db, 'users', userId.value, 'readings')
  const q = query(readingsRef, orderBy('timestamp', 'desc'), limit(50))

  readingsUnsubscribe = onSnapshot(q, (snapshot) => {
    readings.value = snapshot.docs.map((d) => ({ id: d.id, ...d.data() }))
    latestReading.value = readings.value[0] || null
  })
}

async function loadLinkedPatients() {
  const linked = userProfile.value?.linkedPatients || []
  const patients: any[] = []

  for (const patientId of linked) {
    try {
      const patientDoc = await getDoc(doc(db, 'users', patientId))
      if (patientDoc.exists()) {
        const data = patientDoc.data()

        // Fetch latest reading for this patient
        const readingsRef = collection(db, 'users', patientId, 'readings')
        const q = query(readingsRef, orderBy('timestamp', 'desc'), limit(1))

        // One-time fetch for the list view
        const { getDocs } = await import('firebase/firestore')
        const snap = await getDocs(q)
        const latestRead = snap.docs[0]?.data() || null

        patients.push({
          id: patientId,
          name: data.name,
          latestReading: latestRead,
        })
      }
    } catch (e) {
      console.error('Error loading patient:', patientId, e)
    }
  }

  linkedPatients.value = patients
}

function getAlertLevel(hr: number, spo2: number): string {
  if (hr <= 0 || spo2 <= 0) return 'normal'
  if (hr < 50 || hr > 120 || spo2 < 90) return 'critical'
  if (spo2 < 95) return 'warning'
  return 'normal'
}

const hrAlertLevel = computed(() => {
  if (!latestReading.value) return 'normal'
  const hr = latestReading.value.heartRate
  if (hr < 50 || hr > 120) return 'critical'
  return 'normal'
})

const spo2AlertLevel = computed(() => {
  if (!latestReading.value) return 'normal'
  const spo2 = latestReading.value.spo2
  if (spo2 > 0 && spo2 < 90) return 'critical'
  if (spo2 > 0 && spo2 < 95) return 'warning'
  return 'normal'
})

const recentAlerts = computed(() => alerts.value.slice(0, 5))
</script>

<template>
  <div>
    <div class="mb-6">
      <h1 class="text-2xl font-bold text-gray-900">Dashboard</h1>
      <p class="text-gray-500 mt-1">
        Welcome back, {{ userName }}
        <span class="text-xs bg-gray-100 text-gray-600 px-2 py-0.5 rounded-full ml-2 capitalize">{{ userRole }}</span>
      </p>
    </div>

    <!-- Patient View -->
    <div v-if="userRole === 'patient'">
      <!-- Current Vitals -->
      <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4 mb-6">
        <VitalCard
          label="Heart Rate"
          :value="latestReading?.heartRate || 0"
          unit="BPM"
          :alert-level="hrAlertLevel"
        />
        <VitalCard
          label="Blood Oxygen (SpO2)"
          :value="latestReading?.spo2 || 0"
          unit="%"
          :alert-level="spo2AlertLevel"
        />
        <VitalCard
          label="Temperature"
          :value="latestReading?.temperature > 0 ? latestReading.temperature : 0"
          unit="°C"
          alert-level="normal"
        />
      </div>

      <!-- Chart -->
      <div class="mb-6">
        <VitalChart :readings="readings" />
      </div>

      <!-- Recent Alerts -->
      <div v-if="recentAlerts.length > 0" class="bg-white rounded-xl border border-gray-200 p-5">
        <h3 class="text-sm font-medium text-gray-500 mb-4">Recent Alerts</h3>
        <div class="space-y-3">
          <div
            v-for="alert in recentAlerts"
            :key="alert.id"
            :class="[
              'flex items-center justify-between p-3 rounded-lg border',
              alert.alertLevel === 'critical' ? 'bg-red-50 border-red-200' : 'bg-yellow-50 border-yellow-200'
            ]"
          >
            <div>
              <span :class="[
                'text-xs font-semibold px-2 py-0.5 rounded-full uppercase',
                alert.alertLevel === 'critical' ? 'bg-red-100 text-red-700' : 'bg-yellow-100 text-yellow-700'
              ]">{{ alert.alertLevel }}</span>
              <span class="text-sm text-gray-700 ml-2">{{ alert.message }}</span>
            </div>
            <span v-if="alert.acknowledged" class="text-xs text-green-600 font-medium">Acknowledged</span>
          </div>
        </div>
      </div>
    </div>

    <!-- Caregiver / Doctor View -->
    <div v-else>
      <div class="mb-4">
        <h2 class="text-lg font-semibold text-gray-800">Your Patients</h2>
        <p class="text-sm text-gray-500">Monitor linked patients' vital signs in real time</p>
      </div>
      <PatientList :patients="linkedPatients" />

      <!-- Recent Alerts across all patients -->
      <div v-if="recentAlerts.length > 0" class="mt-6 bg-white rounded-xl border border-gray-200 p-5">
        <h3 class="text-sm font-medium text-gray-500 mb-4">Recent Alerts (All Patients)</h3>
        <div class="space-y-3">
          <div
            v-for="alert in recentAlerts"
            :key="alert.id"
            :class="[
              'flex items-center justify-between p-3 rounded-lg border',
              alert.alertLevel === 'critical' ? 'bg-red-50 border-red-200' : 'bg-yellow-50 border-yellow-200'
            ]"
          >
            <div>
              <span :class="[
                'text-xs font-semibold px-2 py-0.5 rounded-full uppercase mr-2',
                alert.alertLevel === 'critical' ? 'bg-red-100 text-red-700' : 'bg-yellow-100 text-yellow-700'
              ]">{{ alert.alertLevel }}</span>
              <span class="text-sm font-medium text-gray-800">{{ alert.patientName }}:</span>
              <span class="text-sm text-gray-600 ml-1">{{ alert.message }}</span>
            </div>
            <div class="flex items-center gap-2">
              <span v-if="alert.acknowledged" class="text-xs text-green-600 font-medium">Acknowledged</span>
              <NuxtLink
                :to="`/patient/${alert.patientId}`"
                class="text-xs text-blue-600 hover:text-blue-700 font-medium"
              >
                View →
              </NuxtLink>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>
