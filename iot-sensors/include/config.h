#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// WiFi Configuration
// ============================================================
#define WIFI_SSID     "Rumah BERET"
#define WIFI_PASSWORD "k123456789abc"

// ============================================================
// Firebase Configuration
// ============================================================
#define FIREBASE_API_KEY      "AIzaSyBjdQ67Naew1jNhzlYZcye7NL5shiCCkfQ"
#define FIREBASE_PROJECT_ID   "telemedicine-idp"
#define FIREBASE_AUTH_DOMAIN  "telemedicine-idp.firebaseapp.com"

// Device authenticates as a patient account (Email/Password)
// 1. Register a patient on the web app
// 2. Copy the email, password, and UID here
#define DEVICE_EMAIL    "arif.zainalalam@gmail.com"
#define DEVICE_PASSWORD "k123456789abc"

// ============================================================
// Device Configuration
// ============================================================
#define DEVICE_ID   "esp32-001"
// Copy the patient's UID from Firebase Console → Authentication → Users
#define PATIENT_UID "JxZOLwnl31abLnhKGMalKb5g6By1"

// ============================================================
// Sensor & Alert Configuration
// ============================================================
#define READING_INTERVAL_MS  10000  // Send reading every 10 seconds

// Heart rate thresholds (BPM)
#define HR_CRITICAL_LOW   50
#define HR_CRITICAL_HIGH  120

// SpO2 thresholds (%)
#define SPO2_CRITICAL     90
#define SPO2_WARNING      95

// ============================================================
// Hardware Pin Configuration (ESP32-Dev Module)
// ============================================================
#define BUZZER_PIN  26
#define LED_RED_PIN 27

// MAX30102 uses default I2C pins:
// SDA = GPIO 21
// SCL = GPIO 22

// DS18B20 (future) — OneWire pin
// #define DS18B20_PIN 4

#endif // CONFIG_H
