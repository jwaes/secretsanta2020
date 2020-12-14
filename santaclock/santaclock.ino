#include <FS.h>          // this needs to be first, or it all crashes and burns...
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include <DS3232RTC.h>   // https://github.com/JChristensen/DS3232RTC
#include <Timezone.h>    // https://github.com/JChristensen/Timezone
#include <FastLED.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <time.h>
#include <WiFiUdp.h>

WiFiManager wm;


DS3232RTC myRTC;

WiFiUDP ntpUDP;
int GTMOffset = 0; // SET TO UTC TIME
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", GTMOffset * 60 * 60, 60 * 60 * 1000);

TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120}; // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};   // Central European Standard Time
Timezone CE(CEST, CET);
TimeChangeRule *tcr; //pointer to the time change rule, use to get TZ abbrev

//flag for saving data
bool shouldSaveConfig = false;

// //for LED status
// #include <Ticker.h>
// Ticker ticker;
// bool tick_on = true;

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
static uint8_t hue = 0;

static tm getDateTimeByParams(long time)
{
    struct tm *newtime;
    const time_t tim = time;
    newtime = localtime(&tim);
    return *newtime;
}

void configModeCallback(WiFiManager *myWiFiManager)
{
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    //if you used auto generated SSID, print it
    Serial.println(myWiFiManager->getConfigPortalSSID());
    //entered config mode, make led toggle faster
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
        leds[firstPosition + i].nscale8(250);
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
    // Serial.print("Going to draw a : ");
    // Serial.print(n);
    // Serial.print(" on position ");
    // Serial.println(offset);

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
}

void setup()
{
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

    Serial.begin(9600);

    myRTC.begin();
    setSyncProvider(myRTC.get);
    if (timeStatus() != timeSet)
        Serial.println("Unable to sync with the RTC");
    else
        Serial.println("RTC has set the system time");

    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    Serial.println("FastLED setup");
    FastLED.setBrightness(50);
    FastLED.clear();
    FastLED.show();

    // wm.resetSettings();
    wm.setHostname("SecretSantaClock");

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

        timeClient.begin();
        delay(1000);
        if (timeClient.update())
        {
            Serial.print("Adjust local clock");
            unsigned long epoch = timeClient.getEpochTime();
            setTime(epoch);
        }
        else
        {
            Serial.print("NTP Update does not WORK!!");
        }

        Serial.print(" now after ntp ? ");
        Serial.println(now());
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

    time_t utc = now();
    // Serial.print("utc ");
    // Serial.println(utc);
    time_t t = CE.toLocal(utc, &tcr);
    // Serial.print("local ");
    // Serial.println(t);

    int hours = hour(t);
    int mins = minute(t);
    int secs = second(t);

    char h1 = '0' + hours / 10;
    char h2 = '0' + hours - ((hours / 10) * 10);
    char m1 = '0' + mins / 10;
    char m2 = '0' + mins - ((mins / 10) * 10);
    char s1 = '0' + secs / 10;
    char s2 = '0' + secs - ((secs / 10) * 10);

    CRGB crgb = CRGB::Red;
    drawdigit(DIGIT1, crgb, h1);
    drawdigit(DIGIT2, crgb, h2);
    drawdigit(DIGIT3, crgb, m1);
    drawdigit(DIGIT4, crgb, m2);

    //crgb = CRGB::Green;
    crgb = CHSV(hue++, 255, 255);
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

    delay(100);
}
