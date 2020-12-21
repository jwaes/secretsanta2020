#include <FS.h>          // this needs to be first, or it all crashes and burns...
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include <Timezone.h>    // https://github.com/JChristensen/Timezone
#include <DS3232RTC.h>   // https://github.com/JChristensen/DS3232RTC
#include <PubSubClient.h>
#include <Ticker.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <time.h>
#include <WiFiUdp.h>
#include <FastLED.h>

#ifdef ESP32
#include <SPIFFS.h>
#endif

const int BUFFER_SIZE = 1024;

boolean online = false;

const int customFieldLength = 40;
const int parameterStdFieldLength = 40;
const int parameterShortFieldLength = 6;
const int parameterColorFieldLength = 12;

char mqtt_server[parameterStdFieldLength] = "homeassistant";
char mqtt_port[parameterShortFieldLength] = "1883";
char mqtt_username[parameterStdFieldLength] = "";
char mqtt_password[parameterStdFieldLength] = "";
char mqtt_topic_state[parameterStdFieldLength] = "secretsanta/clock";
char mqtt_topic_set[parameterStdFieldLength] = "secretsanta/clock/set";
char ntp_server[parameterStdFieldLength] = "europe.pool.ntp.org";
char temp_offset[parameterShortFieldLength] = "-1.0";
char color1[parameterColorFieldLength] = "255,0,0";
char color2[parameterColorFieldLength] = "0,255,0";

int mqtt_port_int = atoi(mqtt_port);
float temp_offset_float = atof(temp_offset);

// WiFiManager, need it here as we want the clock to run in non-blocking mode without network connection too
WiFiManager wm;
const char HOSTNAME[parameterStdFieldLength] = "SecretSantaClock";
const char APNAME[parameterStdFieldLength] = "SecretSantaClockAP";
int timeout = 120;

WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, parameterStdFieldLength);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, parameterShortFieldLength);
WiFiManagerParameter custom_mqtt_username("username", "mqtt username", mqtt_username, parameterStdFieldLength);
WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, parameterStdFieldLength);
WiFiManagerParameter custom_mqtt_topic_state("topic_state", "mqtt topic", mqtt_topic_state, parameterStdFieldLength);
WiFiManagerParameter custom_mqtt_topic_set("topic_set", "mqtt topic", mqtt_topic_set, parameterStdFieldLength);
WiFiManagerParameter custom_ntp_server("ntp", "ntp server", ntp_server, parameterStdFieldLength);
WiFiManagerParameter custom_temp_offset("offset", "temperature correction", temp_offset, parameterShortFieldLength);
WiFiManagerParameter custom_color1("color1", "color 1", color1, parameterColorFieldLength);
WiFiManagerParameter custom_color2("color2", "color 2", color2, parameterColorFieldLength);

// WiFiManagerParameter custom_ntp_timezone("api", "timezone string", timezone_string, parameterStdFieldLength);
WiFiManagerParameter custom_field;

//MQTT stuff
WiFiClient espClient;
PubSubClient mqttClient(espClient);
Ticker mqttTicker;
int mqttFailCounter = 0;
long mqttIgnoreUntil = 0;

//Push button
#define TRIGGER_PIN D5
int button = 0;
boolean buttonPressed = false;
int buttonLockUntil = 0;

//LDR stuff
#define LDRPIN A0
float ldrValue;
int LDR;
float calcLDR;
float diffLDR = 25;
int bright = 100;

//NTP stuff
WiFiUDP ntpUDP;
int GTMOffset = 0; // SET TO UTC TIME
NTPClient timeClient(ntpUDP, ntp_server, GTMOffset * 60 * 60, 60 * 60 * 1000);
Ticker ntpTicker;
DS3232RTC myRTC;
float celsius = 0.0;

TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120}; // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};   // Central European Standard Time
Timezone CE(CEST, CET);
TimeChangeRule *tcr; //pointer to the time change rule, use to get TZ abbrev

//LED stuff
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

CRGB c1;
CRGB c2;

//modes
const char *const MODE_OFF = "OFF";
const char *const MODE_CLOCK = "CLOCK";
const char *const MODE_TEMP = "TEMP";
const char *const MODE_CLOCKTEMP = "CLOCKTEMP";
const char *const MODE_PCT = "PCT";

