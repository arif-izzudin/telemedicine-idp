// ============================================================
// Smart Home Telemedicine IoT - ESP32 Firmware
// Sensors: MAX30102 (Heart Rate + SpO2)
// Backend: Firebase Firestore
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "config.h"

// Firebase helpers
#include <addons/TokenHelper.h>

// ============================================================
// Global Objects
// ============================================================
MAX30105 particleSensor;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig fbConfig;

// ============================================================
// PBA Heart Rate Detection (SparkFun Algorithm)
// ============================================================
const byte RATE_SIZE = 4;  // Sliding average window
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute = 0;
int beatAvg = 0;

// ============================================================
// Real-time SpO2 (R-ratio method)
// ============================================================
#define SPO2_RATE_SIZE 4
float spo2Readings[SPO2_RATE_SIZE];
byte spo2Spot = 0;
int spo2Avg = 0;
bool spo2Valid = false;

// AC/DC tracking for R-ratio
float redAC = 0, redDC = 0, irAC = 0, irDC = 0;
const float ALPHA = 0.95;  // Low-pass filter coefficient
unsigned long lastFingerTime = 0;
#define FINGER_TIMEOUT_MS 5000

// ============================================================
// Timing
// ============================================================
unsigned long lastReadingTime = 0;
bool firebaseReady = false;

// ============================================================
// Function declarations
// ============================================================
void connectWiFi();
void initFirebase();
void initMAX30102();
const char* determineAlertLevel(float hr, float spo2);
void triggerLocalAlert(const char* alertLevel);
void sendToFirestore(float hr, float spo2, const char* alertLevel);
float readTemperature();
String getISO8601Timestamp();
void initNTP();

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n========================================");
  Serial.println("  Telemedicine IoT Patient Monitor");
  Serial.println("========================================");

  // Initialize alert pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_RED_PIN, LOW);

  // Connect to WiFi
  connectWiFi();

  // Sync time via NTP (required for Firestore timestamps)
  initNTP();

  // Initialize Firebase
  initFirebase();

  // Initialize MAX30102 sensor
  initMAX30102();

  Serial.println("[OK] System ready. Monitoring started...\n");
}

// ============================================================
// Main Loop — PBA beat detection + real-time R-ratio SpO2
// ============================================================
void loop() {
  // Keep Firebase token fresh
  if (Firebase.ready() && Firebase.isTokenExpired()) {
    Firebase.refreshToken(&fbConfig);
  }

  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  if (irValue > 50000) {
    // Finger detected
    lastFingerTime = millis();

    // ---- PBA beat detection (SparkFun algorithm) ----
    if (checkForBeat(irValue) == true) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;

        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }

    // ---- Real-time SpO2 via R-ratio ----
    // Low-pass filter to extract DC component
    redDC = (ALPHA * redDC) + ((1.0 - ALPHA) * redValue);
    irDC  = (ALPHA * irDC)  + ((1.0 - ALPHA) * irValue);

    // AC component = current sample minus DC
    float redACsample = redValue - redDC;
    float irACsample  = irValue - irDC;

    // Running RMS-like magnitude for AC
    redAC = (ALPHA * redAC) + ((1.0 - ALPHA) * fabs(redACsample));
    irAC  = (ALPHA * irAC)  + ((1.0 - ALPHA) * fabs(irACsample));

    // Calculate SpO2 on each beat detection (when signal is stable)
    if (checkForBeat(redValue) && irDC > 0 && redDC > 0 && irAC > 0) {
      float R = (redAC / redDC) / (irAC / irDC);
      // Empirical SpO2 formula (standard pulse oximetry approximation)
      float spo2Calc = 110.0 - 25.0 * R;

      if (spo2Calc >= 70 && spo2Calc <= 100) {
        spo2Readings[spo2Spot++] = spo2Calc;
        spo2Spot %= SPO2_RATE_SIZE;

        float sum = 0;
        for (byte x = 0; x < SPO2_RATE_SIZE; x++)
          sum += spo2Readings[x];
        spo2Avg = (int)(sum / SPO2_RATE_SIZE);
        spo2Valid = true;
      }
    }
  }

  // ---- Periodic reporting ----
  unsigned long now = millis();
  if (now - lastReadingTime >= READING_INTERVAL_MS) {
    lastReadingTime = now;

    bool fingerPresent = (millis() - lastFingerTime < FINGER_TIMEOUT_MS);
    float reportHR = (fingerPresent && beatAvg > 0) ? (float)beatAvg : 0;
    float reportSpo2 = (fingerPresent && spo2Valid) ? (float)spo2Avg : 0;

    const char* alertLevel = determineAlertLevel(reportHR, reportSpo2);

    Serial.printf("[DATA] HR: %.0f BPM | SpO2: %.0f%% | Alert: %s\n",
                  reportHR, reportSpo2, alertLevel);

    if (!fingerPresent) {
      Serial.println("[Sensor] No finger detected");
    }

    triggerLocalAlert(alertLevel);

    if (Firebase.ready()) {
      sendToFirestore(reportHR, reportSpo2, alertLevel);
    } else {
      Serial.println("[WARN] Firebase not ready, skipping upload");
    }
  }
}

// ============================================================
// WiFi Connection
// ============================================================
void connectWiFi() {
  Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WiFi] FAILED to connect. Restarting...");
    ESP.restart();
  }
}

