/**
    Copyright © 2019 Sam Shahrestani, Zachary Buzza
    
    Permission is hereby granted, free of charge, to any person obtaining a copy of
    this software and associated documentation files (the “Software”), to deal in the
    Software without restriction, including without limitation the rights to use,
    copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
    Software, and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
    FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
    COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
    AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <WiFi.h>
#include <BluetoothSerial.h>
#include <WebServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>

#include "ArduinoJson.hpp"
#include "wmc.h"
#include "pstate.h"
#include "wtrevent.h"

// class instances
MotorBoard wmc;
PState pstate; // persistent state
BluetoothSerial BTSerial;
AsyncWebServer server(80);

// external globals
// watering event management
extern volatile bool stopFlag[4];

// constants
#define UTC_TIMEZONE (10)
#define DST_OBSERVED (0)
#define BT_NAME ("Team29AG")
#define MDNS_NAME ("team29ag")

#define LED_BUILTIN (2)

/*
 * Initialises all components of the system, including hardware peripherals.
 */
void setup() 
{
    delay(100);
    Serial.begin(115200);
    Serial.setTimeout(200);
    pinMode(LED_BUILTIN, OUTPUT);

    // broadcast bluetooth identity
    BTSerial.begin(BT_NAME); //Bluetooth device name
    // set pin esp_bt_gap_set_pin(esp_bt_pin_type_t pin_type, uint8_t pin_code_len, esp_bt_pin_code_t pin_code);

    // enable HAL components after initial bootup
    wmc.setupPeripherals();    
    Serial.print("[INTERNAL] Soil sensor I2C init: ");
    Serial.println(wmc.isSSconnected() ? "success" : "failure");
    
    pstate.attachToNVS();
    
    // check if settings are set
    if (pstate.requiresSetup())
    {
        // run setup... faking it for now
        char* ssid = "ASUS_Guest1";
        char* pwd = "elephant";
        pstate.setup(ssid, pwd);
        Serial.println("[INTERNAL] Ran setup");
    }

    if(!SPIFFS.begin(true))
    {
      Serial.println("[INTERNAL] SPIFFS was corrupted, flash assumed bad");
      delay(5000);
      ESP.restart();
    }
    
    attemptWifiConnect();
}

/*
 * Attempts to connect to WiFi, and on success establishes a web server and mDNS server.
 */
void attemptWifiConnect()
{   
    // connect to configured wifi network
    char ssid_buf[PSTATE_WIFI_MAX_LEN];
    char pwd_buf[PSTATE_WIFI_MAX_LEN];
    pstate.getWifiSSID(ssid_buf);
    pstate.getWifiPwd(pwd_buf);
    
    int timeout = 0;
    WiFi.disconnect();
    delay(100);
    WiFi.begin(ssid_buf, pwd_buf);
    Serial.println("[INTERNAL] Connecting to WiFi");
    printf("[INTERNAL] SSID=%s\n", ssid_buf);
    printf("[INTERNAL] Password=%s\n", pwd_buf);
    while (WiFi.status() != WL_CONNECTED || timeout > 20) {
        delay(500);
        timeout++;

        if (timeout > 20)
        {
            Serial.println("[INTERNAL] WiFi timeout!");
            WiFi.disconnect(); 
            digitalWrite(LED_BUILTIN, HIGH);
            return;
        }
    }
    Serial.println("[INTERNAL] WiFi Connected");

    
    // configure time from ntp server
    configTime(UTC_TIMEZONE * 3600, DST_OBSERVED * 3600, "pool.ntp.org");

    // configure http server
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html");
    });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/script.js");
    });

    server.on("/int", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        int params_num = request->params();
        printf("[INTERNAL] Get request with %d params\n", params_num);
     
        for (int i = 0; i < params_num; i++) 
        {
            AsyncWebParameter* param = request->getParam(i);
            if (param->name() == "cmd")
            {                
                processUSRCommands(param->value());
            }
        }  
        request->send(200, "text/plain", "got cmd");
    });
    server.on("/poll", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(200, "text/plain", getPollData().c_str());
    });

    server.begin();
    
    if (!MDNS.begin(MDNS_NAME)) 
    {
        Serial.println("[INTERNAL] Error setting up MDNS responder!");
    }    
    else
    {
        MDNS.addService("http", "tcp", 80);
    }
    digitalWrite(LED_BUILTIN, LOW);
 
    delay(5000);
}

/*
 * Check for new commands from the Bluetooth Master or Serial.
 */
void loop() 
{
    // put your main code here, to run repeatedly:
    processDBGCommands();
    processBTCommands();
    
    delay(30);
}

/*
 * Checks for a new debug command from the Serial and executes it, if present.
 */
