#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "secret.h"

// ─── Pin & sensor definitions ─────────────────────────────────────────────────

#define WATER_PUMP_OFF           0
#define WATER_PUMP_FULL_CAPACITY 255

#define DHT_TYPE          DHT22
#define DHT_PIN           26
#define SOIL_HUMIDITY_PIN 34

#define SOIL_DRY 4095
#define SOIL_WET 1400

#define WATER_PUMP_PWM_PIN 32

// ─── MQTT topics ──────────────────────────────────────────────────────────────

#define GREENHOUSE_SOIL_HUMIDITY_SETPOINT "greenhouse/setpoint"
#define GREENHOUSE_SENSORS_CURRENT_QUEUE  "greenhouse/sensors"

// ─── Outbox (cache) settings ──────────────────────────────────────────────────

#define OUTBOX_MAX_SIZE  150
#define OUTBOX_TTL_MS    (2UL * 60UL * 60UL * 1000UL) // 2 hours in milliseconds

struct OutboxEntry {
  char          payload[320];
  unsigned long createdAt; // millis() timestamp when the entry was added
};

OutboxEntry outbox[OUTBOX_MAX_SIZE];
int outboxHead  = 0; // index of the oldest entry
int outboxTail  = 0; // index where the next entry will be written
int outboxCount = 0; // number of valid entries currently stored

// ─── PID & control state ──────────────────────────────────────────────────────

float soilSetpoint         = 70.0;
float lastValidTemperature = 0.0;
float lastValidHumidity    = 0.0;

float Kp_base = 2.0;
float Ki_base = 0.05;
float Kd_base = 1.0;

float T_ref = 25.0;
float alpha  = 0.03;

float pidError      = 0;
float previousError = 0;
float integral      = 0;
float derivative    = 0;
float pidOutput     = 0;

unsigned long lastPIDTime = 0;

const int pwmFrequency = 1000;
const int pwmResolution = 8;

// ─── Connectivity clients ─────────────────────────────────────────────────────

WiFiClientSecure espClient;
MQTTClient       mqttClient(512); // 512-byte buffer for messages (larger than payload to accommodate QoS 2 overhead)

DHT dht(DHT_PIN, DHT_TYPE);

// ═════════════════════════════════════════════════════════════════════════════
// Outbox helpers
// ═════════════════════════════════════════════════════════════════════════════

// Removes entries whose age exceeds OUTBOX_TTL_MS.
void outboxEvictExpired() {
  unsigned long now = millis();
  while (outboxCount > 0) {
    unsigned long age = now - outbox[outboxHead].createdAt;
    if (age > OUTBOX_TTL_MS) {
      outboxHead = (outboxHead + 1) % OUTBOX_MAX_SIZE;
      outboxCount--;
      Serial.println("[OUTBOX] Evicted expired entry (TTL exceeded)");
    } else {
      break; // entries are ordered by insertion time; stop at first valid one
    }
  }
}

// Extracts the "timestamp" field from a JSON payload string.
// Returns 0 if the field is missing or the payload is invalid.
unsigned long extractTimestamp(const char* payload) {
  StaticJsonDocument<320> doc;
  if (deserializeJson(doc, payload) != DeserializationError::Ok) return 0;
  return doc["timestamp"] | 0UL;
}

// Returns true if a payload with the same timestamp already exists in the outbox.
bool outboxIsDuplicate(const char* payload) {
  unsigned long incoming = extractTimestamp(payload);
  if (incoming == 0) return false; // cannot determine — allow it through

  // Only need to check the most recently added entry: the loop() publish block
  // runs every 2 s and always produces a fresh timestamp, so a duplicate is
  // always the immediately preceding tail entry (same message, second enqueue).
  if (outboxCount == 0) return false;

  int lastIndex = (outboxTail - 1 + OUTBOX_MAX_SIZE) % OUTBOX_MAX_SIZE;
  unsigned long lastTimestamp = extractTimestamp(outbox[lastIndex].payload);

  return incoming == lastTimestamp;
}

// Appends a new payload to the outbox. Drops the oldest entry if the buffer is full.
// Silently ignores payloads whose timestamp matches the last stored entry (deduplication).
void outboxEnqueue(const char* payload) {
  outboxEvictExpired();

  if (outboxIsDuplicate(payload)) {
    Serial.println("[OUTBOX] Duplicate detected — skipping enqueue");
    return;
  }

  if (outboxCount == OUTBOX_MAX_SIZE) {
    // Buffer full — drop oldest to make room
    outboxHead = (outboxHead + 1) % OUTBOX_MAX_SIZE;
    outboxCount--;
    Serial.println("[OUTBOX] Buffer full — oldest entry dropped");
  }

  strncpy(outbox[outboxTail].payload, payload, sizeof(outbox[outboxTail].payload) - 1);
  outbox[outboxTail].payload[sizeof(outbox[outboxTail].payload) - 1] = '\0';
  outbox[outboxTail].createdAt = millis();

  outboxTail  = (outboxTail + 1) % OUTBOX_MAX_SIZE;
  outboxCount++;

  Serial.printf("[OUTBOX] Enqueued — %d message(s) pending\n", outboxCount);
}