// ============================================================
// Firebase Initialization
// ============================================================
void initFirebase() {
  Serial.println("[Firebase] Initializing...");

  fbConfig.api_key = FIREBASE_API_KEY;

  // Use anonymous auth if no device credentials set
  if (strlen(DEVICE_EMAIL) == 0) {
    Serial.println("[Firebase] Using anonymous authentication");
    auth.user.email = "";
    auth.user.password = "";
    fbConfig.signer.anonymous = true;
  } else {
    auth.user.email = DEVICE_EMAIL;
    auth.user.password = DEVICE_PASSWORD;
  }

  fbConfig.token_status_callback = tokenStatusCallback;

  Firebase.begin(&fbConfig, &auth);
  Firebase.reconnectNetwork(true);

  // Wait for token
  Serial.print("[Firebase] Authenticating");
  unsigned long tokenStart = millis();
  while (!Firebase.ready() && (millis() - tokenStart < 15000)) {
    Serial.print(".");
    delay(300);
  }

  if (Firebase.ready()) {
    Serial.println("\n[Firebase] Authenticated successfully");
    firebaseReady = true;
  } else {
    Serial.println("\n[Firebase] Auth failed — will retry in loop");
  }
}

// ============================================================
// MAX30102 Initialization
// ============================================================
void initMAX30102() {
  Serial.println("[Sensor] Initializing MAX30102...");

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("[Sensor] MAX30102 not found! Check wiring.");
    Serial.println("[Sensor] Continuing without sensor (demo mode)...");
    return;
  }

  // Use default settings (matches working Instructables config)
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);  // Low red LED to indicate running
  particleSensor.setPulseAmplitudeGreen(0);   // Turn off green LED

  Serial.println("[Sensor] MAX30102 initialized OK");
}



// ============================================================
// Determine Alert Level
// ============================================================
const char* determineAlertLevel(float hr, float spo2) {
  if (hr <= 0 || spo2 <= 0) return "normal"; // No valid data yet

  if (hr < HR_CRITICAL_LOW || hr > HR_CRITICAL_HIGH || spo2 < SPO2_CRITICAL) {
    return "critical";
  }
  if (spo2 < SPO2_WARNING) {
    return "warning";
  }
  return "normal";
}

#define BUZZER_CHANNEL 0
#define BUZZER_RESOLUTION 8

// ============================================================
// Local Alert (Buzzer + LED)
// ============================================================
void triggerLocalAlert(const char* alertLevel) {
  if (strcmp(alertLevel, "critical") == 0) {
    // Critical: buzzer ON, LED ON
    digitalWrite(LED_RED_PIN, HIGH);
    ledcSetup(BUZZER_CHANNEL, 2000, BUZZER_RESOLUTION);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    ledcWriteTone(BUZZER_CHANNEL, 2000); // 2kHz tone
    delay(500);
    ledcWriteTone(BUZZER_CHANNEL, 0);    // Stop tone
    ledcDetachPin(BUZZER_PIN);
    Serial.println("[ALERT] CRITICAL \xe2\x80\x94 Buzzer + LED activated");
  } else if (strcmp(alertLevel, "warning") == 0) {
    // Warning: LED ON only (no buzzer)
    digitalWrite(LED_RED_PIN, HIGH);
    Serial.println("[ALERT] WARNING \xe2\x80\x94 LED activated");
  } else {
    // Normal: all off
    digitalWrite(LED_RED_PIN, LOW);
  }
}

// ============================================================
// Send Reading to Firestore
// ============================================================
void sendToFirestore(float hr, float spo2, const char* alertLevel) {
  String documentPath = "users/" + String(PATIENT_UID) + "/readings";

  FirebaseJson content;
  content.set("fields/heartRate/doubleValue", hr);
  content.set("fields/spo2/doubleValue", spo2);
  content.set("fields/temperature/doubleValue", readTemperature());
  content.set("fields/alertLevel/stringValue", alertLevel);
  content.set("fields/deviceId/stringValue", DEVICE_ID);
  content.set("fields/timestamp/timestampValue", getISO8601Timestamp());

  if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID,
                                         "", documentPath.c_str(),
                                         content.raw())) {
    Serial.println("[Firestore] Reading uploaded OK");
  } else {
    Serial.printf("[Firestore] Upload failed: %s\n", fbdo.errorReason().c_str());
  }
}

// ============================================================
// NTP Time Sync
// ============================================================
void initNTP() {
  Serial.print("[NTP] Syncing time");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov"); // UTC (no offset)
  struct tm timeinfo;
  int retries = 0;
  while (!getLocalTime(&timeinfo) && retries < 20) {
    Serial.print(".");
    delay(500);
    retries++;
  }
  if (retries < 20) {
    Serial.println("\n[NTP] Time synced (UTC): " + String(asctime(&timeinfo)));
  } else {
    Serial.println("\n[NTP] Failed to sync — timestamps may be wrong");
  }
}

String getISO8601Timestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "1970-01-01T00:00:00Z";
  }
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(buf);
}

// ============================================================
// Temperature Stub (DS18B20 — future implementation)
// ============================================================
float readTemperature() {
  // TODO: Implement DS18B20 reading when sensor is connected
  // For now, return -1 to indicate no temperature sensor
  return -1.0;
}