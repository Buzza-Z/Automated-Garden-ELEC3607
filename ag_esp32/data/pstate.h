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
#ifndef PSTATE_H
#define PSTATE_H
#include <Arduino.h>
#include <Preferences.h>
#include <nvs_flash.h>

#define PSTATE_NAMESPACE ("autog")

#define PSTATE_VALID (0xAB)

#define PSTATE_WIFI_MAX_LEN (128)

/*
 * A class that abstracts and simplifies the Preferences library,
 * allowing for trivial storage and retreval of WiFi settings and
 * channel settings.
 */
class PState
{
    private:
        Preferences prefs;
        bool _setup; // should the user setup the PState?

    public:
        PState();
        // Initialises the NVS peripheral and checks for corruption.
        void attachToNVS();

        // Changes the stored WiFi SSID and password.
        void setup(String wifi_ssid, String wifi_pwd);

        // Checks if the stored WiFi details are invalid.
        bool requiresSetup();

        // Puts the stored SSID into a buffer.
        size_t getWifiSSID(char* buff);

        // Puts the stored WiFi password into a buffer.
        size_t getWifiPwd(char* buff);

        enum CHMode { Manual = 0, Timed = 1, Smart = 2, Genius = 3 };

        // Changes the stored settings for a watering channel.
        void setChSett (int chan, CHMode mode, double freq, double goal);

        // Gets parts of the settings of a watering channel.
        CHMode getChSettMode (int chan);
        double getChSettFreq (int chan);
        double getChSettGoal (int chan);
};

#endif
