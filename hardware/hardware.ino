#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "secret.h"

// ─── Pin & sensor definitions ─────────────────────────────────────────────────

#define WATER_PUMP_OFF           0
#define WATER_PUMP_FULL_CAPACITY 255

#define FAN_OFF                  0
#define FAN_FULL_CAPACITY        255

#define DHT_TYPE          DHT22
#define DHT_PIN           26
#define SOIL_HUMIDITY_PIN 34

#define SOIL_DRY 4095
#define SOIL_WET 1400

#define WATER_PUMP_PWM_PIN 32
#define FAN_PWM_PIN        33

// ─── MQTT topics ──────────────────────────────────────────────────────────────

#define GREENHOUSE_SOIL_HUMIDITY_SETPOINT "greenhouse/setpoint/soil"
#define GREENHOUSE_AIR_TEMPERATURE_SETPOINT "greenhouse/setpoint/air/temperature"
#define GREENHOUSE_SENSORS_CURRENT_QUEUE  "greenhouse/sensors"

// ─── Outbox settings ──────────────────────────────────────────────────────────

#define OUTBOX_MAX_SIZE  150
#define OUTBOX_TTL_MS    (2UL * 60UL * 60UL * 1000UL)

struct OutboxEntry {
  char payload[320];
  unsigned long createdAt;
};

OutboxEntry outbox[OUTBOX_MAX_SIZE];
int outboxHead  = 0;
int outboxTail  = 0;
int outboxCount = 0;

// ─── PID & control state ──────────────────────────────────────────────────────

float soilSetpoint           = 70.0;
float airTemperatureSetpoint = 23.0;

float lastValidTemperature   = 0.0;
float lastValidHumidity      = 0.0;

// PID da bomba d'água
float Kp_base = 2.0;
float Ki_base = 0.05;
float Kd_base = 1.0;

float T_ref = 25.0;
float alpha = 0.03;

float pidError      = 0;
float previousError = 0;
float integral      = 0;
float derivative    = 0;
float pidOutput     = 0;

unsigned long lastPIDTime = 0;

// PID do ventilador
float fanKp = 10.0;
float fanKi = 0.2;
float fanKd = 3.0;

float fanError         = 0;
float fanPreviousError = 0;
float fanIntegral      = 0;
float fanDerivative    = 0;
float fanPidOutput     = 0;

unsigned long lastFanPIDTime = 0;

const int pwmFrequency  = 1000;
const int pwmResolution = 8;

// ─── Connectivity clients ─────────────────────────────────────────────────────

WiFiClientSecure espClient;
MQTTClient mqttClient(512);

DHT dht(DHT_PIN, DHT_TYPE);

// ═════════════════════════════════════════════════════════════════════════════
// Outbox helpers
// ═════════════════════════════════════════════════════════════════════════════

void outboxEvictExpired() {
  unsigned long now = millis();

  while (outboxCount > 0) {
    unsigned long age = now - outbox[outboxHead].createdAt;

    if (age > OUTBOX_TTL_MS) {
      outboxHead = (outboxHead + 1) % OUTBOX_MAX_SIZE;
      outboxCount--;
      Serial.println("[OUTBOX] Evicted expired entry");
    } else {
      break;
    }
  }
}

unsigned long extractTimestamp(const char* payload) {
  StaticJsonDocument<320> doc;

  if (deserializeJson(doc, payload) != DeserializationError::Ok) {
    return 0;
  }

  return doc["timestamp"] | 0UL;
}

bool outboxIsDuplicate(const char* payload) {
  unsigned long incoming = extractTimestamp(payload);

  if (incoming == 0) return false;
  if (outboxCount == 0) return false;

  int lastIndex = (outboxTail - 1 + OUTBOX_MAX_SIZE) % OUTBOX_MAX_SIZE;
  unsigned long lastTimestamp = extractTimestamp(outbox[lastIndex].payload);

  return incoming == lastTimestamp;
}

void outboxEnqueue(const char* payload) {
  outboxEvictExpired();

  if (outboxIsDuplicate(payload)) {
    Serial.println("[OUTBOX] Duplicate detected — skipping enqueue");
    return;
  }

  if (outboxCount == OUTBOX_MAX_SIZE) {
    outboxHead = (outboxHead + 1) % OUTBOX_MAX_SIZE;
    outboxCount--;
    Serial.println("[OUTBOX] Buffer full — oldest entry dropped");
  }

  strncpy(outbox[outboxTail].payload, payload, sizeof(outbox[outboxTail].payload) - 1);
  outbox[outboxTail].payload[sizeof(outbox[outboxTail].payload) - 1] = '\0';
  outbox[outboxTail].createdAt = millis();

  outboxTail = (outboxTail + 1) % OUTBOX_MAX_SIZE;
  outboxCount++;

  Serial.printf("[OUTBOX] Enqueued — %d message(s) pending\n", outboxCount);
}

