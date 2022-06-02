#include "Template.h"
#include "DHT.h"

#include <SPI.h>      
#include <MFRC522.h>

// MQTT Config
const String MQTT_SENSOR_NAME = "Zugangstuer";

// Custom config
// RFID
const String MQTT_PUBLISH_TOPIC_EXT_RFID = "/rfid";
const int RFID_COOLDOWN = 3000;
//const int RFID_ACCESS_LED_GREEN = -1;
//const int RFID_ACCESS_LED_RED = -1;

// DHT22 (Temperature/Humidity)
const int DHT_PIN = 0;

const String MQTT_PUBLISH_TOPIC_EXT_TEMPERATURE = "/temperature";
const int TEMPERATURE_COOLDOWN = 30000;
const String MQTT_PUBLISH_TOPIC_EXT_HUMIDITY = "/humidity";
const int HUMIDITY_COOLDOWN = 60000;

DHT dht(DHT_PIN, DHT22);
MFRC522 mfrc522(4, 5);

void setup() {
  Serial.begin(115200);
  Serial.println();
  delay(500);
  Serial.println("ESP wird gestartet");
  templateSetup(MQTT_SENSOR_NAME);

  // LED Indikator
  //pinMode(RFID_ACCESS_LED_GREEN, OUTPUT);
  //pinMode(RFID_ACCESS_LED_RED, OUTPUT);

  dht.begin();
  SPI.begin();      
  mfrc522.PCD_Init();   
}

void accessGranted(String rfidContent) {
  sendMqttMessage(MQTT_PUBLISH_TOPIC_EXT_RFID.c_str(), 0, String("Access Granted - " + rfidContent).c_str());
  //digitalWrite(RFID_ACCESS_LED_GREEN, HIGH);
  //delay(2000);
  //digitalWrite(RFID_ACCESS_LED_GREEN, LOW);
}

void accessRefused(String rfidContent) {
  sendMqttMessage(MQTT_PUBLISH_TOPIC_EXT_RFID.c_str(), 0, String("Access Refused - " + rfidContent).c_str());
  //digitalWrite(RFID_ACCESS_LED_RED, HIGH);
  //delay(2000);
  //digitalWrite(RFID_ACCESS_LED_RED, LOW);
}

void readRFIDReader(unsigned long now) {
  static unsigned long lastReadTime;
  if (now - lastReadTime >= RFID_COOLDOWN) {
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

void loop() {
  if (!fullyConnected)
    return;
  unsigned long now = millis();
  readRFIDReader(now);
  readTemperature(now);
  readHumidity(now);
} 