void processDBGCommands()
{
    // debug commands over Serial
    while (Serial.available() > 0)
    {        
        String cmd = Serial.readStringUntil('\n');
        
        if (cmd.startsWith("RST"))
        {
            ESP.restart();
        }
        
        else if (cmd.startsWith("NVSCLR"))
        {
            nvs_flash_erase();
            Serial.println("[INTERNAL] Cleared NVS partition");
            ESP.restart();
        }
        // T - Test Sequences
        else if (cmd.startsWith("T1"))
        {
            Serial.println("[TEST1] Start");delay(10);
            // set ch1 to manual            
            pstate.setChSett(1, PState::CHMode::Manual, 0, 0);

            // create an event for watering
            newWtrEvent(1);

            // SIMULATE the user hitting stop after a time
            Serial.println("[TEST1] Delaying...");delay(10);
            delay(5000);
            
            Serial.println("[TEST1] Setting stop flag");delay(10);
            stopFlag[0] = true;
        }
        else if (cmd.startsWith("T2"))
        {
            Serial.println("[TEST2] Start");delay(10);
            // set ch1 to manual            
            pstate.setChSett(1, PState::CHMode::Timed, 2000, 1000); // on for half a second then off

            // create an event for watering
            newWtrEvent(1);

            // SIMULATE the user hitting stop after a time
            Serial.println("[TEST2] Delaying...");delay(10);
            delay(5000);
            
            Serial.println("[TEST2] Setting stop flag");delay(10);
            stopFlag[0] = true;
        }
    }
}

/*
 * Processes a command string and executes it.
 */
void processUSRCommands(String cmd)
{
    // respond to commands
    Serial.print("[INTERNAL] New cmd: ");
    Serial.println(cmd);

    // W - WiFi Change
    if (cmd.startsWith("W"))
    {
        int sep = cmd.indexOf(';');
        pstate.setup(cmd.substring(1, sep), cmd.substring(sep+1));
        
        attemptWifiConnect();
    }
    
    // S - Channel Setting Change
    if (cmd.startsWith("S"))
    {            
        int chan = -1;
        int mde = 0;
        double freq = 0;
        double goal = 0;
        sscanf(cmd.c_str(), "S%d,%d,%lf,%lf", &chan, &mde, &freq, &goal);            
        
        if (chan > 0 && chan < 5 
            && mde >= PState::CHMode::Manual && mde <= PState::CHMode::Genius
            && freq >= goal)
        {
            printf("[INTERNAL] Setting channel %d to mode=%d, freq=%lf, goal=%lf\n", chan, mde, freq, goal);
            stopFlag[chan-1] = true;
            pstate.setChSett(chan, (PState::CHMode) mde, freq, goal); // on for half a second then off
        }
    }

    // A - Actions
    // AH - Halt Channel
    else if (cmd.startsWith("AH"))
    {
        // halt action
        int chan = -1;
        sscanf(cmd.c_str(), "AH%d", &chan);

        if (chan > 0 && chan < 5)
            stopFlag[chan-1] = true;
    }
    // AA - Arm Channel
    else if (cmd.startsWith("AA"))
    {
        // arm action -> create event for watering
        int chan = -1;
        sscanf(cmd.c_str(), "AA%d", &chan);

        if (chan > 0 && chan < 5)
        {
            printf("[INTERNAL] Arming channel %d\n", chan);
            newWtrEvent(chan);
        }
    }

    // P - Poll User data (sensors, settings etc) for bluetooth
    else if (cmd.startsWith("P"))
    {            
        sendBTResponse();
    }
    
}

/*
 * Checks for commands from the Bluetooth Master and submits them for processing.
 */
void processBTCommands()
{        
    if (BTSerial.available() > 0)
    {
        // respond to commands
        String cmd = BTSerial.readStringUntil('\n');
        cmd.trim();
        processUSRCommands(cmd);
    }
}

/*
 * Gets the latest JSON poll data to the Bluetooth Master.
 */
void sendBTResponse()
{
    // send the latest sensor data here...
    if (BTSerial.hasClient())
    {        
        BTSerial.println(getPollData());
    }
}

/*
 * Gets the latest sensor data from the MotorBoard, encodes it as JSON,
 * and returns a String containing a serialization of the JSON object.
 */
String getPollData()
{
    String response;
    
    using namespace ArduinoJson;

    DynamicJsonDocument poll(512);  

    // sensor data
    poll["temp"] = wmc.getSSTemp();
    poll["moisture"] = wmc.getSSMoisture();
    poll["water"] = wmc.getWaterDelivered();

    // current settings
    JsonObject ch1 = poll.createNestedObject("ch1");
    ch1["status"] = !stopFlag[0];
    ch1["mode"] = (uint8_t)pstate.getChSettMode(1);
    ch1["frequency"] = pstate.getChSettFreq(1);
    ch1["goal"] = pstate.getChSettGoal(1);
    
    JsonObject ch2 = poll.createNestedObject("ch2");
    ch2["status"] = !stopFlag[1];
    ch2["mode"] = (uint8_t)pstate.getChSettMode(2);
    ch2["frequency"] = pstate.getChSettFreq(2);
    ch2["goal"] = pstate.getChSettGoal(2);
    
    JsonObject ch3 = poll.createNestedObject("ch3");
    ch3["status"] = !stopFlag[2];
    ch3["mode"] = (uint8_t)pstate.getChSettMode(3);
    ch3["frequency"] = pstate.getChSettFreq(3);
    ch3["goal"] = pstate.getChSettGoal(3);
    
    JsonObject ch4 = poll.createNestedObject("ch4");
    ch4["status"] = !stopFlag[3];
    ch4["mode"] = (uint8_t)pstate.getChSettMode(4);
    ch4["frequency"] = pstate.getChSettFreq(4);
    ch4["goal"] = pstate.getChSettGoal(4);
    
    serializeJson(poll, response);

    return response;
}