const char *mode = MODE_CLOCK;

const long MAXLONG = 2147483647;
long tempCounterMin = 0;
long tempCounterShow = MAXLONG;

//this is only called when we are connected
void saveConfigCallback()
{
    Serial.println(F("Should save config"));
    online = true;

    timeClient.setPoolServerName(ntp_server);

    delay(1000);
    updateTimeFromNTP();
}

void saveParamsCallback()
{
    Serial.println(F("Get Params:"));
    Serial.print(custom_mqtt_server.getID());
    Serial.print(" : ");
    Serial.println(custom_mqtt_server.getValue());

    Serial.println(F("saving config"));

    //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    mqtt_port_int = atoi(mqtt_port);
    strcpy(mqtt_username, custom_mqtt_username.getValue());
    strcpy(mqtt_password, custom_mqtt_password.getValue());
    strcpy(mqtt_topic_state, custom_mqtt_topic_state.getValue());
    strcpy(mqtt_topic_set, custom_mqtt_topic_set.getValue());
    strcpy(ntp_server, custom_ntp_server.getValue());
    strcpy(temp_offset, custom_temp_offset.getValue());
    temp_offset_float = atof(temp_offset);
    strcpy(color1, custom_color1.getValue());
    strcpy(color2, custom_color2.getValue());

    DynamicJsonDocument json(BUFFER_SIZE);

    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_username"] = mqtt_username;
    json["mqtt_password"] = mqtt_password;
    json["mqtt_topic_state"] = mqtt_topic_state;
    json["mqtt_topic_set"] = mqtt_topic_set;
    json["ntp_server"] = ntp_server;
    json["temp_offset"] = temp_offset;
    json["color1"] = color1;
    json["color2"] = color2;

    Serial.println(F("About to save ..."));
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
        Serial.println(F("failed to open config file for writing"));
    }

    serializeJsonPretty(json, Serial);
    serializeJson(json, configFile);

    configFile.close();
    Serial.println(F("File closed"));
    //end save
    setBaseColors();
}

void setupSpiffs()
{
    //clean FS, for testing
    //  SPIFFS.format();

    //read configuration from FS json
    Serial.println(F("mounting FS..."));

    if (SPIFFS.begin())
    {
        Serial.println(F("mounted file system"));
        if (SPIFFS.exists("/config.json"))
        {
            //file exists, reading and loading
            Serial.println(F("reading config file"));
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile)
            {
                Serial.println(F("opened config file"));
                size_t size = configFile.size();

                DynamicJsonDocument json(BUFFER_SIZE);
                DeserializationError error = deserializeJson(json, configFile);

                serializeJson(json, Serial);
                if (!error)
                {
                    Serial.println(F("\nparsed json"));

                    strcpy(mqtt_server, json["mqtt_server"]);
                    custom_mqtt_server.setValue(mqtt_server, parameterStdFieldLength);

                    strcpy(mqtt_port, json["mqtt_port"]);
                    custom_mqtt_port.setValue(mqtt_port, parameterShortFieldLength);
                    mqtt_port_int = atoi(mqtt_port);

                    strcpy(mqtt_username, json["mqtt_username"]);
                    custom_mqtt_username.setValue(mqtt_username, parameterStdFieldLength);

                    strcpy(mqtt_password, json["mqtt_password"]);
                    custom_mqtt_password.setValue(mqtt_password, parameterStdFieldLength);

                    strcpy(mqtt_topic_state, json["mqtt_topic_state"]);
                    custom_mqtt_topic_state.setValue(mqtt_topic_state, parameterStdFieldLength);

                    strcpy(mqtt_topic_set, json["mqtt_topic_set"]);
                    custom_mqtt_topic_set.setValue(mqtt_topic_set, parameterStdFieldLength);

                    strcpy(ntp_server, json["ntp_server"]);
                    custom_ntp_server.setValue(ntp_server, parameterStdFieldLength);

                    strcpy(temp_offset, json["temp_offset"]);
                    custom_temp_offset.setValue(temp_offset, parameterShortFieldLength);

                    strcpy(color1, json["color1"]);
                    custom_color1.setValue(color1, parameterColorFieldLength);

                    strcpy(color2, json["color2"]);
                    custom_color2.setValue(color2, parameterColorFieldLength);
                }
                else
                {
                    Serial.println(F("failed to load json config"));
                    Serial.print(F("deserializeJson() failed with code "));
                    Serial.println(error.c_str());
                }
            }
        }
        else
        {
            Serial.println(F("no configfile found"));
        }
    }
    else
    {
        Serial.println(F("failed to mount FS"));
    }
    //end read
}

