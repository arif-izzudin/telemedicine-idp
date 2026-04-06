<script setup lang="ts">
import {
  collection,
  query,
  orderBy,
  limit,
  where,
  onSnapshot,
  doc,
  getDoc,
  type Unsubscribe,
} from 'firebase/firestore'
import { useFirestore } from 'vuefire'

const route = useRoute()
const db = useFirestore()
const { userRole } = useAuth()
const { acknowledgeAlert } = useAlerts()

const patientId = route.params.id as string

const patient = ref<any>(null)
const readings = ref<any[]>([])
const patientAlerts = ref<any[]>([])
const latestReading = ref<any>(null)

let readingsUnsub: Unsubscribe | null = null
let alertsUnsub: Unsubscribe | null = null

onMounted(async () => {
  // Load patient profile
  const patientDoc = await getDoc(doc(db, 'users', patientId))
  if (patientDoc.exists()) {
    patient.value = { id: patientDoc.id, ...patientDoc.data() }
  }

  // Subscribe to readings
  const readingsRef = collection(db, 'users', patientId, 'readings')
  const readingsQuery = query(readingsRef, orderBy('timestamp', 'desc'), limit(50))

  readingsUnsub = onSnapshot(readingsQuery, (snapshot) => {
    readings.value = snapshot.docs.map((d) => ({ id: d.id, ...d.data() }))
    latestReading.value = readings.value[0] || null
  })

  // Subscribe to alerts for this patient
  const alertsRef = collection(db, 'alerts')
  const alertsQuery = query(
    alertsRef,
    where('patientId', '==', patientId),
    orderBy('timestamp', 'desc'),
    limit(20)
  )

  alertsUnsub = onSnapshot(alertsQuery, (snapshot) => {
    patientAlerts.value = snapshot.docs.map((d) => ({ id: d.id, ...d.data() }))
  })
})

onUnmounted(() => {
  if (readingsUnsub) readingsUnsub()
  if (alertsUnsub) alertsUnsub()
})

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

async function handleAcknowledge(alertId: string) {
  await acknowledgeAlert(alertId)
}
</script>

<template>
  <div>
    <!-- Back button + header -->
    <div class="mb-6">
      <NuxtLink to="/dashboard" class="text-sm text-blue-600 hover:text-blue-700 font-medium mb-2 inline-block">
        ← Back to Dashboard
      </NuxtLink>
      <h1 class="text-2xl font-bold text-gray-900">
        {{ patient?.name || 'Loading...' }}
      </h1>
      <p class="text-gray-500 mt-1">
        Patient ID: {{ patientId.slice(0, 12) }}...
        <span v-if="patient?.deviceId" class="ml-2 text-xs bg-gray-100 text-gray-600 px-2 py-0.5 rounded-full">
          Device: {{ patient.deviceId }}
        </span>
      </p>
    </div>

    <div v-if="patient">
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

      <!-- Alert History -->
      <div class="bg-white rounded-xl border border-gray-200 p-5">
        <h3 class="text-sm font-medium text-gray-500 mb-4">Alert History</h3>

        <div v-if="patientAlerts.length > 0" class="space-y-3">
          <div
            v-for="alert in patientAlerts"
            :key="alert.id"
            :class="[
              'flex items-center justify-between p-3 rounded-lg border',
              alert.acknowledged
                ? 'bg-gray-50 border-gray-200'
                : alert.alertLevel === 'critical'
                  ? 'bg-red-50 border-red-200'
                  : 'bg-yellow-50 border-yellow-200'
            ]"
          >
            <div class="flex-1">
              <div class="flex items-center gap-2 mb-1">
                <span :class="[
                  'text-xs font-semibold px-2 py-0.5 rounded-full uppercase',
                  alert.acknowledged
                    ? 'bg-gray-200 text-gray-600'
                    : alert.alertLevel === 'critical'
                      ? 'bg-red-100 text-red-700'
                      : 'bg-yellow-100 text-yellow-700'
                ]">{{ alert.alertLevel }}</span>
                <span class="text-xs text-gray-400">
                  {{ alert.timestamp?.toDate ? alert.timestamp.toDate().toLocaleString() : '' }}
                </span>
              </div>
              <p class="text-sm text-gray-700">{{ alert.message }}</p>
            </div>

            <div class="ml-4 flex-shrink-0">
              <span v-if="alert.acknowledged" class="text-xs text-green-600 font-medium">
                ✓ Acknowledged
              </span>
              <button
                v-else-if="userRole === 'doctor'"
                @click="handleAcknowledge(alert.id)"
                class="text-xs bg-blue-600 hover:bg-blue-700 text-white px-3 py-1.5 rounded-lg transition"
              >
                Acknowledge
              </button>
              <span v-else class="text-xs text-orange-600 font-medium">Pending</span>
            </div>
          </div>
        </div>

        <div v-else class="text-center py-6 text-gray-400">
          No alerts recorded
        </div>
      </div>
    </div>

    <!-- Loading state -->
    <div v-else class="text-center py-12 text-gray-400">
      <div class="animate-spin w-8 h-8 border-4 border-blue-500 border-t-transparent rounded-full mx-auto mb-3" />
      <p>Loading patient data...</p>
    </div>
  </div>
</template>
