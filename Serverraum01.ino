#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include <Adafruit_NeoPixel.h>
#include "DHT.h"

// WiFi Config
#define WIFI_SSID "Brennholzverleih"
#define WIFI_PASSWORD "Brennholzverleih"

// MQTT Config
#define MQTT_HOST IPAddress(192, 168, 0, 1)
#define MQTT_PORT 1883

// WS2812B Config
#define WS2812B_PIN 0
Adafruit_NeoPixel leds(12, WS2812B_PIN, NEO_GRB + NEO_KHZ800);
#define WS2812B_LIGHT_COLOR leds.Color(52,168,174)

// Light Button Config
#define LIGHT_BUTTON_PIN 5
#define LIGHT_BUTTON_COOLDOWN 1000

// DHT22 Config
#define DHT_PIN 4
#define DHT_COOLDOWN 10000
DHT dht(DHT_PIN, DHT22);

boolean fullyConnected =  false;
boolean lightOn = false;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

void connectToWifi() {
  Serial.println((String)"Connecting to Wi-Fi '" + WIFI_SSID + "'");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println((String)"Connected to Wi-Fi '" + WIFI_SSID + "'");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  fullyConnected = false;
  Serial.println((String)"Disconnected from Wi-Fi '" + WIFI_SSID + "'");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  fullyConnected = true;
  Serial.println("Connected to MQTT.");
  
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  
  uint16_t packetIdSub = mqttClient.subscribe("/AD02/Brennholzverleih/Serverraum/licht", 1);
  Serial.print("Subscribing at QoS 1, packetId: ");
  Serial.println(packetIdSub);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  fullyConnected = false;
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.print("Subscribe acknowledged. PacketId: ");
  Serial.print(packetId);
  Serial.print(",  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.print("Unsubscribe acknowledged. PacketId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  qos: ");
  Serial.println(properties.qos);
  Serial.print("  dup: ");
  Serial.println(properties.dup);
  Serial.print("  retain: ");
  Serial.println(properties.retain);
  Serial.print("  len: ");
  Serial.println(len);
  Serial.print("  index: ");
  Serial.println(index);
  Serial.print("  total: ");
  Serial.println(total);
  
  char new_payload[len+1];
  new_payload[len] = '\0';
  strncpy(new_payload, payload, len);
  Serial.print("  payload: ");
  Serial.println(new_payload);

  if (strcmp(topic, "/AD02/Brennholzverleih/Serverraum/licht") == 0) {
    if (strcmp(new_payload, "1") == 0) {
      turnLightOn();
    } else if (strcmp(new_payload, "0") == 0) {
      turnLightOff();
    }
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged. PacketId: ");
  Serial.println(packetId);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();

  // Sensoren
  dht.begin();

  leds.begin();
  leds.show();

  pinMode(LIGHT_BUTTON_PIN, INPUT);
}

void loop() {
  if (!fullyConnected)
    return;
  unsigned long now = millis();
  readDHT(now);
  readLightButton(now);
  
}

// ------------- WS2812B -------------

void turnLightOn() {
  leds.fill(WS2812B_LIGHT_COLOR);
  leds.show();
  lightOn = true;
}

void turnLightOff() {
  leds.clear();
  leds.show();
  lightOn = false;
}

void toggleLight() {
  if (lightOn) {
    turnLightOff();
  } else {
    turnLightOn();
  }
}

// ------------- Light button -------------

void readLightButton(unsigned long now) {
  static unsigned long lastReadTime;
  if (now - lastReadTime >= LIGHT_BUTTON_COOLDOWN) {
    if (digitalRead(LIGHT_BUTTON_PIN) == HIGH) {
      toggleLight();
      lastReadTime = now;
    }
  }
}

// ------------- DHT22 -------------

void readDHT(unsigned long now) {
  static unsigned long lastReadTime;
  if (now - lastReadTime >= DHT_COOLDOWN) {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("ERROR | Failed to read from DHT sensor!");
    } else {
      uint16_t packetIdDhtPub1_1 = mqttClient.publish("/AD02/Brennholzverleih/Serverraum/Serverraum01/temperature", 1, true, String(temperature).c_str());
      Serial.print("Publishing at QoS 1, packetId: ");
      Serial.println(packetIdDhtPub1_1);

      uint16_t packetIdDhtPub1_2 = mqttClient.publish("/AD02/Brennholzverleih/Serverraum/Serverraum01/humidity", 1, true, String(humidity).c_str());
      Serial.print("Publishing at QoS 1, packetId: ");
      Serial.println(packetIdDhtPub1_2);
    }

    lastReadTime = now;
  }
}
