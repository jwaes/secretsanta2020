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
#include <PubSubClient.h>

WiFiManager wm;
// WiFiManagerParameter custom_mqtt_server("server", "mqtt server", "servername", 40);
// WiFiManagerParameter custom_mqtt_port("port", "mqtt port", "1234", 6);

char mqtt_server[40] = "homeassistant";
char mqtt_portStr[6] = "1883";
char mqtt_username[32] = "hamqtt";
char mqtt_password[32] = "a7iyLj0ktkniru3NgnXf";
char mqtt_topic[32] = "secretsanta/clock";

int mqtt_port = atoi(mqtt_portStr);
String readStr;
long chipid;
bool online = false;

const int BUFFER_SIZE = 300;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

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

void saveConfigJson()
{
    //save the custom parameters to FS
    Serial.println("saving config");
    //   DynamicJsonBuffer jsonBuffer;
    //   JsonObject& json = jsonBuffer.createObject();
    StaticJsonDocument<BUFFER_SIZE> json;
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_portStr;
    json["mqtt_username"] = mqtt_username;
    json["mqtt_password"] = mqtt_password;
    json["mqtt_topic"] = mqtt_topic;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
        Serial.println("failed to open config file for writing");
    }

    //   json.printTo(Serial);
    //   Serial.println();
    //   json.printTo(configFile);

    if (serializeJson(json, configFile) == 0)
    {
        Serial.println(F("Failed to write to file"));
    }

    configFile.close();
    //end save
}

void reconnectMqtt()
{

    // Loop until we're reconnected
    //   while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection to ");
    Serial.print(mqtt_server);
    Serial.println("...");
    // Attempt to connect
    char mqtt_clientid[15];
    snprintf(mqtt_clientid, 14, "ESP%u", chipid);

    if (mqttClient.connect(mqtt_clientid, mqtt_username, mqtt_password))
    {
        Serial.println("MQTT connected.");
        long rssi = WiFi.RSSI();

        //     char buf[50];
        //     sprintf(buf, "ESP: %u Connected @ %i dBm", chipid, rssi);
        //     char topic_buf[50];
        //     sprintf(topic_buf, mqtt_topic, chipid);
        //     mqttClient.publish(topic_buf, buf);

        // send proper JSON startup message
        StaticJsonDocument<BUFFER_SIZE> json;

        //   DynamicJsonBuffer jsonBuffer;
        //   JsonObject& json = jsonBuffer.createObject();
        json["id"] = chipid;
        json["rssi"] = rssi;
        json["message"] = "Sensor startup";
        //   char buf[110];
        //   json.printTo(buf, sizeof(buf));

        //   Serial.print("Publish message: ");
        //   Serial.println(buf);

        //   char topic_buf[50];
        //   sprintf(topic_buf, mqtt_topic, chipid);
        //   mqttClient.publish(topic_buf, buf);
        char buffer[256];
        size_t n = serializeJson(json, buffer);
        mqttClient.publish("secretsanta/clocktest", buffer, n);
    }

    else
    {
        Serial.print("failed, rc=");
        Serial.print(mqttClient.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
    }
    //   }
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

      //clean FS, for testing
      Serial.println("formatting FS ...");
  SPIFFS.format();

    //read configuration from FS json
    Serial.println("mounting FS...");

    if (SPIFFS.begin())
    {
        Serial.println("mounted file system");
        if (SPIFFS.exists("/config.json"))
        {
            //file exists, reading and loading
            Serial.println("reading config file");
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile)
            {
                Serial.println("opened config file");
                size_t size = configFile.size();

                StaticJsonDocument<BUFFER_SIZE> json;
                // Deserialize the JSON document
                DeserializationError error = deserializeJson(json, configFile);
                if (error)
                    Serial.println(F("Failed to read file, using default configuration"));

                strcpy(mqtt_server, json["mqtt_server"]);
                strcpy(mqtt_portStr, json["mqtt_port"]);
                mqtt_port = atoi(mqtt_portStr);
                strcpy(mqtt_username, json["mqtt_username"]);
                strcpy(mqtt_password, json["mqtt_password"]);
                strcpy(mqtt_topic, json["mqtt_topic"]);
            }
        }
        else
        {
            Serial.println("/config.json does not exist, creating");
            saveConfigJson(); // saving the hardcoded default values
        }
    }
    else
    {
        Serial.println("failed to mount FS");
    }
    //end read

    wm.setSaveConfigCallback(saveConfigCallback);

    wm.resetSettings();
    wm.setHostname("SecretSantaClock");

    // wm.setAPCallback(configModeCallback);
    wm.setConfigPortalBlocking(false);

    online = wm.autoConnect("SecretSantaClockAP");

    if (!online)
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
    // wm.startConfigPortal("SecretSantaClockAP");

    //hardcoded ... should be a button
    boolean startConfigPortal = true;

    if (startConfigPortal)
    {

        Serial.println("setting parameters ");
        
        WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
        WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_portStr, 6);
        WiFiManagerParameter custom_mqtt_username("username", "mqtt username", mqtt_username, 32);
        WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, 32);
        WiFiManagerParameter custom_mqtt_topic("topic", "mqtt topic", mqtt_topic, 32);

        wm.addParameter(&custom_mqtt_server);
        wm.addParameter(&custom_mqtt_port);
        wm.addParameter(&custom_mqtt_username);
        wm.addParameter(&custom_mqtt_password);
        wm.addParameter(&custom_mqtt_topic);

        // If the user requests it, start the wifimanager
        wm.startConfigPortal("SecretSantaClockAP");

        if (shouldSaveConfig)
        {
            // read the updated parameters
            strcpy(mqtt_server, custom_mqtt_server.getValue());
            strcpy(mqtt_portStr, custom_mqtt_port.getValue());
            mqtt_port = atoi(mqtt_portStr);
            strcpy(mqtt_username, custom_mqtt_username.getValue());
            strcpy(mqtt_password, custom_mqtt_password.getValue());
            strcpy(mqtt_topic, custom_mqtt_topic.getValue());

            saveConfigJson();
            shouldSaveConfig = false;
        }
    }

    mqttClient.setServer(mqtt_server, mqtt_port);
}

void loop()
{
    // because we are not blocking ...
    wm.process();

    // if (online)
    // {
    //     // make sure the MQTT client is connected
    //     if (!mqttClient.connected())
    //     {
    //         reconnectMqtt();
    //     }
    //     mqttClient.loop(); // Need to call this, otherwise mqtt breaks
    // }

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
