#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "secret.h"

#define DHT_TYPE DHT22
#define SOLO_DRY 4095
#define SOLO_WET 1400
#define DHT_PIN 27
#define SOLO_HUMIDITY_PIN 34
#define GREENHOUSE_AIR_TEMPERATURE_THRESHOLD "greenhouse/air/temperature/threshold"
#define GREENHOUSE_SENSORS_TEMPERATURE_CURRENT "greenhouse/sensors"

// const int DHT_PIN = 27;
// const int SOLO_HUMIDITY_PIN = 34;

// const char* GREENHOUSE_AIR_TEMPERATURE_THRESHOLD = "greenhouse/air/temperature/threshold";
// const char* GREENHOUSE_SENSORS_TEMPERATURE_CURRENT = "greenhouse/sensors";

float temperatureLimit = 0.0;

WiFiClientSecure espClient;
PubSubClient client(espClient);

DHT dht(DHT_PIN, DHT_TYPE);

float readCurrentSoloHumidityPercentage() {
  int raw = analogRead(SOLO_HUMIDITY_PIN);

  int soloHumidity = constrain(raw, SOLO_WET, SOLO_DRY);

  float result = map(soloHumidity, SOLO_WET, SOLO_DRY, 100, 0);

  return result;
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  StaticJsonDocument<200> doc;

  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.f_str());
    return;
  }

  if (doc.containsKey("temperatureLimit")) {
    temperatureLimit = doc["temperatureLimit"];
    Serial.print("Temperature Limit: ");
    Serial.println(temperatureLimit);
  }
}

void setup() {
  dht.begin();
  delay(2000);

  Serial.begin(115200);
  
  pinMode(SOLO_HUMIDITY_PIN, INPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("connected");
  
  espClient.setInsecure();
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);
}

void reconnect() {
  static unsigned long lastAttempt = 0;

  if (millis() - lastAttempt < 5000) return;
  lastAttempt = millis();

  String clientId = "ESP32Client-" + String(random(0xffff), HEX);
  if (client.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.println("MQTT connected");
    client.subscribe(GREENHOUSE_AIR_TEMPERATURE_THRESHOLD);
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float currentTemperature = dht.readTemperature();
  float currentHumidity = dht.readHumidity();
  float currentSoilHumidityPercentage = readCurrentSoloHumidityPercentage();

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();
    Serial.printf("[AIR] Current Temperature [%.2f] | Temperature Limit [%.2f]\n", currentTemperature, temperatureLimit);
    Serial.printf("[SOLO] Current Percentage Solo Humidity [%.2f]\n", currentSoilHumidityPercentage);

    StaticJsonDocument<200> sensorsMessage;
    sensorsMessage["air"]["temperature"] = currentTemperature;
    sensorsMessage["air"]["humidity"] = currentHumidity;
    sensorsMessage["soil"]["humidity"] = currentSoilHumidityPercentage;
    sensorsMessage["deviceId"] = WiFi.macAddress();
    sensorsMessage["timestamp"] = millis();

    char buffer[200];
    serializeJson(sensorsMessage, buffer);
    client.publish(GREENHOUSE_SENSORS_TEMPERATURE_CURRENT, buffer);
    Serial.print("Published MQTT: ");
    Serial.println(buffer);
  }
}
