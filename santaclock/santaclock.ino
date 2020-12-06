
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <FastLED.h>

WiFiManager wm;

//for LED status
#include <Ticker.h>
Ticker ticker;
bool tick_on = true;

#define NUM_LEDS 60
#define DATA_PIN 2
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS 100

#define DIGIT1_OFFSET 14
#define DIGIT2_OFFSET 35
#define DIGIT3_OFFSET 58
#define DIGIT4_OFFSET 79
#define DOT1_OFFSET 56
#define DOT2_OFFSET 57

CRGB leds[NUM_LEDS];
int i = 0;

// void tick()
// {
//     leds[DOT2_OFFSET] = CRGB::Blue;
//     int brightness = 0;
//     if (tick_on)
//     {
//         brightness = 255;
//     }
//     else
//     {
//         brightness = 100;
//     }
//     tick_on = !tick_on;
//     FastLED.setBrightness(brightness);
//     FastLED.show();
// }

// void configModeCallback(WiFiManager *myWiFiManager)
// {
//     Serial.println("Entered config mode");
//     Serial.println(WiFi.softAPIP());
//     //if you used auto generated SSID, print it
//     Serial.println(myWiFiManager->getConfigPortalSSID());
//     //entered config mode, make led toggle faster
//     ticker.attach(0.2, tick);
// }

void setup()
{
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

    Serial.begin(9600);

    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    Serial.println("FastLED setup");

    FastLED.clear();
    FastLED.show();

    // ticker.attach(0.6, tick);

    wm.resetSettings();

    // wm.setAPCallback(configModeCallback);
    wm.setConfigPortalBlocking(false);

    bool res;
    res = wm.autoConnect("SecretSantaClockAP");

    if (!res)
    {
        Serial.println("Failed to connect");
        // ESP.restart();
    }
    else
    {
        //if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");
    }
}

void loop()
{
    // because we are not blocking ...
    wm.process();

    leds[i] = CRGB::Red;
    Serial.println(i);
    i++;

    if (i > NUM_LEDS)
    {
        FastLED.clear();
        i = 0;
    }
    FastLED.show();
    delay(500);
}
