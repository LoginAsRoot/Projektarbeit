#include "DHT.h"

// DHT22 (Temperature/Humidity)
const int DHT_PIN = 0;

const String MQTT_PUBLISH_TOPIC_EXT_TEMPERATURE = "/temperature";
const int TEMPERATURE_COOLDOWN = 5000;
const String MQTT_PUBLISH_TOPIC_EXT_HUMIDITY = "/humidity";
const int HUMIDITY_COOLDOWN = 5000;

DHT dht(DHT_PIN, DHT22);

void setupDHT() {
  dht.begin();
}

void readTemperature(unsigned long now) {
  static unsigned long lastReadTime;
  if (now - lastReadTime >= TEMPERATURE_COOLDOWN) {
    float temperature = dht.readTemperature();
    if (isnan(temperature)) {
      Serial.println("ERROR | Failed to read temperature from DHT sensor!");
      return;
    }
    
    sendMqttMessage(MQTT_PUBLISH_TOPIC_EXT_TEMPERATURE.c_str(), 0, String(temperature).c_str());
    lastReadTime = now;
  }
}

void readHumidity(unsigned long now) {
  static unsigned long lastReadTime;
  if (now - lastReadTime >= HUMIDITY_COOLDOWN) {
    float humidity = dht.readHumidity();
    if (isnan(humidity)) {
      Serial.println("ERROR | Failed to read humidity from DHT sensor!");
      return;
    }
    
    sendMqttMessage(MQTT_PUBLISH_TOPIC_EXT_HUMIDITY.c_str(), 0, String(humidity).c_str());
    lastReadTime = now;
  }
}
