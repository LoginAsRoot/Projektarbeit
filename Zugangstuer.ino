#include "Template.h"

#include <SPI.h>      
#include <MFRC522.h>

// MQTT Config
const String MQTT_SENSOR_NAME = "Zugangstuer";

//Custom config
const String MQTT_PUBLISH_TOPIC_EXT_RFID = "/rfid";

MFRC522 mfrc522(4, 5); 

void setup() {
  Serial.begin(115200);
  Serial.println();
  delay(500);
  Serial.println("ESP wird gestartet");
  templateSetup();
  MQTT_PUBLISH_TOPIC.replace("%SENSOR_NAME%", MQTT_SENSOR_NAME);

  
  SPI.begin();      
  mfrc522.PCD_Init();   
}

void accessGranted(String rfidContent) {
  sendMqttMessage((MQTT_PUBLISH_TOPIC + MQTT_PUBLISH_TOPIC_EXT_RFID).c_str(), 0, String("Access Granted - " + rfidContent).c_str());
  delay(3000);
}

void accessRefused(String rfidContent) {
   sendMqttMessage((MQTT_PUBLISH_TOPIC + MQTT_PUBLISH_TOPIC_EXT_RFID).c_str(), 0, String("Access Refused - " + rfidContent).c_str());
   delay(3000);
}

void loop() {
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
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
} 
