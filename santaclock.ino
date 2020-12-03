#include <FastLED.h>
#include <Ticker.h>
#include <DHT.h>

#include "defines.h"
#include "Credentials.h"
#include "dynamicParams.h"

#define NUM_LEDS 60
#define DATA_PIN 2
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS 100

DHT dht(DHT_PIN, DHT_TYPE);
BlynkTimer timer;
Ticker led_ticker;

CRGB leds[NUM_LEDS];
int i = 0;

void setup()
{
    Serial.begin(9600);
    while (!Serial)
        ;

    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    Serial.println("FastLED setup");
    FastLED.clear();
    FastLED.show();

#if (USE_LITTLEFS || USE_SPIFFS)
    Serial.print("\nStarting Async_ESP8266WM_Config using " + String(CurrentFileFS));
#else
    Serial.print("\nStarting Async_ESP8266WM_Config using EEPROM");
#endif

#if USE_SSL
    Serial.println(" with SSL on " + String(ARDUINO_BOARD));
#else
    Serial.println(" without SSL on " + String(ARDUINO_BOARD));
#endif

    Serial.println("Version " + String(BLYNK_ASYNC_WM_VERSION));

    // From v1.0.5
    // Set config portal SSID and Password
    Blynk.setConfigPortal("TestPortal-ESP8266", "TestPortalPass");
    // Set config portal IP address
    Blynk.setConfigPortalIP(IPAddress(192, 168, 12, 25));
    // Set config portal channel, default = 1. Use 0 => random channel from 1-13
    Blynk.setConfigPortalChannel(0);

    // From v1.0.5, select either one of these to set static IP + DNS
    Blynk.setSTAStaticIPConfig(IPAddress(192, 168, 2, 220), IPAddress(192, 168, 2, 1), IPAddress(255, 255, 255, 0));
    //Blynk.setSTAStaticIPConfig(IPAddress(192, 168, 2, 220), IPAddress(192, 168, 2, 1), IPAddress(255, 255, 255, 0),
    //                           IPAddress(192, 168, 2, 1), IPAddress(8, 8, 8, 8));
    //Blynk.setSTAStaticIPConfig(IPAddress(192, 168, 2, 220), IPAddress(192, 168, 2, 1), IPAddress(255, 255, 255, 0),
    //                           IPAddress(4, 4, 4, 4), IPAddress(8, 8, 8, 8));

    // Use this to default DHCP hostname to ESP8266-XXXXXX or ESP32-XXXXXX
    //Blynk.begin();
    // Use this to personalize DHCP hostname (RFC952 conformed)
    // 24 chars max,- only a..z A..Z 0..9 '-' and no '-' as last char
    Blynk.begin(HOST_NAME);

    timer.setInterval(60 * 1000, readAndSendData);

    if (Blynk.connected())
    {
#if (USE_LITTLEFS || USE_SPIFFS)
        Serial.println("\nBlynk ESP8288 using " + String(CurrentFileFS) + " connected. Board Name : " + Blynk.getBoardName());
#else
        {
            Serial.println("\nBlynk ESP8288 using EEPROM connected. Board Name : " + Blynk.getBoardName());
            Serial.printf("EEPROM size = %d bytes, EEPROM start address = %d / 0x%X\n", EEPROM_SIZE, EEPROM_START, EEPROM_START);
        }
#endif
    }
}

#if USE_DYNAMIC_PARAMETERS
void displayCredentials(void)
{
    Serial.println("\nYour stored Credentials :");

    for (int i = 0; i < NUM_MENU_ITEMS; i++)
    {
        Serial.println(String(myMenuItems[i].displayName) + " = " + myMenuItems[i].pdata);
    }
}
#endif

void loop()
{

    Blynk.run();
    //        fill_solid( leds, NUM_LEDS, CRGB::Red);
    if (i < NUM_LEDS)
    {
        leds[i] = CRGB::Red;
        Serial.println(i);
    }
    else
    {
        FastLED.clear();
        i = 0;
    }
    FastLED.show();
    delay(500);
    i++;

#if USE_DYNAMIC_PARAMETERS
    static bool displayedCredentials = false;

    if (!displayedCredentials)
    {
        for (int i = 0; i < NUM_MENU_ITEMS; i++)
        {
            if (!strlen(myMenuItems[i].pdata))
            {
                break;
            }

            if (i == (NUM_MENU_ITEMS - 1))
            {
                displayedCredentials = true;
                displayCredentials();
            }
        }
    }
#endif
}