// Flushes all valid cached entries to the broker, then clears the outbox.
// Sends one message at a time, waiting for the QoS 2 handshake to complete
// before moving to the next. Stops (without clearing) on the first failure
// so no messages are lost.
void outboxFlush() {
  if (outboxCount == 0) return;

  outboxEvictExpired();

  Serial.printf("[OUTBOX] Flushing %d cached message(s)...\n", outboxCount);

  int flushed = 0;

  while (outboxCount > 0) {
    OutboxEntry& entry = outbox[outboxHead];

    // Allow the MQTT stack to process incoming handshake packets before publishing
    mqttClient.loop();

    bool ok = mqttClient.publish(GREENHOUSE_SENSORS_CURRENT_QUEUE, entry.payload, false, 2);

    if (!ok) {
      Serial.printf("[OUTBOX] Publish failed on message %d — aborting flush (remaining messages kept)\n", flushed + 1);
      // Do NOT clear — remaining entries stay in the outbox for the next flush
      Serial.printf("[OUTBOX] %d message(s) flushed, %d still pending\n", flushed, outboxCount);
      return;
    }

    Serial.printf("[OUTBOX] Flushed (%d/%d): %s\n", flushed + 1, flushed + outboxCount, entry.payload);
    flushed++;

    outboxHead = (outboxHead + 1) % OUTBOX_MAX_SIZE;
    outboxCount--;

    // Wait for the QoS 2 four-way handshake to complete before the next publish.
    // Without this delay the broker receives a new PUBLISH before responding to
    // the previous PUBREC, which causes rc=-3 (buffer overflow / timeout).
    if (outboxCount > 0) {
      delay(150);
      mqttClient.loop(); // process PUBREC / PUBCOMP for the message just sent
    }
  }

  // All messages sent — reset pointers
  outboxHead  = 0;
  outboxTail  = 0;
  outboxCount = 0;

  Serial.printf("[OUTBOX] Cleared. All %d message(s) flushed successfully.\n", flushed);
}

// ═════════════════════════════════════════════════════════════════════════════
// Connectivity
// ═════════════════════════════════════════════════════════════════════════════

bool isConnected() {
  return WiFi.status() == WL_CONNECTED && mqttClient.connected();
}

// Non-blocking Wi-Fi reconnection — called every loop iteration when offline.
void tryReconnectWiFi() {
  static unsigned long lastAttempt    = 0;
  static bool          connecting     = false;
  const  unsigned long RETRY_INTERVAL = 10000; // 10 s between attempts

  if (WiFi.status() == WL_CONNECTED) return;

  unsigned long now = millis();

  if (!connecting) {
    if (now - lastAttempt < RETRY_INTERVAL) return;
    lastAttempt = now;

    Serial.println("[WIFI] Attempting to connect...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(100);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    connecting = true;
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WIFI] Connected: " + WiFi.localIP().toString());
    connecting = false;
  } else if (now - lastAttempt > 15000) {
    Serial.println("[WIFI] Attempt timed out — will retry");
    connecting  = false;
    lastAttempt = now;
  }
}

// Non-blocking MQTT reconnection — called every loop iteration when Wi-Fi is up but broker is down.
void tryReconnectMQTT() {
  static unsigned long lastAttempt    = 0;
  const  unsigned long RETRY_INTERVAL = 5000; // 5 s between attempts

  if (mqttClient.connected())             return;
  if (WiFi.status() != WL_CONNECTED)     return;

  unsigned long now = millis();
  if (now - lastAttempt < RETRY_INTERVAL) return;
  lastAttempt = now;

  String clientId = "ESP32Client-" + String(random(0xffff), HEX);

  Serial.print("[MQTT] Connecting...");

  if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.println(" connected!");
    mqttClient.subscribe(GREENHOUSE_SOIL_HUMIDITY_SETPOINT, 2);

    // Connection restored — flush the outbox
    outboxFlush();
  } else {
    Serial.print(" failed, rc=");
    Serial.println(mqttClient.lastError());
  }
}

// ═════════════════════════════════════════════════════════════════════════════
// MQTT callback
// ═════════════════════════════════════════════════════════════════════════════

void mqttCallback(String& topic, String& payload) {
  StaticJsonDocument<200> doc;

  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("[MQTT] Failed to parse JSON: ");
    Serial.println(err.f_str());
    return;
  }

  if (doc.containsKey("setpoint")) {
    soilSetpoint = doc["setpoint"];
    Serial.print("[MQTT] Setpoint received: ");
    Serial.println(soilSetpoint);
  }
}

// ═════════════════════════════════════════════════════════════════════════════
// Soil humidity sensor
// ═════════════════════════════════════════════════════════════════════════════

float readCurrentSoilHumidityPercentage() {
  int raw          = analogRead(SOIL_HUMIDITY_PIN);
  int soilHumidity = constrain(raw, SOIL_WET, SOIL_DRY);
  return map(soilHumidity, SOIL_WET, SOIL_DRY, 100, 0);
}

