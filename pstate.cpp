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
#include "pstate.h"

PState::PState()
{    

}

void PState::attachToNVS()
{
    prefs.begin(PSTATE_NAMESPACE, false); // select namespace

    uint8_t valid = prefs.getUChar("valid", 0);

    if (valid != PSTATE_VALID)
    {
        _setup = true;
        prefs.clear();       
    
        setChSett(1, CHMode::Manual, 0, 0);
        setChSett(2, CHMode::Manual, 0, 0);
        setChSett(3, CHMode::Manual, 0, 0);
        setChSett(4, CHMode::Manual, 0, 0);
    }
    else
    {
        _setup = false;
    }
}

void PState::setup(String wifi_ssid, String wifi_pwd)
{
    prefs.putString("wifi_ssid", wifi_ssid);
    prefs.putString("wifi_pwd", wifi_pwd);
    
    prefs.putUChar("valid", PSTATE_VALID);  
}

bool PState::requiresSetup()
{
    return _setup;
}

size_t PState::getWifiSSID(char* buff)
{
    return prefs.getString("wifi_ssid", buff, PSTATE_WIFI_MAX_LEN);
}

size_t PState::getWifiPwd(char* buff)
{
    return prefs.getString("wifi_pwd", buff, PSTATE_WIFI_MAX_LEN);
}

void PState::setChSett (int chan, CHMode mode, double freq, double goal)
{
    if (chan < 1 || chan > 4)
        return;

    char key[10] = "ch";
    
    sprintf(key+2, "%d%s", chan, "mode");    
    prefs.putUChar(key, (unsigned char) mode);

    sprintf(key+2, "%d%s", chan, "freq");  
    prefs.putDouble(key, freq);
    
    sprintf(key+2, "%d%s", chan, "goal");  
    prefs.putDouble(key, goal);
}

PState::CHMode PState::getChSettMode (int chan)
{
    if (chan < 1 || chan > 4)
        return CHMode::Manual;

    char key[10] = "ch";
    
    sprintf(key+2, "%d%s", chan, "mode");    
    return (CHMode)prefs.getUChar(key, 0);
}

double PState::getChSettFreq (int chan)
{
    if (chan < 1 || chan > 4)
        return 0;
        
    char key[10] = "ch";
    
    sprintf(key+2, "%d%s", chan, "freq");    
    return prefs.getDouble(key, 0);
}

double PState::getChSettGoal (int chan)
{
    if (chan < 1 || chan > 4)
        return 0;

    char key[10] = "ch";
    
    sprintf(key+2, "%d%s", chan, "goal");    
    return prefs.getDouble(key, 0);
}
