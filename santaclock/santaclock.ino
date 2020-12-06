#include <FS.h>          // this needs to be first, or it all crashes and burns...
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include <DS3232RTC.h>   // https://github.com/JChristensen/DS3232RTC
#include <FastLED.h>

WiFiManager wm;
DS3232RTC myRTC;

//flag for saving data
bool shouldSaveConfig = false;

//for LED status
#include <Ticker.h>
Ticker ticker;
bool tick_on = true;

#define NUM_LEDS 60
#define DATA_PIN 2
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS 100

#define LOGO 0
#define DIGIT1 14
#define DIGIT2 35
#define DIGIT3 58
#define DIGIT4 79
#define DOT1 56
#define DOT2 57

CRGB leds[NUM_LEDS];
int i = 0;

void tick()
{

    leds[DOT2] = CRGB::Blue;
    int brightness = 0;
    if (tick_on)
    {
        brightness = 255;
    }
    else
    {
        brightness = 100;
    }
    tick_on = !tick_on;
    FastLED.setBrightness(brightness);
    FastLED.show();
}

void configModeCallback(WiFiManager *myWiFiManager)
{
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    //if you used auto generated SSID, print it
    Serial.println(myWiFiManager->getConfigPortalSSID());
    //entered config mode, make led toggle faster
    ticker.attach(0.2, tick);
}

//callback notifying us of the need to save config
void saveConfigCallback()
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

void setNumberOfLeds(int firstPosition, int number, CRGB crgb)
{
    for (int i = 0; i < number; i++)
    {
        leds[firstPosition + i] = crgb;
    }
}

void setLedSegment(int offset, CRGB crgb, String charArray, char n)
{
    if (charArray.indexOf(n) >= 0)
        setNumberOfLeds(offset, 3, crgb);
    else
        setNumberOfLeds(offset, 3, CRGB(0, 0, 0));
}

void drawdigit(int offset, CRGB crgb, char n)
{
    Serial.print("Going to draw a : ");
    Serial.print(n);
    Serial.print(" on position ");
    Serial.println(offset);

    String top = "02356789x";
    String top_right = "01234789x";
    String bottom_right = "013456789o";
    String bottom = "0235689o";
    String top_left = "045689x";
    String bottom_left = "0268o";
    String middle = "2345689xo";

    setLedSegment(0 + offset, crgb, middle, n);
    setLedSegment(3 + offset, crgb, bottom_right, n);
    setLedSegment(6 + offset, crgb, bottom, n);
    setLedSegment(9 + offset, crgb, bottom_left, n);
    setLedSegment(12 + offset, crgb, top_left, n);
    setLedSegment(15 + offset, crgb, top, n);
    setLedSegment(18 + offset, crgb, top_right, n);

    // if (middle.indexOf(n) >= 0)
    // {
    //     setNumberOfLeds(0 + offset, 3, crgb);
    // }
    // else
    // {
    //     setNumberOfLeds(0 + offset, 3, CRGB(0, 0, 0));
    // }

    // if (bottom_right.indexOf(n) >= 0)
    // {
    //     setNumberOfLeds(3 + offset, 3, crgb);
    // }
    // else
    // {
    //     setNumberOfLeds(3 + offset, 3, CRGB(0, 0, 0));
    // }

    // if (bottom.indexOf(n) >= 0)
    // {
    //     setNumberOfLeds(6 + offset, 3, crgb);
    // }
    // else
    // {
    //     setNumberOfLeds(6 + offset, 3, CRGB(0, 0, 0));
    // }

    // if (bottom_left.indexOf(n) >= 0)
    // {
    //     setNumberOfLeds(9 + offset, 3, crgb);
    // }
    // else
    // {
    //     setNumberOfLeds(9 + offset, 3, CRGB(0, 0, 0));
    // }

    // if (top_left.indexOf(n) >= 0)
    // {
    //     setNumberOfLeds(12 + offset, 3, crgb);
    // }
    // else
    // {

    //     setNumberOfLeds(12 + offset, 3, CRGB(0, 0, 0));
    // }

    // if (top.indexOf(n) >= 0)
    // {
    //     setNumberOfLeds(15 + offset, 3, crgb);
    // }
    // else
    // {

    //     setNumberOfLeds(15 + offset, 3, CRGB(0, 0, 0));
    // }

    // if (top_right.indexOf(n) >= 0)
    // {
    //     setNumberOfLeds(18 + offset, 3, crgb);
    // }
    // else

    // {
    //     setNumberOfLeds(18 + offset, 3, CRGB(0, 0, 0));
    // }
}

void setup()
{
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

    Serial.begin(9600);

    myRTC.begin();

    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    Serial.println("FastLED setup");

    FastLED.clear();
    FastLED.show();

    // ticker.attach(0.6, tick);

    wm.resetSettings();
    wm.setHostname("SecretSanctaClock");

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

    // leds[i] = CRGB::Red;
    // Serial.println(i);
    // i++;

    // if (i > NUM_LEDS)
    // {
    //     FastLED.clear();
    //     i = 0;
    // }

    // if(i > 9){
    //     i = 0;
    // }

    // char c = '0' + i;

    // i++;

    // CRGB crgb = CRGB::Green;
    // crgb = CRGB::Red;
    // drawdigit(DIGIT1, crgb, c); //Draw the first digit of the hour
    // drawdigit(DIGIT2, crgb, c); //Draw the second digit of the hour
    // drawdigit(DIGIT3, crgb, c); //Draw the first digit of the hour
    // drawdigit(DIGIT4, crgb, c); //Draw the second digit of the hour

    time_t t = myRTC.get();
    int hours = hour(t);
    int mins = minute(t);

    char h1 = '0' + hours / 10;
    char h2 = '0' + hours - ((hours / 10) * 10);
    char m1 = '0' + mins / 10;
    char m2 = '0' + mins - ((mins / 10) * 10);

    CRGB crgb = CRGB::Red;
    drawdigit(DIGIT1, crgb, h1); //Draw the first digit of the hour
    drawdigit(DIGIT2, crgb, h2); //Draw the second digit of the hour
    drawdigit(DIGIT3, crgb, m1); //Draw the first digit of the hour
    drawdigit(DIGIT4, crgb, m2); //Draw the second digit of the hour

    crgb = CRGB::Green;
    setNumberOfLeds(LOGO, DIGIT1, crgb);
    setNumberOfLeds(DOT1, 1, crgb);
    setNumberOfLeds(DOT2, 1, crgb);

    FastLED.setBrightness(150);
    FastLED.show();

    // char buf[40];
    // time_t t = myRTC.get();
    // float celsius = myRTC.temperature() / 4.0;
    // float fahrenheit = celsius * 9.0 / 5.0 + 32.0;
    // sprintf(buf, "%.2d:%.2d:%.2d %.2d%s%d ",
    //         hour(t), minute(t), second(t), day(t), monthShortStr(month(t)), year(t));
    // Serial.print(buf);
    // Serial.print(celsius);
    // Serial.print("C ");
    // Serial.print(fahrenheit);
    // Serial.println("F");
    // Serial.print("hours ");
    // Serial.println(hour(t));

    delay(500);
}
