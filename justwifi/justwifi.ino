#include <FS.h>          // this needs to be first, or it all crashes and burns...
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson

#ifdef ESP32
#include <SPIFFS.h>
#endif

const int BUFFER_SIZE = 1024;

boolean online = false;

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40];
char mqtt_port[6] = "8080";
char api_token[32] = "YOUR_API_TOKEN";

// WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
WiFiManager wm;

// setup custom parameters
//
// The extra parameters to be configured (can be either global or just in the setup)
// After connecting, parameter.getValue() will get you the configured value
// id/name placeholder/prompt default length
WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
WiFiManagerParameter custom_api_token("api", "api token", api_token, 32);

//default custom static IP
char static_ip[16] = "10.0.1.56";
char static_gw[16] = "10.0.1.1";
char static_sn[16] = "255.255.255.0";

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback()
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
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
    strcpy(api_token, custom_api_token.getValue());

    DynamicJsonDocument json(BUFFER_SIZE);

    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["api_token"] = api_token;

    // json["ip"]          = WiFi.localIP().toString();
    // json["gateway"]     = WiFi.gatewayIP().toString();
    // json["subnet"]      = WiFi.subnetMask().toString();
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
    // SPIFFS.format();

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
                Serial.println("What is on file ? ");
                serializeJson(json, Serial);
                if (!error)
                {
                    Serial.println("\nparsed json");

                    strcpy(mqtt_server, json["mqtt_server"]);
                    strcpy(mqtt_port, json["mqtt_port"]);
                    strcpy(api_token, json["api_token"]);

                    // if(json["ip"]) {
                    //   Serial.println("setting custom ip from config");
                    //   strcpy(static_ip, json["ip"]);
                    //   strcpy(static_gw, json["gateway"]);
                    //   strcpy(static_sn, json["subnet"]);
                    //   Serial.println(static_ip);
                    // } else {
                    //   Serial.println("no custom ip in config");
                    // }
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

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(9600);
    Serial.println();

    setupSpiffs();

    //set config save notify callback
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setSaveParamsCallback(saveParamsCallback);

    //add all your parameters here
    wm.addParameter(&custom_mqtt_server);
    wm.addParameter(&custom_mqtt_port);
    wm.addParameter(&custom_api_token);

    // set static ip
    // IPAddress _ip,_gw,_sn;
    // _ip.fromString(static_ip);
    // _gw.fromString(static_gw);
    // _sn.fromString(static_sn);
    // wm.setSTAStaticIPConfig(_ip, _gw, _sn);

    //reset settings - wipe credentials for testing
    // wm.resetSettings();

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
    }
    // if (!wm.autoConnect("AutoConnectAP", "password"))
    // {
    //     Serial.println("failed to connect and hit timeout");
    //     delay(3000);
    //     // if we still have not connected restart and try all over again
    //     ESP.restart();
    //     delay(5000);
    // }

    // always start configportal for a little while
    // wm.setConfigPortalTimeout(60);
    // wm.startConfigPortal("AutoConnectAP","password");

    //if you get here you have connected to the WiFi


    //save the custom parameters to FS
    if (shouldSaveConfig)
    {
        Serial.println("saving config");

        DynamicJsonDocument json(BUFFER_SIZE);

        json["mqtt_server"] = mqtt_server;
        json["mqtt_port"] = mqtt_port;
        json["api_token"] = api_token;

        // json["ip"]          = WiFi.localIP().toString();
        // json["gateway"]     = WiFi.gatewayIP().toString();
        // json["subnet"]      = WiFi.subnetMask().toString();

        File configFile = SPIFFS.open("/config.json", "w");
        if (!configFile)
        {
            Serial.println("failed to open config file for writing");
        }

        serializeJsonPretty(json, Serial);
        serializeJson(json, configFile);

        configFile.close();
        //end save
        shouldSaveConfig = false;
    }

    Serial.println("local ip");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.gatewayIP());
    Serial.println(WiFi.subnetMask());
}

void loop()
{
    wm.process();
    Serial.print("my token is ");
    Serial.println(api_token);
    // put your main code here, to run repeatedly:
    delay(1000);
}