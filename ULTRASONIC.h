const int ULTRASONIC_TRIG_PIN = 15;
const int ULTRASONIC_ECHO_PIN = 16;
const String MQTT_PUBLISH_TOPIC_EXT_ULTRASONIC = "/ultrasonic";
const int ULTRASONIC_COOLDOWN = 1000;

void setupULTRASONIC() {
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
}

void readDistance(unsigned long now) {
  static unsigned long lastReadTime;
  if (now - lastReadTime >= ULTRASONIC_COOLDOWN) {
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
    
    long dauer = pulseIn(ULTRASONIC_ECHO_PIN, HIGH);
    float abstand = dauer * 0.034 / 2;
    
    Serial.println((String)"Abstand: " + abstand + "cm");
    //sendMqttMessage(MQTT_PUBLISH_TOPIC_EXT_ULTRASONIC.c_str(), 0, String(abstand).c_str());
    lastReadTime = now;
  }
}
