#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include <SPI.h>      
#include <MFRC522.h>
#include <Adafruit_NeoPixel.h>

#include "DHT.h"

// WiFi Config
#define WIFI_SSID "Brennholzverleih"
#define WIFI_PASSWORD "Brennholzverleih"

// MQTT Config
#define MQTT_HOST IPAddress(192, 168, 0, 1)
#define MQTT_PORT 1883

// RFID Config
#define RFID_COOLDOWN 3000
#define MQTT_PUBLISH_TOPIC_EXT_RFID "/rfid"
MFRC522 mfrc522(4, 5);

// WS2812B Config
#define WS2812B_PIN 0
Adafruit_NeoPixel leds(12, WS2812B_PIN, NEO_GRB + NEO_KHZ800);

// DHT22 Config
#define DHT_PIN 15
#define DHT_COOLDOWN 5000
#define MQTT_PUBLISH_TOPIC_EXT_TEMPERATURE "/temperature"
#define MQTT_PUBLISH_TOPIC_EXT_HUMIDITY "/humidity"
DHT dht(DHT_PIN, DHT22);

boolean fullyConnected = false;
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
  mqttReconnectTimer.detach();
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  fullyConnected = true;
  digitalWrite(BUILTIN_LED, LOW);
  Serial.println("Connected to MQTT.");
  
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
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

  // RFID
  SPI.begin();
  mfrc522.PCD_Init();

  // BUILTIN_LED
  pinMode(BUILTIN_LED, OUTPUT);

}

void loop() {
  if (!fullyConnected) {
    if (digitalRead(BUILTIN_LED) == LOW)
      digitalWrite(BUILTIN_LED, HIGH);
    return;
  }
  unsigned long now = millis();
  readDHT(now);
  readRFID(now);
}


// ------------- DHT22 -------------

void readDHT(unsigned long now) {
  static unsigned long lastReadTime;
  if (now - lastReadTime >= DHT_COOLDOWN) {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("ERROR | Failed to read from DHT sensor!");
      return;
    }

    uint16_t packetIdDhtPub1_1 = mqttClient.publish("/AD02/Brennholzverleih/Serverraum/Zugangstuer/temperature", 1, true, String(temperature).c_str());
    Serial.print("Publishing at QoS 1, packetId: ");
    Serial.println(packetIdDhtPub1_1);

    uint16_t packetIdDhtPub1_2 = mqttClient.publish("/AD02/Brennholzverleih/Serverraum/Zugangstuer/humidity", 1, true, String(humidity).c_str());
    Serial.print("Publishing at QoS 1, packetId: ");
    Serial.println(packetIdDhtPub1_2);
    
    lastReadTime = now;
  }
}


// ---------------- RFID ----------------

void readRFID(unsigned long now) {
  static unsigned long lastReadTime;
  if (now - lastReadTime >= RFID_COOLDOWN) {
    if (lightOn) {
      leds.clear();
      leds.show();
    }
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      String rfidContent = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        rfidContent.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        rfidContent.concat(String(mfrc522.uid.uidByte[i], HEX));
      }
      rfidContent.toUpperCase();
      if (rfidContent.substring(1) == "39 30 CC C2") {
        accessGranted(rfidContent.substring(1));
      } else {
        accessRefused(rfidContent.substring(1));
      }
      lastReadTime = now;
    }
  }
}

void accessGranted(String rfidContent) {
  uint16_t packetIdRfidPub1 = mqttClient.publish("/AD02/Brennholzverleih/Serverraum/Zugangstuer/rfid", 1, true, String("Access Granted - " + rfidContent).c_str());
  Serial.print("Publishing at QoS 1, packetId: ");
  Serial.println(packetIdRfidPub1);

  uint16_t packetIdLightPub1 = mqttClient.publish("/AD02/Brennholzverleih/Serverraum/licht", 1, true, "1");
  Serial.print("Publishing at QoS 1, packetId: ");
  Serial.println(packetIdLightPub1);
  leds.fill(leds.Color(0, 255, 0));
  leds.show();
  lightOn = true;
}

void accessRefused(String rfidContent) {
  uint16_t packetIdRfidPub1 = mqttClient.publish("/AD02/Brennholzverleih/Serverraum/Zugangstuer/rfid", 1, true, String("Access Refused - " + rfidContent).c_str());
  Serial.print("Publishing at QoS 1, packetId: ");
  Serial.println(packetIdRfidPub1);
  
  leds.fill(leds.Color(255, 0, 0));
  leds.show();
}



