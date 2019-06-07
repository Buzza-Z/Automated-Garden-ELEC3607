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
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_seesaw.h"

#include "wmc.h"

static volatile uint64_t MB_wtr_delivered = 0;
// IRAM_ATTR forces code into internal memory
static void IRAM_ATTR MB_incWtr()
{
    MB_wtr_delivered++;
}

static volatile bool MB_sensor_outdated = true;
static void IRAM_ATTR MB_void_sensor_data()
{
    MB_sensor_outdated = true;
}


MotorBoard::MotorBoard()
{
    _ss = new Adafruit_seesaw();
    
    pinMode(WMC_MCTL_FAULT, INPUT_PULLUP); // open drain on driver
    pinMode(WMC_MCTL_EN, OUTPUT); // active low
    digitalWrite(WMC_MCTL_EN, LOW);

    // motor channels
    pinMode(WMC_MCTL1, OUTPUT);
    pinMode(WMC_MCTL2, OUTPUT);
    pinMode(WMC_MCTL3, OUTPUT);
    pinMode(WMC_MCTL4, OUTPUT);
    _motorStates[0] = false;
    _motorStates[1] = false;
    _motorStates[2] = false;
    _motorStates[3] = false;
    
    // water flow sensor
    pinMode(WMC_WTR_FLOW, INPUT_PULLUP); 
    attachInterrupt(digitalPinToInterrupt(WMC_WTR_FLOW), MB_incWtr, RISING);
}

void MotorBoard::setupPeripherals()
{
    // sensor timer
    double apb_freq = ((READ_PERI_REG(RTC_APB_FREQ_REG)) & UINT16_MAX) << 12; // timer clk
    
    hw_timer_t* timer = timerBegin(1, apb_freq/1000000, true); // ~"TC_Configure", prescale for 1us ticks
    timerAttachInterrupt(timer, MB_void_sensor_data, true); // ~"NVIC_EnableIRQ" + "TCX_Handler"
    timerAlarmWrite(timer, 200000, true); // ~"TC_SetRC", 200000 * 1us ticks = 200ms
    timerAlarmEnable(timer);
    
    // STEMMA header
    Wire.begin(WMC_I2C_SDA, WMC_I2C_SCL, WMC_I2C_CLK);
    
    _ssState = _ss->begin(0x36);

}

void MotorBoard::_updateSensors()
{
    if (MB_sensor_outdated)
    {
        _lastSSTemp = _ss->getTemp();
        _lastSSMoisture = _ss->touchRead(0);
        MB_sensor_outdated = false;
    }
}

bool MotorBoard::isSSconnected()
{
    return _ssState;
}

uint16_t MotorBoard::getSSMoisture()
{
	if (!_ssState)
		return 0;

    _updateSensors();
      
	return _lastSSMoisture;	
}

float MotorBoard::getSSTemp()
{
	if (!_ssState)
		return 0;

    _updateSensors();
       	
	return _lastSSTemp; // in celcius
}

int MotorBoard::setMotor(int motor, bool enable)
{    
    if (motor < 1 || motor > 4)
        return -1;
        
    if (isFaultState())
        return WMC_MOTOR_FAULT;
    switch (motor)
    {
        case 1:
            digitalWrite(WMC_MCTL1, enable);
            break;
        case 2:
            digitalWrite(WMC_MCTL2, enable);
            break;
        case 3:
            digitalWrite(WMC_MCTL3, enable);
            break;
        case 4:
            digitalWrite(WMC_MCTL4, enable);
            break;
        default:
            return 0;
    }
    _motorStates[motor-1] = enable;
    
    return enable;
}

int MotorBoard::getMotorState(int motor)
{
    if (motor < 1 || motor > 4)
        return -1;
        
    if (isFaultState())
        return WMC_MOTOR_FAULT;
    
    return _motorStates[motor-1];
}

bool MotorBoard::isFaultState()
{
    return digitalRead(WMC_MCTL_FAULT) == LOW;
}

double MotorBoard::getWaterDelivered()
{
    double wtr = MB_wtr_delivered / 450.0;
    return wtr;
}


MotorBoard::~MotorBoard()
{
	delete _ss;
}
