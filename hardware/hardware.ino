#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
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

#define GREENHOUSE_AIR_TEMPERATURE_THRESHOLD "greenhouse/air/temperature/threshold"
#define GREENHOUSE_SENSORS_TEMPERATURE_CURRENT "greenhouse/sensors"

float soilSetpoint = 70.0;
float temperatureLimit = 0.0;
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

const int pwmChannel = 0;
const int pwmFrequency = 1000;
const int pwmResolution = 8;

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

DHT dht(DHT_PIN, DHT_TYPE);

float readCurrentSoilHumidityPercentage() {

  int raw = analogRead(SOIL_HUMIDITY_PIN);

  int soilHumidity = constrain(raw, SOIL_WET, SOIL_DRY);

  float result = map(soilHumidity, SOIL_WET, SOIL_DRY, 100, 0);

  return result;
}

void callback(char* topic, byte* payload, unsigned int length) {

  StaticJsonDocument<200> doc;

  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.f_str());
    return;
  }

  if (doc.containsKey("temperatureLimit")) {

    temperatureLimit = doc["temperatureLimit"];

    Serial.print("Temperature Limit received: ");
    Serial.println(temperatureLimit);
  }
}

void setup() {

  Serial.begin(115200);

  dht.begin();
  delay(2000);

  pinMode(SOIL_HUMIDITY_PIN, INPUT);

  ledcAttach(WATER_PUMP_PWM_PIN, pwmFrequency, pwmResolution);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");

  espClient.setInsecure();

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(callback);

  lastPIDTime = millis();
}

void reconnectMQTT() {
  static unsigned long lastAttempt = 0;

  if (millis() - lastAttempt < 5000) return;

  lastAttempt = millis();

  String clientId = "ESP32Client-" + String(random(0xffff), HEX);

  if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {

    Serial.println("MQTT connected");

    mqttClient.subscribe(GREENHOUSE_AIR_TEMPERATURE_THRESHOLD);

  } else {

    Serial.print("MQTT failed, rc=");
    Serial.println(mqttClient.state());
  }
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }

  mqttClient.loop();

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

    if (abs(error) < 2) {
      pidOutput = 0;
    }

    if (pidOutput > 0) {
      pidOutput = map(pidOutput, 0, 255, 90, 255);
    }

    ledcWrite(WATER_PUMP_PWM_PIN, pidOutput);

    previousError = error;
    lastPIDTime = currentTime;
  }

  static unsigned long lastPublish = 0;

  if (millis() - lastPublish > 2000) {

    lastPublish = millis();

    Serial.printf("[AIR] Temp: %.2f | Humidity: %.2f\n", currentTemperature, currentHumidity);
    Serial.printf("[SOIL] Humidity: %.2f\n", currentSoilHumidity);
    Serial.printf("[PID] Output: %.2f\n", pidOutput);

    StaticJsonDocument<200> sensorsMessage;

    sensorsMessage["air"]["temperature"] = currentTemperature;
    sensorsMessage["air"]["humidity"] = currentHumidity;

    sensorsMessage["soil"]["humidity"] = currentSoilHumidity;

    sensorsMessage["irrigation"]["pid_output"] = pidOutput;

    sensorsMessage["deviceId"] = WiFi.macAddress();
    sensorsMessage["timestamp"] = millis();

    char buffer[200];

    serializeJson(sensorsMessage, buffer);

    mqttClient.publish(GREENHOUSE_SENSORS_TEMPERATURE_CURRENT, buffer);

    Serial.print("Published MQTT: ");
    Serial.println(buffer);
  }
}