/*

// ---------------- MQTT ----------------

boolean setupMQTT() {
  Serial.print((String)"Verbindungsversuch mit MQTT-Broker '" + MQTT_HOST.toString() + "' (Port: " + MQTT_PORT + ", Timeout: " + MQTT_TIMEOUT + " Sekunden) ");
  mqttClient.onMessage(onMqttMessage);
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
  Serial.println((String)"MQTT Publish topic: " + MQTT_PUBLISH_TOPIC);
  Serial.println("-----------------------");
  mqttClient.subscribe(MQTT_SUBSCRIBE_TOPIC_EXT_TUER.c_str(), 0);
  return true;
}

void processMqttMessage(const char* topic, const char* payload) {
  if (strcmp(topic, MQTT_SUBSCRIBE_TOPIC_EXT_TUER.c_str()) == 0) {
    if (strcmp(payload, "0") == 0) {
      Serial.println("Tür zu");
    } else if (strcmp(payload, "1") == 0) {
      Serial.println("Tür auf");
    }
  }
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  char new_payload[len+1];
  new_payload[len] = '\0';
  strncpy(new_payload, payload, len);

  Serial.println((String)"Recieved '" + new_payload + "' on topic '" + topic + "' at QoS " + properties.qos);
  processMqttMessage(topic, new_payload);
}

void sendMqttMessage(String topicExtention, int QoS, const char* message) {
  mqttClient.publish((MQTT_PUBLISH_TOPIC + topicExtention).c_str(), QoS, true, message);
  Serial.println((String)"Sending '" + message + "' on topic '" + topicExtention + "' at QoS " + QoS);
}

// ---------------- RFID ----------------

void readRFIDReader(unsigned long now) {
  static unsigned long lastReadTime;
  if (now - lastReadTime >= RFID_COOLDOWN) {
    if (digitalRead(BUILTIN_LED) == LOW) {
      digitalWrite(BUILTIN_LED, HIGH);
    }
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      String rfidContent = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        rfidContent.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        rfidContent.concat(String(mfrc522.uid.uidByte[i], HEX));
      }
      rfidContent.toUpperCase();
      if (rfidContent.substring(1) == "39 30 CC C2") {
        accessGranted(rfidContent.substring(1));
      } else {
        accessRefused(rfidContent.substring(1));
      }
      lastReadTime = now;
    }
  }
}

void accessGranted(String rfidContent) {
  sendMqttMessage(MQTT_PUBLISH_TOPIC_EXT_RFID.c_str(), 0, String("Access Granted - " + rfidContent).c_str());
  mqttClient.publish(MQTT_PUBLISH_TOPIC_LIGHT.c_str(), 0, true, "1");
  Serial.println((String)"Sending '1' on topic '" + MQTT_PUBLISH_TOPIC_LIGHT + "' at QoS 0");
  digitalWrite(BUILTIN_LED, LOW);
  
  // LEDS
  for(int i = 0; i < leds.numPixels(); i++) { 
    leds.setPixelColor(i, leds.Color(0, 255, 0));
    leds.show();
    delay(25);
  }
  delay(1000);
  for(int i = 0; i < leds.numPixels(); i++) {
    leds.setPixelColor(i, leds.Color(0, 0, 0));
    leds.show();
    delay(25);
  }
}

void accessRefused(String rfidContent) {
  sendMqttMessage(MQTT_PUBLISH_TOPIC_EXT_RFID.c_str(), 0, String("Access Refused - " + rfidContent).c_str());
  
  // LEDS
  for(int i = 0; i < leds.numPixels(); i++) { 
    leds.setPixelColor(i, leds.Color(255, 0, 0));
    leds.show();
    delay(25);
  }
  delay(1000);
  for(int i = 0; i < leds.numPixels(); i++) {
    leds.setPixelColor(i, leds.Color(0, 0, 0));
    leds.show();
    delay(25);
  }
}

// ---------------- SETUP ----------------

#include "DHT22.h";
//#include "ULTRASONIC.h";

void setup() {
  Serial.begin(115200);
  Serial.println();
  delay(500);
  Serial.println("ESP wird gestartet");
  if (setupWiFi() && setupMQTT()) {
    fullyConnected = true;
    // DHT22
    setupDHT();

    // RFID
    SPI.begin();
    mfrc522.PCD_Init();
    
    // ULTRASONIC
    //setupULTRASONIC();

    // WS2812B
    leds.begin();
    leds.show();
    leds.setBrightness(150);

    // BUILTIN_LED
    pinMode(BUILTIN_LED, OUTPUT);
  } else {
    Serial.println("-----------------------");
    Serial.println("ERROR!");
    Serial.println("Um den Sketch nutzen zu können müssen WLAN und MQTT-Broker verbunden sein.");
    Serial.println("Überprüfe deine eingegebenen Daten und starte den ESP neu");
    Serial.println("-----------------------");
  }
}

// ---------------- LOOP ----------------

void loop() {
  if (!fullyConnected)
    return;
  unsigned long now = millis();
  readRFIDReader(now);
  readTemperature(now);
  readHumidity(now);
  //readDistance(now);
}

*/
