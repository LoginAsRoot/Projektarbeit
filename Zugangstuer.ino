#include "Template.h"
#include "DHT22.h";
#include "RFID.h";
#include "ULTRASONIC.h";

// MQTT Config
const String MQTT_SENSOR_NAME = "Zugangstuer";

void setup() {
  Serial.begin(115200);
  Serial.println();
  delay(500);
  Serial.println("ESP wird gestartet");
  if (templateSetup(MQTT_SENSOR_NAME)) {
    setupDHT();
    setupRFID();
    setupULTRASONIC();
  }
}



void loop() {
  if (!fullyConnected)
    return;
  unsigned long now = millis();
  readRFIDReader(now);
  readTemperature(now);
  readHumidity(now);
  readDistance(now);
}
