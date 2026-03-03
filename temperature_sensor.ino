#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secret.h"

const int LED_PIN = 18;
const char* SET_TEMPERATURE_TOPIC = "temperature/set";
const char* CURRENT_TEMPERATURE_TOPIC = "temperature/current";

// const char* ssid = WIFI_SSID;
// const char* password = WIFI_PASSWORD;

// const char* mqtt_broker = MQTT_BROKER;
// const int mqtt_port = MQTT_PORT;
// const char* mqtt_username = MQTT_USERNAME;
// const char* mqtt_password = MQTT_PASSWORD;

float temperatureLimit = 0.0;
float currentTemperature = 10.0;

WiFiClientSecure espClient;
PubSubClient client(espClient);

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
  Serial.begin(115200);
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

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
      client.subscribe(SET_TEMPERATURE_TOPIC);
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

  if (currentTemperature > temperatureLimit) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();
    Serial.printf("Status: Received [%.2f] | Limit [%.2f]\n", currentTemperature, temperatureLimit);
  }

  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 10000) {
    lastMsg = millis();

    StaticJsonDocument<200> docPublish;
    docPublish["currentTemperature"] = currentTemperature;

    char buffer[200];
    serializeJson(docPublish, buffer);

    client.publish(CURRENT_TEMPERATURE_TOPIC, buffer);
    
    Serial.print("Published MQTT: ");
    Serial.println(buffer);
  }
}
