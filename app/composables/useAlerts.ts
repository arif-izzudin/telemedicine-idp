import { ref, computed } from 'vue'
import {
  collection,
  query,
  where,
  orderBy,
  limit,
  onSnapshot,
  addDoc,
  updateDoc,
  doc,
  serverTimestamp,
  type Unsubscribe,
} from 'firebase/firestore'
import { useFirestore } from 'vuefire'
import { useAuth } from './useAuth'

export interface Alert {
  id: string
  patientId: string
  patientName: string
  type: string
  value: number
  threshold: number
  message: string
  alertLevel: 'warning' | 'critical'
  timestamp: any
  acknowledged: boolean
  acknowledgedBy: string | null
}

export interface Reading {
  id: string
  heartRate: number
  spo2: number
  temperature: number
  alertLevel: string
  deviceId: string
  timestamp: any
}

const alerts = ref<Alert[]>([])
const unacknowledgedCount = computed(
  () => alerts.value.filter((a) => !a.acknowledged).length
)

let alertUnsubscribe: Unsubscribe | null = null

export function useAlerts() {
  const db = useFirestore()
  const { userId, userRole, userProfile } = useAuth()

  function subscribeToAlerts(patientId?: string) {
    if (alertUnsubscribe) alertUnsubscribe()

    const alertsRef = collection(db, 'alerts')
    let q

    if (patientId) {
      q = query(
        alertsRef,
        where('patientId', '==', patientId),
        orderBy('timestamp', 'desc'),
        limit(50)
      )
    } else if (userRole.value === 'patient') {
      q = query(
        alertsRef,
        where('patientId', '==', userId.value),
        orderBy('timestamp', 'desc'),
        limit(50)
      )
    } else {
      // Caregivers/doctors: get alerts for all linked patients
      const linked = userProfile.value?.linkedPatients || []
      if (linked.length === 0) {
        alerts.value = []
        return
      }
      q = query(
        alertsRef,
        where('patientId', 'in', linked),
        orderBy('timestamp', 'desc'),
        limit(50)
      )
    }

    alertUnsubscribe = onSnapshot(q, (snapshot) => {
      const newAlerts: Alert[] = []
      snapshot.forEach((docSnap) => {
        newAlerts.push({ id: docSnap.id, ...docSnap.data() } as Alert)
      })

      // Check for new critical alerts for browser notification
      const prevIds = new Set(alerts.value.map((a) => a.id))
      for (const alert of newAlerts) {
        if (!prevIds.has(alert.id) && alert.alertLevel === 'critical' && !alert.acknowledged) {
          sendBrowserNotification(alert)
        }
      }

      alerts.value = newAlerts
    })
  }

  async function createAlert(reading: Reading, patientName: string) {
    const types: string[] = []
    const messages: string[] = []

    if (reading.heartRate > 0) {
      if (reading.heartRate < 50) {
        types.push('low_heart_rate')
        messages.push(`Low heart rate: ${reading.heartRate} BPM (threshold: 50)`)
      } else if (reading.heartRate > 120) {
        types.push('high_heart_rate')
        messages.push(`High heart rate: ${reading.heartRate} BPM (threshold: 120)`)
      }
    }

    if (reading.spo2 > 0 && reading.spo2 < 95) {
      types.push('low_spo2')
      messages.push(`Low SpO2: ${reading.spo2}% (threshold: 95%)`)
    }

    if (types.length === 0) return

    const alertType = types[0]!
    const alertDoc = {
      patientId: userId.value,
      patientName,
      type: alertType,
      value: alertType.includes('heart') ? reading.heartRate : reading.spo2,
      threshold: alertType === 'low_heart_rate' ? 50 : alertType === 'high_heart_rate' ? 120 : 95,
      message: messages.join('; '),
      alertLevel: reading.alertLevel === 'critical' ? 'critical' : 'warning',
      timestamp: serverTimestamp(),
      acknowledged: false,
      acknowledgedBy: null,
    }

    await addDoc(collection(db, 'alerts'), alertDoc)
  }

  async function acknowledgeAlert(alertId: string) {
    await updateDoc(doc(db, 'alerts', alertId), {
      acknowledged: true,
      acknowledgedBy: userId.value,
    })
  }

  function sendBrowserNotification(alert: Alert) {
    if (!('Notification' in window)) return
    if (Notification.permission !== 'granted') return

    new Notification(`${alert.alertLevel.toUpperCase()} Alert`, {
      body: alert.message,
      icon: '/favicon.ico',
      tag: alert.id,
    })
  }

  function requestNotificationPermission() {
    if ('Notification' in window && Notification.permission === 'default') {
      Notification.requestPermission()
    }
  }

  function unsubscribe() {
    if (alertUnsubscribe) {
      alertUnsubscribe()
      alertUnsubscribe = null
    }
  }

  return {
    alerts,
    unacknowledgedCount,
    subscribeToAlerts,
    createAlert,
    acknowledgeAlert,
    requestNotificationPermission,
    unsubscribe,
  }
}
