#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>

const String WIFI_SSID = "Brennholzverleih";
const String WIFI_PASSWORD = "Brennholzverleih";
const int WIFI_TIMEOUT = 15;

IPAddress MQTT_HOST = IPAddress(192, 168, 0, 1);
const int MQTT_PORT = 1883;
const int MQTT_TIMEOUT = 10;
const int MQTT_PUBLISH_INTERVAL = 3000;
String MQTT_PUBLISH_TOPIC = "/AD02/Brennholzverleih/Serverraum/%SENSOR_NAME%";

boolean fullyConnected = false;
AsyncMqttClient mqttClient;

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
  Serial.println((String)"MQTT Publish interval: " + MQTT_PUBLISH_INTERVAL + "ms");
  Serial.println("-----------------------");
  return true;
}

void sendMqttMessage(const char* topic, int QoS, const char* message) {
  uint16_t packetId = mqttClient.publish(topic, QoS, true, message);
  Serial.println((String)"Sending '" + message + "' on topic '" + topic + "' at QoS " + QoS);
}


void templateSetup() {
  if (connectToWiFi() && connectToMQTT()) {
    fullyConnected = true;
  } else {
    Serial.println("-----------------------");
    Serial.println("ERROR!");
    Serial.println("Um den Sketch nutzen zu können müssen WLAN und MQTT-Broker verbunden sein.");
    Serial.println("Überprüfe deine eingegebenen Daten und starte den ESP neu");
    Serial.println("-----------------------");
  }
}
