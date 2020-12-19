#include <FS.h>          // this needs to be first, or it all crashes and burns...
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>

#ifdef ESP32
#include <SPIFFS.h>
#endif

const int BUFFER_SIZE = 1024;

boolean online = false;

const int customFieldLength = 40;
const int parameterStdFieldLength = 40;
const int parameterShortFieldLength = 6;

char mqtt_server[parameterStdFieldLength] = "homeassistant";
char mqtt_port[parameterShortFieldLength] = "1883";
char mqtt_username[parameterStdFieldLength] = "";
char mqtt_password[parameterStdFieldLength] = "";
char mqtt_topic[parameterStdFieldLength] = "secretsanta/clock";
char ntp_server[parameterStdFieldLength] = "europe.pool.ntp.org";

int mqtt_port_int = atoi(mqtt_port);

// WiFiManager, need it here as we want the clock to run in non-blocking mode without network connection too
WiFiManager wm;

WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, parameterStdFieldLength);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, parameterShortFieldLength);
WiFiManagerParameter custom_mqtt_username("username", "mqtt username", mqtt_username, parameterStdFieldLength);
WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, parameterStdFieldLength);
WiFiManagerParameter custom_mqtt_topic("topic", "mqtt topic", mqtt_topic, parameterStdFieldLength);
WiFiManagerParameter custom_ntp_server("ntp", "ntp server", ntp_server, parameterStdFieldLength);
// WiFiManagerParameter custom_ntp_timezone("api", "timezone string", timezone_string, parameterStdFieldLength);
WiFiManagerParameter custom_field;

WiFiClient espClient;
PubSubClient mqttClient(espClient);
long chipid;

void saveConfigCallback()
{
    Serial.println("Should save config");
}

void saveParamsCallback()
{
    Serial.println("Get Params:");
    Serial.print(custom_mqtt_server.getID());
    Serial.print(" : ");
    Serial.println(custom_mqtt_server.getValue());

    Serial.println("saving config");

    //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    mqtt_port_int = atoi(mqtt_port);
    strcpy(mqtt_username, custom_mqtt_username.getValue());
    strcpy(mqtt_password, custom_mqtt_password.getValue());
    strcpy(mqtt_topic, custom_mqtt_topic.getValue());
    strcpy(ntp_server, custom_ntp_server.getValue());

    DynamicJsonDocument json(BUFFER_SIZE);

    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_username"] = mqtt_username;
    json["mqtt_password"] = mqtt_password;
    json["mqtt_topic"] = mqtt_topic;
    json["ntp_server"] = ntp_server;

    Serial.println("About to save ...");
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
        Serial.println("failed to open config file for writing");
    }

    serializeJsonPretty(json, Serial);
    serializeJson(json, configFile);

    configFile.close();
    Serial.println("File closed");
    //end save
}

void setupSpiffs()
{
    //clean FS, for testing
    //  SPIFFS.format();

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

                DynamicJsonDocument json(BUFFER_SIZE);
                DeserializationError error = deserializeJson(json, configFile);

                serializeJson(json, Serial);
                if (!error)
                {
                    Serial.println("\nparsed json");

                    strcpy(mqtt_server, json["mqtt_server"]);
                    custom_mqtt_server.setValue(mqtt_server, parameterStdFieldLength);

                    strcpy(mqtt_port, json["mqtt_port"]);
                    custom_mqtt_port.setValue(mqtt_port, parameterShortFieldLength);
                    mqtt_port_int = atoi(mqtt_port);

                    strcpy(mqtt_username, json["mqtt_username"]);
                    custom_mqtt_username.setValue(mqtt_username, parameterStdFieldLength);

                    strcpy(mqtt_password, json["mqtt_password"]);
                    custom_mqtt_password.setValue(mqtt_password, parameterStdFieldLength);

                    strcpy(mqtt_topic, json["mqtt_topic"]);
                    custom_mqtt_topic.setValue(mqtt_topic, parameterStdFieldLength);

                    strcpy(ntp_server, json["ntp_server"]);
                    custom_ntp_server.setValue(ntp_server, parameterStdFieldLength);
                }
                else
                {
                    Serial.println("failed to load json config");
                    Serial.print(F("deserializeJson() failed with code "));
                    Serial.println(error.c_str());
                }
            }
        }
        else
        {
            Serial.println("no configfile found");
        }
    }
    else
    {
        Serial.println("failed to mount FS");
    }
    //end read
}

void reconnectMqtt()
{
    mqttClient.setServer(mqtt_server, mqtt_port_int);
    // Loop until we're reconnected
    //   while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection to ");
    Serial.print(mqtt_server);
    Serial.println("...");
    // Attempt to connect
    char mqtt_clientid[15];
    snprintf(mqtt_clientid, 14, "ESP%u", chipid);
    Serial.print("mqtt connect with clientid: ");
    Serial.println(mqtt_clientid);

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
}

void setup()
{
    Serial.begin(9600);
    Serial.println();

    setupSpiffs();

    //callbacks
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setSaveParamsCallback(saveParamsCallback);

    //params
    wm.addParameter(&custom_mqtt_server);
    wm.addParameter(&custom_mqtt_port);
    wm.addParameter(&custom_mqtt_username);
    wm.addParameter(&custom_mqtt_password);
    wm.addParameter(&custom_mqtt_topic);
    wm.addParameter(&custom_ntp_server);

    new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength, "placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type
    wm.addParameter(&custom_field);

    //reset settings - wipe credentials for testing
    // wm.resetSettings();

    wm.setConfigPortalBlocking(false);
    wm.setHostname("SecretSantaClock");

    online = wm.autoConnect("SecretSantaClockAP");

    if (!online)
    {
        Serial.println("Failed to connect");
        // ESP.restart();
    }
    else
    {
        Serial.println("connected...yeey :)");
    }

    // Serial.println("local ip");
    // Serial.println(WiFi.localIP());
    // Serial.println(WiFi.gatewayIP());
    // Serial.println(WiFi.subnetMask());
}

void loop()
{
    wm.process();
    Serial.print("my username is ");
    Serial.println(mqtt_username);

    if (online)
    {
        // make sure the MQTT client is connected
        if (!mqttClient.connected())
        {
            Serial.println("reconnecting");
            reconnectMqtt();
        }
        mqttClient.loop(); // Need to call this, otherwise mqtt breaks
    }

    delay(1000);
}
