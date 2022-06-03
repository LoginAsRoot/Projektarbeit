#include <SPI.h>      
#include <MFRC522.h>

#include "WS2812B.h";

// RFID
const String MQTT_PUBLISH_TOPIC_EXT_RFID = "/rfid";
const int RFID_COOLDOWN = 3000;

MFRC522 mfrc522(4, 5);

void setupRFID() {
  // LED Indikator
  setupWS2812B();
  
  SPI.begin();      
  mfrc522.PCD_Init();
}

void accessGranted(String rfidContent) {
  sendMqttMessage(MQTT_PUBLISH_TOPIC_EXT_RFID.c_str(), 0, String("Access Granted - " + rfidContent).c_str());
  WS2812B_RFID_ACCESS_GRANTED();
}

void accessRefused(String rfidContent) {
  sendMqttMessage(MQTT_PUBLISH_TOPIC_EXT_RFID.c_str(), 0, String("Access Refused - " + rfidContent).c_str());
  WS2812B_RFID_ACCESS_REFUSED();
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
