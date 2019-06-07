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
#include "wtrevent.h"

#include "wmc.h"
#include "pstate.h"

// class instances
extern MotorBoard wmc;
extern PState pstate; // persistent state

// watering event management
volatile bool stopFlag[4] = {true,true,true,true};

// Delays a watering event task for a predefined amount of time.
void _tick()
{
    // this doesnt busy wait, it sacrifices a time slice, so we dont waste cpu time
    vTaskDelay(WTREVENT_POLLTIME);     
}

// Used for timing purposes, replace with anything operating in milliseconds
int64_t IRAM_ATTR waterTime()
{
    // this is the same as the core "millis" function, 
    // but avoids overflow issues when packing 64-bit int into a ulong (32-bit on xtensa)
    return esp_timer_get_time() / 1000ULL;    
}

// Generic behaviour of a manual (user controlled) watering event
void _manualWtrEvent(void* pvParameters)
{
    int chan = ((wtrEventParams*)pvParameters)->chan;
    wmc.setMotor(chan, true);
    while (!stopFlag[chan-1])
    {
        _tick();
    }
    wmc.setMotor(chan, false); 

    vPortFree(pvParameters);
    vTaskDelete(NULL);
}

// Generic behaviour of a timed watering event.
void _timedWtrEvent(void* pvParameters)
{
    int chan = ((wtrEventParams*)pvParameters)->chan;
    double t_on = pstate.getChSettGoal(chan);
    double t_off = pstate.getChSettFreq(chan) - t_on;

    while (!stopFlag[chan-1])
    {
        int64_t start = waterTime();
        
        wmc.setMotor(chan, false); 
        while (!stopFlag[chan-1] && (start+t_off) > waterTime())
        {
            _tick();
        }
        
        start = waterTime();
        wmc.setMotor(chan, true);
        while (!stopFlag[chan-1] && (start+t_on) > waterTime())
        {
            _tick(); 
        }
        
        _tick();
    }

    wmc.setMotor(chan, false); 
    vPortFree(pvParameters);
    vTaskDelete(NULL);
}

// Generic behaviour of a smart (measured) watering event.
void _smartWtrEvent(void* pvParameters)
{
    int chan = ((wtrEventParams*)pvParameters)->chan;
    double goal = pstate.getChSettGoal(chan); // target amount in litres
    double freq = pstate.getChSettFreq(chan); // how often to dispense

    while (!stopFlag[chan-1])
    {
        double start_wtr = wmc.getWaterDelivered();
        int64_t start_time = waterTime();
        wmc.setMotor(chan, true); 
        while (!stopFlag[chan-1] && (start_wtr+goal) > wmc.getWaterDelivered())
        {
            _tick();
        }
        
        wmc.setMotor(chan, false);
        while (!stopFlag[chan-1] && (start_time+freq) > waterTime())
        {
            _tick(); 
        }
        
        _tick();
    }

    wmc.setMotor(chan, false); 
    vPortFree(pvParameters);
    vTaskDelete(NULL);
}

// Starts a new watering event on a particular channel based on its current settings.
void newWtrEvent(int chan)
{
    // create an event for watering
    wtrEventParams* pvParams = (wtrEventParams*)pvPortMalloc(sizeof(wtrEventParams));
    pvParams->chan = chan;
    
    char e_name[10] = "test";
    e_name[4] = chan + '0';
    e_name[5] = '\0';
    
    TaskFunction_t event;
    
    switch (pstate.getChSettMode(chan))
    {
        case PState::CHMode::Manual:
            event = _manualWtrEvent;
            break;
        case PState::CHMode::Timed:
            event = _timedWtrEvent;
            break;
        case PState::CHMode::Smart:
            event = _smartWtrEvent;
            break;
        case PState::CHMode::Genius:
        default:
            // no goal
            vPortFree(pvParams);
            return;
    }
    stopFlag[chan-1] = false;
    xTaskCreate(
        event,  
        e_name,  
        WTREVENT_STACKSZ,  // Stack size  
        (void*)pvParams,  
        WTREVENT_PRIORITY,  // Priority
        NULL );
}

// stop all water events
void stopWtrEvents()
{
    stopFlag[0] = true;
    stopFlag[1] = true;
    stopFlag[2] = true;
    stopFlag[3] = true;

    _tick();
    _tick();// wait for everything to stop
}
