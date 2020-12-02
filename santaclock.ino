#include <FastLED.h>
#define NUM_LEDS 60
#define DATA_PIN    2
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    100

CRGB leds[NUM_LEDS];
int i = 0;

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  Serial.println("FastLED setup");
  FastLED.clear();
  FastLED.show(); 

}

void loop() {
//        fill_solid( leds, NUM_LEDS, CRGB::Red);
  if (i<NUM_LEDS){
    leds[i] = CRGB::Red;
    Serial.println(i);
  } else {
    FastLED.clear();
    i = 0;
  }
  FastLED.show(); 
  delay(500);
  i++;
}