void reconnectMqtt()
{
    mqttClient.setServer(mqtt_server, mqtt_port_int);
    mqttClient.setCallback(mqttCallback);

    if (mqttFailCounter < 5)
    {
        Serial.print("Attempting MQTT connection to ");
        Serial.print(mqtt_server);
        Serial.println(F("..."));

        if (mqttClient.connect(HOSTNAME, mqtt_username, mqtt_password))
        {
            Serial.println(F("MQTT connected."));
            Serial.print(F("MQTT subscribe to "));
            Serial.print(mqtt_topic_set);
            if (mqttClient.subscribe(mqtt_topic_set))
            {
                Serial.println(F(" successfull"));
            }
            else
            {
                Serial.println(F(" failed"));
            }
            postMqttStatus();
        }
        else
        {
            Serial.println(F("Connection to MQTT server failed"));
            mqttFailCounter++;
            mqttIgnoreUntil = now() + (10 * 60);
        }
    }
    else
    {
        if (now() > mqttIgnoreUntil)
        {
            mqttFailCounter = 0;
        }
    }
}

void postMqttStatus()
{
    if (mqttClient.connected())
    {
        Serial.println(F("posting Mqtt status"));
        long rssi = WiFi.RSSI();

        StaticJsonDocument<BUFFER_SIZE> json;
        json["rssi"] = rssi;
        json["mode"] = mode;
        char buffer[BUFFER_SIZE];
        size_t n = serializeJson(json, buffer);
        mqttClient.publish(mqtt_topic_state, buffer, n);

        serializeJsonPretty(json, Serial);
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    Serial.print(F("Message arrived ["));
    Serial.print(topic);
    Serial.print(F("] "));

    char message[length + 1];
    for (int i = 0; i < length; i++)
    {
        message[i] = (char)payload[i];
    }
    message[length] = '\0';
    Serial.println(message);

    if (!processJson(message))
    {
        return;
    }

    postMqttStatus();
}

bool processJson(char *message)
{
    StaticJsonDocument<BUFFER_SIZE> doc;

    auto error = deserializeJson(doc, message);
    if (error)
    {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(error.c_str());
        return false;
    }

    if (doc.containsKey("mode"))
    {
        if (strcmp(doc["mode"], MODE_CLOCK) == 0)
        {
            mode = MODE_CLOCK;
        }
        else if (strcmp(doc["mode"], MODE_OFF) == 0)
        {
            mode = MODE_OFF;
        }
        else if (strcmp(doc["mode"], MODE_CLOCKTEMP) == 0)
        {
            mode = MODE_CLOCKTEMP;
        }
        else if (strcmp(doc["mode"], MODE_TEMP) == 0)
        {
            mode = MODE_TEMP;
        }
        else if (strcmp(doc["mode"], MODE_PCT) == 0)
        {
            mode = MODE_PCT;
        }
        else if (strcmp(doc["mode"], "NTP") == 0)
        {
            updateTimeFromNTP();
        }

        FastLED.clear();
        FastLED.show();
        return true;
    }
}

static tm getDateTimeByParams(long time)
{
    struct tm *newtime;
    const time_t tim = time;
    newtime = localtime(&tim);
    return *newtime;
}

void updateTimeFromNTP()
{
    if (online)
    {
    }
    if (timeClient.update())
    {
        Serial.println(F("Adjust local clock"));
        unsigned long epoch = timeClient.getEpochTime();
        Serial.print(F("NTP epoch time: "));
        Serial.println(epoch);
        setTime(epoch);
        Serial.println(F("Going to set RTC"));
        myRTC.set(epoch);
        Serial.print(F("Time on RTC is now: "));
        Serial.println(myRTC.get());
    }
    else
    {
        Serial.print(F("NTP update failed"));
    }
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
    // Serial.print(F("Going to draw a : "));
    // Serial.print(n);
    // Serial.print(F(" on position "));
    // Serial.println(offset);

    String top = "02356789x°";
    String top_right = "01234789x°";
    String bottom_right = "013456789o";
    String bottom = "0235689o";
    String top_left = "045689x°";
    String bottom_left = "0268o";
    String middle = "2345689xo°";

    setLedSegment(0 + offset, crgb, middle, n);
    setLedSegment(3 + offset, crgb, bottom_right, n);
    setLedSegment(6 + offset, crgb, bottom, n);
    setLedSegment(9 + offset, crgb, bottom_left, n);
    setLedSegment(12 + offset, crgb, top_left, n);
    setLedSegment(15 + offset, crgb, top, n);
    setLedSegment(18 + offset, crgb, top_right, n);
}

void showTime()
{
    time_t utc = now();
    time_t t = CE.toLocal(utc, &tcr);

    int hours = hour(t);
    int mins = minute(t);
    int secs = second(t);

    char h1 = '0' + hours / 10;
    char h2 = '0' + hours - ((hours / 10) * 10);
    char m1 = '0' + mins / 10;
    char m2 = '0' + mins - ((mins / 10) * 10);
    char s1 = '0' + secs / 10;
    char s2 = '0' + secs - ((secs / 10) * 10);

    CRGB crgb = c1;
    drawdigit(DIGIT1, crgb, h1);
    drawdigit(DIGIT2, crgb, h2);
    drawdigit(DIGIT3, crgb, m1);
    drawdigit(DIGIT4, crgb, m2);

    crgb = CHSV(hue++, 255, 255);
    setNumberOfLeds(LOGO, DIGIT1, crgb);
    setNumberOfLeds(DOT1, 1, crgb);
    setNumberOfLeds(DOT2, 1, crgb);

    FastLED.setBrightness(150);
}

void showTemp()
{
    int c = (celsius + temp_offset_float) * 10;

    char buffer[7];
    itoa(c, buffer, 10);

    char d1 = 0 + buffer[0];
    char d2 = 0 + buffer[1];
    char d3 = 0 + buffer[2];
    char d4 = char(176);

    CRGB crgb = c2;
    drawdigit(DIGIT1, crgb, d1);
    drawdigit(DIGIT2, crgb, d2);
    drawdigit(DIGIT3, crgb, d3);
    drawdigit(DIGIT4, crgb, d4);

    crgb = CHSV(hue++, 255, 255);
    setNumberOfLeds(LOGO, DIGIT1, crgb);
    setNumberOfLeds(DOT2, 1, crgb);
    setNumberOfLeds(DOT1, 1, CRGB::Black);
}

CRGB getCRGBFromString(char *rgbStr)
{
    int rgb[3];
    int i = 0;
    char *p = rgbStr;
    char *str;
    while ((str = strtok_r(p, ",", &p)) != NULL)
    {
        rgb[i] = atoi(str);
        // Serial.print(i);
        // Serial.print(F(":"));
        // Serial.println((int) rgb[i]);

        i++;
    }

    // int r = rgb[0];
    // int g = rgb[1];
    // int b = rgb[2];
    // Serial.print("about to create crgb ");
    // Serial.print(r);
    // Serial.print(",");
    // Serial.print(g);
    // Serial.print(",");
    // Serial.println(b);
    CRGB output = CRGB(rgb[0], rgb[1], rgb[2]);
    // Serial.print("infunc avg ");
    // Serial.println(output.getAverageLight());
    return output;
}

void setBaseColors()
{
    c1 = getCRGBFromString(color1);
    c2 = getCRGBFromString(color2);
}

bool checkBoundSensor(float newValue, float prevValue, float maxDiff)
{
    return newValue < prevValue - maxDiff || newValue > prevValue + maxDiff;
}

void setup()
{
    Serial.begin(9600);
    Serial.println();

    pinMode(LDRPIN, INPUT);
    pinMode(TRIGGER_PIN, INPUT_PULLUP);

    setupSpiffs();

    timeClient.begin();
    myRTC.begin();
    setSyncProvider(myRTC.get);
    if (timeStatus() != timeSet)
        Serial.println(F("Unable to sync with the RTC"));
    else
    {
        Serial.print(F("RTC has set the system time: "));
        Serial.println(now());
    }
    celsius = myRTC.temperature() / 4.0;

    //callbacks
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setSaveParamsCallback(saveParamsCallback);

    //params
    wm.addParameter(&custom_mqtt_server);
    wm.addParameter(&custom_mqtt_port);
    wm.addParameter(&custom_mqtt_username);
    wm.addParameter(&custom_mqtt_password);
    wm.addParameter(&custom_mqtt_topic_state);
    wm.addParameter(&custom_ntp_server);
    wm.addParameter(&custom_temp_offset);
    wm.addParameter(&custom_color1);
    wm.addParameter(&custom_color2);

    new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength, "placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type
    wm.addParameter(&custom_field);

    //reset settings - wipe credentials for testing
    // wm.resetSettings();
    // ESP.reset();

    wm.setConfigPortalBlocking(false);
    wm.setHostname(HOSTNAME);

    online = wm.autoConnect(APNAME);

    if (!online)
    {
        Serial.println(F("Failed to connect"));
        // ESP.restart();
    }
    else
    {
        Serial.println(F("connected...yeey :)"));
        updateTimeFromNTP();
    }

    //every 5 minutes = 300
    mqttTicker.attach(300, postMqttStatus);
    ntpTicker.attach(600, updateTimeFromNTP);

    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    Serial.println(F("FastLED setup"));
    FastLED.setBrightness(50);
    FastLED.clear();
    FastLED.show();

    setBaseColors();
}

void loop()
{
    wm.process();

    if (online)
    {
        // make sure the MQTT client is connected
        if (!mqttClient.connected())
        {
            // Serial.println(F("reconnecting"));
            reconnectMqtt();
        }
        mqttClient.loop(); // Need to call this, otherwise mqtt breaks
    }

    if (mode == MODE_CLOCK)
    {
        showTime();
    }
    else if (mode == MODE_TEMP)
    {
        celsius = myRTC.temperature() / 4.0;
        showTemp();
    }
    else if (mode == MODE_CLOCKTEMP)
    {
        if (now() > tempCounterMin)
        {
            celsius = myRTC.temperature() / 4.0;
            tempCounterMin = now() + 60;
            tempCounterShow = now() + 5;
            Serial.println(F("Resetting timer for temp"));
            Serial.print(F("tempCounterMin: "));
            Serial.println(tempCounterMin);
            Serial.print(F("tempCounterShow: "));
            Serial.println(tempCounterShow);
        }
        if (now() < tempCounterShow)
        {
            showTemp();
        }
        else
        {
            showTime();
        }
    }
    else
    {
        // implies MODE_OFF
        FastLED.clear();
    }

    if (now() % 100 == 0)
    {
        if (now() % 60 == 0)
        {
            Serial.println(F("."));
        }
        else
        {
            Serial.print(F("."));
        }
    }

    int newLDR = analogRead(LDRPIN);
    if (checkBoundSensor(newLDR, LDR, diffLDR))
    {
        LDR = newLDR;
        //      sendState();
    }
    // Serial.print(F("Lightsensor ; "));
    // Serial.println(LDR);
    bright = (1 - ((LDR - 100.) / 1024.)) * 150.;
    // Serial.print(F("proposed brightness "));
    // Serial.println(bright);
    FastLED.setBrightness(bright);

    button = digitalRead(TRIGGER_PIN);
    // Serial.print(F("Button: "));
    // Serial.println(button);

    if (button == LOW)
    {
        if (buttonPressed == false)
        {
            buttonPressed = true;
            // buttonLockUntil = now() + 10;
            // Serial.println(F("Button pressed"));
            // wm.setConfigPortalTimeout(timeout);
            // wm.startConfigPortal(APNAME);
            fill_solid(leds, NUM_LEDS, CRGB::White);
            FastLED.show();
            Serial.println(F("Resetting ... "));
            wm.resetSettings();
            Serial.println(F("Reset wm settings ... going to format"));
            SPIFFS.format();
            Serial.println(F("Format complet ... resetting ESP ..."));
            ESP.reset();
            delay(1000);

            // } else {
            //     if(now() > buttonLockUntil){
            //         Serial.println(F("Button press timed out"));
            //         buttonPressed = false;
            //     } else {
            //         Serial.println(F("Button considered pressed"));
            //         fill_solid(leds, NUM_LEDS, CRGB::White);
            //     }
            // }
        }
    }

    FastLED.show();

    delay(1000);
}
