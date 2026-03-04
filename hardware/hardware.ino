#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "secret.h"

#define DHT_TYPE DHT22

const int DHT_PIN = 27;
const int LED_PIN = 18;
const char* AIR_SENSOR_TYPE = "air";
const char* GREENHOUSE_AIR_TEMPERATURE_THRESHOLD = "greenhouse/air/temperature/threshold";
const char* GREENHOUSE_AIR_TEMPERATURE_CURRENT = "greenhouse/air/temperature/current";

float temperatureLimit = 0.0;

WiFiClientSecure espClient;
PubSubClient client(espClient);

DHT dht(DHT_PIN, DHT_TYPE);

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
  
  // pinMode(LED_PIN, OUTPUT);
  // digitalWrite(LED_PIN, LOW);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  espClient.setInsecure();
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("\nAttempting MQTT connection...");

    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      client.subscribe(GREENHOUSE_AIR_TEMPERATURE_THRESHOLD);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float currentTemperature = dht.readTemperature();
  float currentHumidity = dht.readHumidity();

  if (currentTemperature > temperatureLimit) {
    // TODO: turn on the water pump
  } else {
    // TODO: turn off the water pump
  }

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();
    Serial.printf("Status: Received [%.2f] | Limit [%.2f]\n", currentTemperature, temperatureLimit);
    StaticJsonDocument<200> docPublish;
    docPublish["sensorType"] = AIR_SENSOR_TYPE;
    docPublish["currentTemperature"] = currentTemperature;
    docPublish["currentHumidity"] = currentHumidity;

    char buffer[200];
    serializeJson(docPublish, buffer);

    client.publish(GREENHOUSE_AIR_TEMPERATURE_CURRENT, buffer);
    
    Serial.print("Published MQTT: ");
    Serial.println(buffer);
  }
}
