#include "DHT.h"
#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>

#define DHTPIN 5
#define DHTTYPE DHT22

#define WIFI_SSID "Brennholzverleih"
#define WIFI_PASSWORD "Brennholzverleih"
#define WIFI_TIMEOUT 15

#define MQTT_HOST IPAddress(192, 168, 0, 1)
#define MQTT_PORT 1883
#define MQTT_TIMEOUT 10
#define MQTT_PUBLISH_INTERVAL 5000
#define MQTT_TEMPERATURE_TOPIC "esp01/temperature"
#define MQTT_HUMIDITY_TOPIC "esp01/humidity"

#define DEEP_SLEEP true

boolean fullyConnected = false;
AsyncMqttClient mqttClient;
DHT dht(DHTPIN, DHTTYPE);

boolean connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print((String)"Verbindungsversuch mit Wlan '" + WIFI_SSID + "' (Timeout: " + WIFI_TIMEOUT + " Sekunden) ");

  for (int i = WIFI_TIMEOUT; WiFi.status() != WL_CONNECTED; i--) {
    if (i == 0) {
      Serial.print(" Fehlgeschlagen!");
      Serial.println();
      return false;
    }
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" erfolgreich!");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());
  Serial.println("-----------------------");
  return true;
}

boolean connectToMQTT() {
  Serial.print((String)"Verbindungsversuch mit MQTT-Broker '" + MQTT_HOST.toString() + "' (Port: " + MQTT_PORT + ", Timeout: " + MQTT_TIMEOUT + " Sekunden) ");
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.connect();
  for (int i = MQTT_TIMEOUT; !mqttClient.connected(); i--) {
    if (i == 0) {
      Serial.print(" Fehlgeschlagen!");
      Serial.println();
      return false;
    }
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" erfolgreich!");
  Serial.println((String)"MQTT Publish Interval: " + MQTT_PUBLISH_INTERVAL + "ms");
  Serial.println("-----------------------");
  return true;
}

void sendMqttMessage(const char* topic, int QoS, const char* message) {
  uint16_t packetId = mqttClient.publish(topic, QoS, true, message);
  Serial.println((String)"Sending '" + message + "' on topic '" + topic + "' at QoS " + QoS + " (packetId: " + packetId + ")");
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  delay(500);
  Serial.println("ESP wird gestartet");
  if (connectToWiFi() && connectToMQTT()) {
    fullyConnected = true;
    dht.begin();
  } else {
    Serial.println("-----------------------");
    Serial.println("ERROR!");
    Serial.println("Um den Sketch nutzen zu können müssen WLAN und MQTT-Broker verbunden sein.");
    Serial.println("Überprüfe deine eingegebenen Daten und starte den ESP neu");
    Serial.println("-----------------------");
  }
}

void loop() {
  if (!fullyConnected) return;
  delay(MQTT_PUBLISH_INTERVAL);

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  String payload = "";
  payload += t;
  sendMqttMessage(MQTT_TEMPERATURE_TOPIC, 1, payload.c_str());
  payload = "";
  payload += h;
  sendMqttMessage(MQTT_HUMIDITY_TOPIC, 1, payload.c_str());
}
