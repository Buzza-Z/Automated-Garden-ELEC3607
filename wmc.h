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
#ifndef WMC_H
#define WMC_H
#include <Arduino.h>

#define WMC_MCTL1 (23)
#define WMC_MCTL2 (22)
#define WMC_MCTL3 (21)
#define WMC_MCTL4 (19)
#define WMC_MCTL_EN (18)
#define WMC_MCTL_FAULT (5)

#define WMC_I2C_SDA (17)
#define WMC_I2C_SCL (16)
#define WMC_I2C_CLK (100000)

#define WMC_WTR_FLOW (27)

#define WMC_MOTOR_FAULT (-1)
#define WMC_MOTOR_ON (1)
#define WMC_MOTOR_OFF (0)

#ifndef RTC_APB_FREQ_REG
#define RTC_APB_FREQ_REG 0x3FF480B4
#endif

class Adafruit_seesaw;

class MotorBoard
{
  public:
    MotorBoard();

    // Initialises connections to sensors, timers, interrupts etc
    void setupPeripherals();

    // Enables or disables a particular motor channel.
    int setMotor(int motor, bool enable);

    // Gets the current setting of a particular motor channel.
    int getMotorState(int motor);

    // Checks the fault output of the motor driver IC.
    bool isFaultState();

    // Checks if the soil sensor was successfully connected to
    bool isSSconnected();

    // Gets the latest soil temperature/ moisture. 
    // May return a cached value or retrieve a new value.
	float getSSTemp();
	uint16_t getSSMoisture();

    // Gets the total amount of water delivered so far.
    double getWaterDelivered();
	
	~MotorBoard();
    
  private:
    void _updateSensors();
    bool _motorStates[4];
	Adafruit_seesaw* _ss;
	
	bool _ssState = false;  
    float _lastSSTemp;
    uint16_t _lastSSMoisture;    
};

#endif