void outboxFlush() {
  if (outboxCount == 0) return;

  outboxEvictExpired();

  Serial.printf("[OUTBOX] Flushing %d cached message(s)...\n", outboxCount);

  int flushed = 0;

  while (outboxCount > 0) {
    OutboxEntry& entry = outbox[outboxHead];

    mqttClient.loop();

    bool ok = mqttClient.publish(GREENHOUSE_SENSORS_CURRENT_QUEUE, entry.payload, false, 2);

    if (!ok) {
      Serial.printf("[OUTBOX] Publish failed on message %d — aborting flush\n", flushed + 1);
      Serial.printf("[OUTBOX] %d message(s) flushed, %d still pending\n", flushed, outboxCount);
      return;
    }

    Serial.printf("[OUTBOX] Flushed (%d): %s\n", flushed + 1, entry.payload);
    flushed++;

    outboxHead = (outboxHead + 1) % OUTBOX_MAX_SIZE;
    outboxCount--;

    if (outboxCount > 0) {
      delay(150);
      mqttClient.loop();
    }
  }

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

void tryReconnectWiFi() {
  static unsigned long lastAttempt = 0;
  static bool connecting = false;

  const unsigned long RETRY_INTERVAL = 10000;

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
    connecting = false;
    lastAttempt = now;
  }
}

void tryReconnectMQTT() {
  static unsigned long lastAttempt = 0;

  const unsigned long RETRY_INTERVAL = 5000;

  if (mqttClient.connected()) return;
  if (WiFi.status() != WL_CONNECTED) return;

  unsigned long now = millis();

  if (now - lastAttempt < RETRY_INTERVAL) return;

  lastAttempt = now;

  String clientId = "ESP32Client-" + String(random(0xffff), HEX);

  Serial.print("[MQTT] Connecting...");

  if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.println(" connected!");

    mqttClient.subscribe(GREENHOUSE_SOIL_HUMIDITY_SETPOINT, 2);
    mqttClient.subscribe(GREENHOUSE_AIR_TEMPERATURE_SETPOINT, 2);

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

  if (doc.containsKey("soilSetpoint")) {
    soilSetpoint = doc["soilSetpoint"];
    Serial.print("[MQTT] Soil setpoint received: ");
    Serial.println(soilSetpoint);
  }

  if (doc.containsKey("airTemperatureSetpoint")) {
    airTemperatureSetpoint = doc["airTemperatureSetpoint"];
    Serial.print("[MQTT] Air temperature setpoint received: ");
    Serial.println(airTemperatureSetpoint);
  }
}

// ═════════════════════════════════════════════════════════════════════════════
// Soil humidity sensor
// ═════════════════════════════════════════════════════════════════════════════

float readCurrentSoilHumidityPercentage() {
  int raw = analogRead(SOIL_HUMIDITY_PIN);

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
  ledcAttach(FAN_PWM_PIN, pwmFrequency, pwmResolution);

  espClient.setInsecure();

  mqttClient.begin(MQTT_BROKER, MQTT_PORT, espClient);
  mqttClient.setOptions(10, true, 5000);
  mqttClient.onMessage(mqttCallback);

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
      mqttClient.subscribe(GREENHOUSE_AIR_TEMPERATURE_SETPOINT, 2);
    } else {
      Serial.println("[MQTT] Initial connection failed — offline mode active");
    }
  } else {
    Serial.println("\n[WIFI] Not reachable — offline mode active");
  }

  lastPIDTime = millis();
  lastFanPIDTime = millis();
}

// ═════════════════════════════════════════════════════════════════════════════
// Loop
// ═════════════════════════════════════════════════════════════════════════════

