#include "Template.h"
#include "DHT.h"

// MQTT Config
const String MQTT_SENSOR_NAME = "Server01";

// Custom config
// DHT22 (Temperature/Humidity)
const int DHT_PIN = 0;

const String MQTT_PUBLISH_TOPIC_EXT_TEMPERATURE = "/temperature";
const int TEMPERATURE_COOLDOWN = 30000;
const String MQTT_PUBLISH_TOPIC_EXT_HUMIDITY = "/humidity";
const int HUMIDITY_COOLDOWN = 30000;

DHT dht(DHT_PIN, DHT22);

void setup() {
  Serial.begin(115200);
  Serial.println();
  delay(500);
  Serial.println("ESP wird gestartet");
  templateSetup(MQTT_SENSOR_NAME);

  dht.begin();  
}

void readTemperature(unsigned long now) {
  static unsigned long lastReadTime;
  if (now - lastReadTime >= TEMPERATURE_COOLDOWN) {
    float temperature = dht.readTemperature();
    if (isnan(temperature)) {
      Serial.println("ERROR | Failed to read temperature from DHT sensor!");
      lastReadTime = now;
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
      lastReadTime = now;
      return;
    }
    
    sendMqttMessage(MQTT_PUBLISH_TOPIC_EXT_HUMIDITY.c_str(), 0, String(humidity).c_str());
    lastReadTime = now;
  }
}

void loop() {
  if (!fullyConnected)
    return;
  unsigned long now = millis();
  readTemperature(now);
  readHumidity(now);
}