// ═════════════════════════════════════════════════════════════════════════════
// Setup
// ═════════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);

  dht.begin();
  delay(2000);

  pinMode(SOIL_HUMIDITY_PIN, INPUT);
  ledcAttach(WATER_PUMP_PWM_PIN, pwmFrequency, pwmResolution);

  espClient.setInsecure();
  mqttClient.begin(MQTT_BROKER, MQTT_PORT, espClient);
  mqttClient.setOptions(10, true, 5000); // keepalive 10s, cleanSession, timeout 5s
  mqttClient.onMessage(mqttCallback);

  // Best-effort initial connection — offline mode activates automatically if it fails
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("[WIFI] Connecting");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] Connected: " + WiFi.localIP().toString());

    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("[MQTT] Connected!");
      mqttClient.subscribe(GREENHOUSE_SOIL_HUMIDITY_SETPOINT, 2);
    } else {
      Serial.println("[MQTT] Initial connection failed — offline mode active");
    }
  } else {
    Serial.println("\n[WIFI] Not reachable — offline mode active");
  }

  lastPIDTime = millis();
}

// ═════════════════════════════════════════════════════════════════════════════
// Loop
// ═════════════════════════════════════════════════════════════════════════════

void loop() {

  // ─── Connectivity management (non-blocking) ────────────────────────────────
  tryReconnectWiFi();
  tryReconnectMQTT();

  if (isConnected()) {
    mqttClient.loop();
  }

  // ─── Sensor readings ───────────────────────────────────────────────────────
  float currentTemperature = dht.readTemperature();
  float currentHumidity    = dht.readHumidity();

  if (isnan(currentTemperature) || isnan(currentHumidity)) {
    currentTemperature = lastValidTemperature;
    currentHumidity    = lastValidHumidity;
  } else {
    lastValidTemperature = currentTemperature;
    lastValidHumidity    = currentHumidity;
  }

  float currentSoilHumidity = readCurrentSoilHumidityPercentage();

  // ─── PID control ──────────────────────────────────────────────────────────
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - lastPIDTime) / 1000.0;

  if (deltaTime >= 1.0) {
    float thermalFactor = 1.0 + alpha * (currentTemperature - T_ref);
    thermalFactor = constrain(thermalFactor, 0.7, 1.5);

    float Kp = Kp_base * thermalFactor;
    float Ki = Ki_base * thermalFactor;
    float Kd = Kd_base * thermalFactor;

    pidError   = soilSetpoint - currentSoilHumidity;
    integral  += pidError * deltaTime;
    integral   = constrain(integral, -100, 100);
    derivative = (pidError - previousError) / deltaTime;

    pidOutput = (Kp * pidError) + (Ki * integral) + (Kd * derivative);
    pidOutput = constrain(pidOutput, WATER_PUMP_OFF, WATER_PUMP_FULL_CAPACITY);

    if (abs(pidError) < 2.0) pidOutput = 0;
    if (pidOutput > 0) pidOutput = map(pidOutput, 0, 255, 90, 255);

    ledcWrite(WATER_PUMP_PWM_PIN, (int)pidOutput);

    previousError = pidError;
    lastPIDTime   = currentTime;
  }

  // ─── Sensor publish / outbox ───────────────────────────────────────────────
  static unsigned long lastPublish = 0;

  if (millis() - lastPublish > 2000) {
    lastPublish = millis();

    Serial.printf("[AIR]  Temp: %.2f | Humidity: %.2f\n", currentTemperature, currentHumidity);
    Serial.printf("[SOIL] Humidity: %.2f\n", currentSoilHumidity);
    Serial.printf("[PID]  Setpoint: %.2f | Output: %.2f\n", soilSetpoint, pidOutput);

    // Build JSON payload
    unsigned long ts     = millis();
    String        mac    = WiFi.macAddress();
    String        momentId = String(ts) + ":" + mac;

    StaticJsonDocument<320> sensorsMessage;
    sensorsMessage["air"]["temperature"]       = currentTemperature;
    sensorsMessage["air"]["humidity"]          = currentHumidity;
    sensorsMessage["soil"]["humidity"]         = currentSoilHumidity;
    sensorsMessage["irrigation"]["pid_output"] = pidOutput;
    sensorsMessage["deviceId"]                 = mac;
    sensorsMessage["timestamp"]                = ts;
    sensorsMessage["sensor_data_moment_id"]    = momentId;

    char buffer[320];
    serializeJson(sensorsMessage, buffer);

    if (isConnected()) {
      // Online — publish directly with QoS 2
      bool ok = mqttClient.publish(GREENHOUSE_SENSORS_CURRENT_QUEUE, buffer, false, 2);

      if (ok) {
        Serial.print("[MQTT] Published (QoS 2): ");
        Serial.println(buffer);
      } else {
        // Publish failed even though connected — fall back to outbox
        Serial.println("[MQTT] Publish failed — saving to outbox");
        outboxEnqueue(buffer);
      }
    } else {
      // Offline — save to outbox
      Serial.println("[OUTBOX] Offline — saving to outbox");
      outboxEnqueue(buffer);
    }
  }
}