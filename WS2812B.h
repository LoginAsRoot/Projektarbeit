#include <Adafruit_NeoPixel.h>

const int WS2812B_PIN = 2;
const int WS2812B_LED_COUNT = 12;

Adafruit_NeoPixel strip(WS2812B_LED_COUNT, WS2812B_PIN, NEO_GRB + NEO_KHZ800);

void setupWS2812B() {
  strip.begin();
  strip.show();
  strip.setBrightness(150);
}

void WS2812B_RFID_ACCESS_GRANTED() {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, strip.Color(0, 255, 0));         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(25);                           //  Pause for a moment
  }
  delay(1000);
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, strip.Color(0, 0, 0));         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(25);                           //  Pause for a moment
  }
}

void WS2812B_RFID_ACCESS_REFUSED() {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, strip.Color(0, 0, 255));         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(25);                           //  Pause for a moment
  }
  delay(1000);
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, strip.Color(0, 0, 0));         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(25);                           //  Pause for a moment
  }
}
