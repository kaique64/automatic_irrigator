#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "secret.h"

#define WATER_PUMP_OFF 0
#define WATER_PUMP_FULL_CAPACITY 255

#define DHT_TYPE DHT22
#define DHT_PIN 27
#define SOIL_HUMIDITY_PIN 34

#define SOIL_DRY 4095
#define SOIL_WET 1400

#define WATER_PUMP_PWM_PIN 25

#define GREENHOUSE_SOIL_HUMIDITY_SETPOINT "greenhouse/setpoint"
#define GREENHOUSE_SENSORS_CURRENT_QUEUE "greenhouse/sensors"

float soilSetpoint = 70.0;
float lastValidTemperature = 0.0;
float lastValidHumidity = 0.0;

float Kp_base = 2.0;
float Ki_base = 0.05;
float Kd_base = 1.0;

float T_ref = 25.0;
float alpha = 0.03;

float error = 0;
float previousError = 0;
float integral = 0;
float derivative = 0;
float pidOutput = 0;

unsigned long lastPIDTime = 0;

const int pwmFrequency = 1000;
const int pwmResolution = 8;

WiFiClientSecure espClient;
MQTTClient mqttClient(512); // 512-byte buffer for messages

DHT dht(DHT_PIN, DHT_TYPE);

// ─── Wi-Fi ────────────────────────────────────────────────────────────────────

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(1000);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to Wi-Fi! Status: " + String(WiFi.status()));
    Serial.println("Restarting in 3s...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("\nWi-Fi connected: " + WiFi.localIP().toString());
}

// ─── MQTT ─────────────────────────────────────────────────────────────────────

void callback(String& topic, String& payload) {
  StaticJsonDocument<200> doc;

  DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(err.f_str());
    return;
  }

  if (doc.containsKey("setpoint")) {
    soilSetpoint = doc["setpoint"];
    Serial.print("Setpoint received: ");
    Serial.println(soilSetpoint);
  }
}

void connectMQTT() {
  mqttClient.setOptions(10, true, 5000); // keepalive 10s, cleanSession, timeout 5s

  String clientId = "ESP32Client-" + String(random(0xffff), HEX);

  Serial.print("Connecting to MQTT...");

  while (!mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.print(".");
    Serial.print(" rc=");
    Serial.println(mqttClient.lastError());
    delay(5000);
  }

  Serial.println("MQTT connected!");

  // Subscribe to setpoint topic with QoS 2
  // (QoS 2 on subscribe is supported, but public brokers rarely honor above QoS 1)
  mqttClient.subscribe(GREENHOUSE_SOIL_HUMIDITY_SETPOINT, 2);
}

// ─── Soil humidity sensor ─────────────────────────────────────────────────────

float readCurrentSoilHumidityPercentage() {
  int raw = analogRead(SOIL_HUMIDITY_PIN);
  int soilHumidity = constrain(raw, SOIL_WET, SOIL_DRY);
  return map(soilHumidity, SOIL_WET, SOIL_DRY, 100, 0);
}

// ─── Setup ────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);

  dht.begin();
  delay(2000);

  pinMode(SOIL_HUMIDITY_PIN, INPUT);
  ledcAttach(WATER_PUMP_PWM_PIN, pwmFrequency, pwmResolution);

  connectWiFi();

  espClient.setInsecure();

  mqttClient.begin(MQTT_BROKER, MQTT_PORT, espClient);
  mqttClient.onMessage(callback);

  connectMQTT();

  lastPIDTime = millis();
}

// ─── Loop ─────────────────────────────────────────────────────────────────────

void loop() {

  // Keep MQTT connection alive
  if (!mqttClient.connected()) {
    Serial.println("MQTT disconnected, reconnecting...");
    connectMQTT();
  }
  mqttClient.loop();

  // Read DHT22
  float currentTemperature = dht.readTemperature();
  float currentHumidity = dht.readHumidity();

  if (isnan(currentTemperature) || isnan(currentHumidity)) {
    currentTemperature = lastValidTemperature;
    currentHumidity = lastValidHumidity;
  } else {
    lastValidTemperature = currentTemperature;
    lastValidHumidity = currentHumidity;
  }

  float currentSoilHumidity = readCurrentSoilHumidityPercentage();

  // ─── PID ──────────────────────────────────────────────────────────────────

  unsigned long currentTime = millis();
  float deltaTime = (currentTime - lastPIDTime) / 1000.0;

  if (deltaTime >= 1.0) {
    float thermalFactor = 1 + alpha * (currentTemperature - T_ref);
    thermalFactor = constrain(thermalFactor, 0.7, 1.5);

    float Kp = Kp_base * thermalFactor;
    float Ki = Ki_base * thermalFactor;
    float Kd = Kd_base * thermalFactor;

    error = soilSetpoint - currentSoilHumidity;

    integral += error * deltaTime;
    integral = constrain(integral, -100, 100);

    derivative = (error - previousError) / deltaTime;

    pidOutput = (Kp * error) + (Ki * integral) + (Kd * derivative);
    pidOutput = constrain(pidOutput, WATER_PUMP_OFF, WATER_PUMP_FULL_CAPACITY);

    if (abs(error) < 2.0) {
      pidOutput = 0;
    }

    if (pidOutput > 0) {
      pidOutput = map(pidOutput, 0, 255, 90, 255);
    }

    ledcWrite(WATER_PUMP_PWM_PIN, (int)pidOutput);

    previousError = error;
    lastPIDTime = currentTime;
  }

  // ─── MQTT publish with QoS 2 ──────────────────────────────────────────────

  static unsigned long lastPublish = 0;

  if (millis() - lastPublish > 2000) {
    lastPublish = millis();

    Serial.printf("[AIR]  Temp: %.2f | Humidity: %.2f\n", currentTemperature, currentHumidity);
    Serial.printf("[SOIL] Humidity: %.2f\n", currentSoilHumidity);
    Serial.printf("[PID]  Setpoint: %.2f | Output: %.2f\n", soilSetpoint, pidOutput);

    StaticJsonDocument<256> sensorsMessage;

    sensorsMessage["air"]["temperature"] = currentTemperature;
    sensorsMessage["air"]["humidity"]    = currentHumidity;

    sensorsMessage["soil"]["humidity"] = currentSoilHumidity;

    sensorsMessage["irrigation"]["pid_output"] = pidOutput;

    sensorsMessage["deviceId"]  = WiFi.macAddress();
    sensorsMessage["timestamp"] = millis();

    char buffer[256];
    serializeJson(sensorsMessage, buffer);

    // Publish with QoS 2 (exactly-once delivery)
    bool ok = mqttClient.publish(GREENHOUSE_SENSORS_CURRENT_QUEUE, buffer, false, 2);

    if (ok) {
      Serial.print("MQTT published (QoS 2): ");
      Serial.println(buffer);
    } else {
      Serial.println("Failed to publish MQTT, rc=" + String(mqttClient.lastError()));
    }
  }
}