void loop() {
  // ─── Connectivity management ───────────────────────────────────────────────
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

  // ─── Water pump PID control based on soil humidity ─────────────────────────
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - lastPIDTime) / 1000.0;

  if (deltaTime >= 1.0) {
    float thermalFactor = 1.0 + alpha * (currentTemperature - T_ref);
    thermalFactor = constrain(thermalFactor, 0.7, 1.5);

    float Kp = Kp_base * thermalFactor;
    float Ki = Ki_base * thermalFactor;
    float Kd = Kd_base * thermalFactor;

    pidError = soilSetpoint - currentSoilHumidity;

    integral += pidError * deltaTime;
    integral = constrain(integral, -100, 100);

    derivative = (pidError - previousError) / deltaTime;

    pidOutput = (Kp * pidError) + (Ki * integral) + (Kd * derivative);
    pidOutput = constrain(pidOutput, WATER_PUMP_OFF, WATER_PUMP_FULL_CAPACITY);

    if (abs(pidError) < 2.0) {
      pidOutput = 0;
    }

    if (pidOutput > 0) {
      pidOutput = map(pidOutput, 0, 255, 90, 255);
    }

    ledcWrite(WATER_PUMP_PWM_PIN, (int)pidOutput);

    previousError = pidError;
    lastPIDTime   = currentTime;
  }

  // ─── Fan PID control based on air temperature ──────────────────────────────
  unsigned long currentFanTime = millis();
  float fanDeltaTime = (currentFanTime - lastFanPIDTime) / 1000.0;

  if (fanDeltaTime >= 1.0) {
    // Erro positivo significa que a temperatura está acima da desejada
    fanError = currentTemperature - airTemperatureSetpoint;

    fanIntegral += fanError * fanDeltaTime;
    fanIntegral = constrain(fanIntegral, -100, 100);

    fanDerivative = (fanError - fanPreviousError) / fanDeltaTime;

    fanPidOutput = (fanKp * fanError) +
                   (fanKi * fanIntegral) +
                   (fanKd * fanDerivative);

    fanPidOutput = constrain(fanPidOutput, FAN_OFF, FAN_FULL_CAPACITY);

    // Se estiver abaixo ou muito próximo da temperatura desejada, desliga
    if (fanError < 0.5) {
      fanPidOutput = 0;
    }

    // Evita PWM muito baixo, que pode não ser suficiente para girar o ventilador
    if (fanPidOutput > 0) {
      fanPidOutput = map(fanPidOutput, 0, 255, 90, 255);
    }

    ledcWrite(FAN_PWM_PIN, (int)fanPidOutput);

    fanPreviousError = fanError;
    lastFanPIDTime   = currentFanTime;
  }

  // ─── Sensor publish / outbox ───────────────────────────────────────────────
  static unsigned long lastPublish = 0;

  if (millis() - lastPublish > 2000) {
    lastPublish = millis();

    Serial.printf("[AIR]  Temp: %.2f | Humidity: %.2f\n", currentTemperature, currentHumidity);
    Serial.printf("[SOIL] Humidity: %.2f\n", currentSoilHumidity);
    Serial.printf("[PUMP] Setpoint: %.2f | Output: %.2f\n", soilSetpoint, pidOutput);
    Serial.printf("[FAN]  Setpoint: %.2f | Output: %.2f\n", airTemperatureSetpoint, fanPidOutput);

    unsigned long ts = millis();
    String mac = WiFi.macAddress();
    String momentId = String(ts) + ":" + mac;

    StaticJsonDocument<320> sensorsMessage;

    sensorsMessage["air"]["temperature"]          = currentTemperature;
    sensorsMessage["air"]["humidity"]             = currentHumidity;
    sensorsMessage["soil"]["humidity"]            = currentSoilHumidity;
    sensorsMessage["irrigation"]["pid_output"]    = pidOutput;
    sensorsMessage["ventilation"]["pid_output"]   = fanPidOutput;
    sensorsMessage["deviceId"]                    = mac;
    sensorsMessage["timestamp"]                   = ts;
    sensorsMessage["sensor_data_moment_id"]       = momentId;

    char buffer[320];
    serializeJson(sensorsMessage, buffer);

    if (isConnected()) {
      bool ok = mqttClient.publish(GREENHOUSE_SENSORS_CURRENT_QUEUE, buffer, false, 2);

      if (ok) {
        Serial.print("[MQTT] Published (QoS 2): ");
        Serial.println(buffer);
      } else {
        Serial.println("[MQTT] Publish failed — saving to outbox");
        outboxEnqueue(buffer);
      }
    } else {
      Serial.println("[OUTBOX] Offline — saving to outbox");
      outboxEnqueue(buffer);
    }
  }
}
