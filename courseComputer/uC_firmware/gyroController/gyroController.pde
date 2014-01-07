
/*
  Autopilot firmware
  ==================

  for Splashelec course computer/servo controller and associated hardware:
     - course computer hardware version V1.0
     - Autonnic A5020 NMEA compass connected using software serial
     - gyro sensor connected as Arduino shield
     - analog proportional joystick 
     - Raymarine rudder reference sensor
     - Raymarine SPX-5 electric ram
     - waterproof, application specific keyboard connected to
       hardware UART
     - mode selection lever switch (standby - joystick)
     - ACS714-30 current sensor connected to the motor output

  The software works also with neither compass, nor keyboard
  connected. In this case, the switch mounted to the course computer selects 
  standby or joystick mode.

  ******************************************************************************
  Todo :
     - replace the PID library by an up to date version
     - This code needs heavy cleanup, the code must be restructured.
     - The algorithm combining gyro and compass to calculate fast heading data
       is a complete humbug. It was written under pressure, the night before 
       the first sea-trial. I'm still surprised that it works good enough 
       for auto steering. Somebody should be able to replace this part by a real 
       filter (Kalman?). Or, for a simpler solution, use a compas (or complete
       AHRS integrated gyro sensor that does these calculations externally. 
  ******************************************************************************


  V1.3 2013-04-06

  by Bernt Weber (bernt.weber@splashelec.com)
  

This code published using the following (MIT) license:

Copyright (c) 2010, 2011  Bernt WEBER (Splashelec)

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

// Compass + gyro
#include <NewSoftSerial.h>

// compass NMEA 
#define rxPin 12 

// unused switch 3, dummy for ReverseSoftwareSerial
#define txPin 13

// Tri-state for Xilinx CPLD
#define GTS1 2

// analog in 3
#define gyroPin 3

// set up a new serial port for NMEA compass
NewSoftSerial mySerial(rxPin, txPin, true);

int    nextHeadingTimes10;
double heading, oldHeading;
int    decimalPointReceived=1; // decimal point received in current NMEA heading phrase
int    headingReceived=1;      // true after first decimal read in current NMEA heading phrase

long int gyro;
double gyroAverage;
double integration = 0.0;
double gyroMeasuredTurnRate, compassMeasuredTurnRate;
double offset = 0; // between integrated gyro and compass

double offsetCorrectionDivisor = -10; // starts with  low value and becomes bigger, the correction becomes slower
                                      // after fast startup
 
// PID
#include <PID_Beta6.h>

double Setpoint, Input;
double SpeedSetting, Speed, Output;
double OutputBias = 49.0; // motor stands still @ 50%

//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &Setpoint, &OutputBias, 2, 0, 0);
//PID myPID(&Input, &SpeedSetting, &Setpoint, &OutputBias, -3, 0,0);

byte   keyboardAvailableCountdown = 0; // If zero keyboard was not heard for a while.
char   Mode = 'S';
double Heading, HeadingToSteer;
double Straight = 180.0;  // dummy for the PID library.
double ErrorPlusStraight; // 
double RudderAngle;
double RudderBias = 0.5;

//===============================================================================
//===============================================================================
// PID compasPID(&Heading, &RudderAngle, &HeadingToSteer, &RudderBias, 5, 1, 200);
// économique pour durer
///PID compasPID(&Heading, &RudderAngle, &HeadingToSteer, &RudderBias, 5, 0, 0.5);
// reste une légère oscillation sur 1 minute avec ce trim PID compasPID(&Heading, &RudderAngle, &HeadingToSteer, &RudderBias, 5, 35, 0.5);

// We use now precalculated course error, so that we do not get an problem when 
// compass goes over 0 degrees. This allows us to take into account the cyclic nature
// of the heading.
// (Probably this makes that the changes generated at headingToSteer change are not 
// handled correctly. One could switch briefly to PID-Manual mode.)
PID compasPID(&ErrorPlusStraight, &RudderAngle, &Straight, &RudderBias, 5, 0, 0.5);
//===============================================================================
//===============================================================================

int switch1Pin = 11; // mode selector switch
int joystickPin = 7; // analog 7
int rudderPin   = 6; // analog 6

int   currentSense = 2;       // analog 2
float motorResistance = 1.1;  // in Ohms
float dissipatedEnergy = 0.0; // accumulation disspated energy
int   energyTimer = 0;        // timer for energy/power calculations
float powerLimit = 16.0;      // [W] the average output power will be limited to this amount
int   powerLimitWait = 0;     // Time to wait before using the motor again to limit power dissipation

int joystickMin = 117;
int joystickMax = 883;

int joystickSpread = joystickMax - joystickMin;
double joystickMiddle; // the value the joystick gives when in its stable middle position
double joystickAutoTrim; // correction calculated from past joystick position

int rudderMin = 317;
int rudderMax = 655;
int rudderSpread = rudderMax - rudderMin;

boolean limitPower()
// true if powerLimit has been exeeded
{
  if ((energyTimer >= 10) && (powerLimitWait == 0)){
    energyTimer = 0;
    dissipatedEnergy = 0.0;
  }
  energyTimer++;
  float current = abs((2.5 - (analogRead(currentSense)/1024.0 * 5.0)) / 0.066); // ACS714-30: 0.066 V/A  
  dissipatedEnergy += current * current * motorResistance * 0.1; // power * time
      
  if ((powerLimitWait == 0) && (dissipatedEnergy > powerLimit))
  {
    powerLimitWait = (10 - energyTimer) + ((dissipatedEnergy - powerLimit)/powerLimit) * 10 + 2; // wait 0.2 s longer than necessary   
    Serial.print(powerLimitWait); Serial.print ("* 0.1 s waiting after "); Serial.print(energyTimer); Serial.println(" * 0.1s of counting.");
  }
  
  if (energyTimer == 10)
  {
    Serial.print("***** energy : "); Serial.println(dissipatedEnergy);
  }
  
  if (powerLimitWait > 0) {
    powerLimitWait --;
    return true;
  } else
    return false;
    
}// limit power

double getRelativeJoystick()
{
    //Serial.print("raw Joystick: "); Serial.println(analogRead(joystickPin));
    double relativeJoystick = ((analogRead(joystickPin)-joystickMin) * 1.0) / (joystickSpread * 1.0); 
    if (relativeJoystick < 0.0) relativeJoystick = 0.0;
    else if (relativeJoystick > 1.0) relativeJoystick = 1.0;

    // Serial.print("relative Joystick: "); Serial.println(relativeJoystick * 100, 0);
    return relativeJoystick;
}


void setup()  {
  
  // compas + gyro
  // define pin modes for tx, rx, led pins:
  pinMode(rxPin, INPUT);
  // pinMode(txPin, OUTPUT);
  // set the data rate for the ReverseSoftwareSerial port
  mySerial.begin(4800);  
  Serial.begin(38400); 
  Serial.println("Hello here is the course computer!");

  gyroAverage = analogRead(gyroPin);
  
  // PID
    pinMode(3, OUTPUT);
  // pinMode(11, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  
  //Timer/Counter2
  TCCR2A = _BV(COM2A0) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS21); // toggle OC2A on OCR2A match; prescaler: /8
  OCR2A = 99;
  OCR2B = 39;
  
//    * Output A frequency: 16 MHz / 8 / (99+1) / 2 = 10 kHz
//    * Output A duty cycle: 50%
//    * Output B frequency: 16 MHz / 8 / (99+1) = 20 kHz
//    * Output B duty cycle: (39+1) / (99+1) = 40 % 
//    Output B = DigitalOut 3;
//    Output A = DigitalOut 11;

// Timer/counter1
  TCCR1A = _BV(WGM11) | // FastPwm, TOP = ICR1 
           _BV(COM1A1)|  // clear OCR1 on match
           _BV(COM1B1);  // clear OCR2 on match
            
  TCCR1B = _BV(WGM12) |_BV(WGM13) |_BV(CS11);    // prescaler: /8
  
  ICR1  = 99;
  //OCR1A = 19;
  OCR1A = 0; // relay_pwm 
  
  OCR1B = 29;
  
  // Frequency: 16 MHz/ 8 / (99+1) = 20 kHz
  // Output A duty: (19+1) / (99+1) = 20 %
  // Output B duty: (29+1) / (99+1) = 30 %
  // Output A = DigitalOut 9
  // Output B = DigitalOut 10
  
  // wait for precharging the filter capacitor at the H-bridge's entry
  delay(2000);
  // activate the tristate output buffers of the CPLD 
  // in order to control the MOSFET drivers
  digitalWrite(0, GTS1);
  pinMode(GTS1, OUTPUT); 
 
  double relativeJoystick = getRelativeJoystick();
   
  // set mid point of joystick, at power or auto on
  joystickMiddle = relativeJoystick; 
  joystickAutoTrim = 0.0;

  //Serial.begin(9600); 
  
  Input = ((analogRead(6) - 512) >> 2);
  Setpoint = (analogRead(7) - 512) >> 2;
  myPID.SetSampleTime(95); // 100 ms; 10 x per second (slightly shorter so that it will
                           // allways be executed. Timing comes anyway from the fact
                           // that we wait for the compass wich sends data every 100 ms.
                           
  myPID.SetInputLimits(0.0, 1.0);
  myPID.SetOutputLimits(0.0, 99.0);
  //turn the PID on
  myPID.SetMode(AUTO);
  
  compasPID.SetMode(MANUAL);
  compasPID.SetSampleTime(95); // see explanation from line myPID.SetSampleTime(95);
  compasPID.SetOutputLimits(0.0, 1.0);

}

void loop() {
  int i;
  double relativeJoystick;
  double relativeRudder;
  char someChar = 'X';
  
  // listen for compass serial coming in:
  // wait slightly longer than the compass send interval
  // or fall through without having read a char
  someChar = 'X'; // dummy, marker for nothing received
  for (i = 1; i <= 102; i++){ 
    if (mySerial.available()){
      someChar = mySerial.read();
      // Serial.print(someChar);
      break;
    }
    delay(1);
  }  

 
  if ((someChar == '\n') || (someChar == 'X')) { // 100 ms are over: with a compas reading or without)
    if (someChar == '\n') {
      nextHeadingTimes10 = 0;
      decimalPointReceived = 0;
      headingReceived = 0;
      //Serial.print(heading); Serial.println(" ");
      gyro = 0;
      for (i = 0; i < 10; i++)
        gyro += analogRead(gyroPin);
      gyro /= i;
      //Serial.print(gyro); Serial.print(" ");
      gyroAverage += (gyro - gyroAverage) / 1000;
      //Serial.print(gyroAverage);
      gyroMeasuredTurnRate = -1.15 * (gyro - gyroAverage) * 4.88 / 15.0; // in deg/s (1024 ticks are 5000 mV, 15 mV are 1deg/s
        // factor 1.15 experimentaly determined (Heading mooved to slowly during turns
        
      //Serial.print("\t gmtr: \t"); Serial.print(gyroMeasuredTurnRate, 0);
      compassMeasuredTurnRate = (heading - oldHeading) * 10; // in deg/s
      // correct if north is between
      if (compassMeasuredTurnRate <= -1800) compassMeasuredTurnRate += 3600;
      if (compassMeasuredTurnRate >   1800) compassMeasuredTurnRate -= 3600;
      
      //Serial.print("\t cmtr: \t"); Serial.print(compassMeasuredTurnRate, 0); Serial.print(" \t");
      
      // correct gyro average by compas turn
      gyroAverage -= -1 * (compassMeasuredTurnRate)/335.0; 
        // factor experimentally determined by observing changes ouv gyroAverage (tuned to stabilize it)
      
      integration += gyroMeasuredTurnRate * 0.1; // 0.1s interval
      // correct if north is between
      if (integration <    0) integration += 360;
      if (integration >= 360) integration -= 360;
      
      offset = integration - heading;
      // correct if north is between
      if (offset <= -180) offset += 360;
      if (offset >   180) offset -= 360;
      
      // correct gyro integration
      if (offsetCorrectionDivisor < 2) integration -= offset / 2;
      else integration -= offset / (offsetCorrectionDivisor);
      if (offsetCorrectionDivisor < 50.0) offsetCorrectionDivisor += 0.1;
      // correct if north is between
      if (integration <     0) integration += 360;
      if (integration >=  360) integration -= 360;
            
      //Serial.print(integration);Serial.print(" diff: "); Serial.print(offset);Serial.print(" ocd: "); Serial.println(offsetCorrectionDivisor);
      Heading = integration; // would be better to rename one of those ? ...
      
      // Serial.print(heading); Serial.print(" "); Serial.println(Heading);
      
    }; // someChar == '\n'
  
    // PID ======================================
    relativeJoystick = getRelativeJoystick(); 
    // Serial.print(" Joystick: "); Serial.print(relativeJoystick * 100, 0);
  
    // accumulate joystickAutotrim when not in a maneuver 
    // note: a soft transition between the two behaviors could be better then this
    //       all or nothing "if"
    if ((relativeJoystick > 0.1) && (relativeJoystick < 0.9))
    // accumulate joystickAutoTrim
       joystickAutoTrim += (relativeJoystick - joystickMiddle) / 300.0;
    else {
    // slow manoeuver => reset joystickAutotrim slowly
       // joystickAutoTrim *= 0.9995;
    // do not accumulate, but no reset;
       ;  
    } 
       
    // Serial.print(" Autotrim: "); Serial.print(joystickAutoTrim * 100, 0);
    
    if (joystickAutoTrim > 0.25) joystickAutoTrim = 0.25;
    else if (joystickAutoTrim < -0.25) joystickAutoTrim = -0.25;
    
    // relativeJoystick += joystickAutoTrim;
    // weighted add of autotrim: full trim in the middle, going to zero when close to the ram limits
    if (relativeJoystick >= 0.5) relativeJoystick += (1 - relativeJoystick) * 2.0 * joystickAutoTrim;
    else
      relativeJoystick += relativeJoystick * 2.0 * joystickAutoTrim;

    if (relativeJoystick < 0.0) relativeJoystick = 0.0;
    else if (relativeJoystick > 1.0) relativeJoystick = 1.0;
       
    if (relativeJoystick < 0.0) relativeJoystick = 0.0;
    else if (relativeJoystick > 1.0) relativeJoystick = 1.0;
  
    //Serial.print("raw Rudder: "); Serial.println(analogRead(rudderPin));
    relativeRudder = ((analogRead(rudderPin)-rudderMin) * 1.0) / (rudderSpread * 1.0); 
  
    // reverse rudder
    relativeRudder = (relativeRudder * -1.0) + 1.0;
   
    if (relativeRudder < 0.0) relativeRudder = 0.0;
    else if (relativeRudder > 1.0) relativeRudder = 1.0;
    
    // Serial.print("Joystick: "); Serial.println(relativeJoystick * 100, 0);
      
      
    // read commands from keyboard
    while (Serial.available() > 0) {
      // get incoming byte:
      char inByte = Serial.read();
      if (inByte == '7') HeadingToSteer += -1;
      if (inByte == '9') HeadingToSteer += +1;
      if (inByte == '4') HeadingToSteer += -10;
      if (inByte == '6') HeadingToSteer += +10;
      if (inByte == '1') HeadingToSteer += -90;
      if (inByte == '3') HeadingToSteer += +90;
      if (inByte == 'S') Mode = 'S';
      if (inByte == 'A') Mode = 'A';
      if (inByte == 'J') Mode = 'J';
      if (inByte == 'K') keyboardAvailableCountdown = 50;
    }
    if (keyboardAvailableCountdown > 0) keyboardAvailableCountdown -= 1;
    
    //===============================================================================
    //Serial.print(" Compass: "); Serial.print(Heading); Serial.print(" Bearing: "); Serial.println(HeadingToSteer);
    //===============================================================================
      
      
    if (keyboardAvailableCountdown == 0){
      if (digitalRead(switch1Pin)) Mode = 'J';
      else                         Mode = 'S';
    };
    
    if(Mode == 'J'){ // "Handy" proportional joystick mode
       // Serial.println(" Handy mode"); 
       compasPID.SetMode(MANUAL);
       
       Setpoint = relativeJoystick;
       
       // set heading to steer for auto mode
       HeadingToSteer = Heading;

       OCR1A = 98; // relay_pwm 
    }
    else if (Mode == 'A') {
       // autopilot mode
       compasPID.SetMode(AUTO); // I'am not shure if this destroys the PID 'D' part calculation.
       // Serial.println(" Autopilot mode");
       
       // precalculate ErrorPlusStraight, that we feed to the PID
       ErrorPlusStraight = Heading - HeadingToSteer + Straight;
       // in case that the north is between Heading and Heading to steer
       if (ErrorPlusStraight <    0.0) ErrorPlusStraight += 360.0;
       if (ErrorPlusStraight >= 360.0) ErrorPlusStraight -= 360.0;
       compasPID.Compute();
       Setpoint = RudderAngle;
       
       // reset auto trim for joystick mode
       joystickAutoTrim = 0.0;

       OCR1A = 98; // relay_pwm 
    }
    else { // Mode is supposed to be 'S'tandby

       OCR1A = 0; // relay_pwm 
    
       // reset auto trim for joystick mode
       joystickAutoTrim = 0.0;
       
        // set heading to steer for auto mode
       HeadingToSteer = Heading;
   
    }
  
    if (Setpoint < 0.0) Setpoint = 0.0;
    else if (Setpoint > 1.0) Setpoint = 1.0;
    // Serial.println(Setpoint);
    Input = relativeRudder;
    myPID.Compute();
    int pwm = (int) Output;
    if ((pwm >= 46) and (pwm <= 52)) // deadband
       pwm = 49;
    if (pwm < 0) pwm = 0;
    if (pwm > 98) pwm = 98;
    
    //Serial.println(pwm);
    if (! limitPower())
       OCR1B = pwm;
    else
       OCR1B = 49; // motor current has to go zero
    
      
    
    // END PID ======================================
    
  };
  if (!headingReceived) {
    if (someChar == '.'){
      decimalPointReceived = 1;     
    }
    else {
      if (someChar == '0')
        nextHeadingTimes10 = nextHeadingTimes10 * 10;       
      if (someChar == '1')
        nextHeadingTimes10 = 1 + nextHeadingTimes10 * 10;
      if (someChar == '2')
        nextHeadingTimes10 = 2 + nextHeadingTimes10 * 10;
      if (someChar == '3')
        nextHeadingTimes10 = 3 + nextHeadingTimes10 * 10;
      if (someChar == '4')
        nextHeadingTimes10 = 4 + nextHeadingTimes10 * 10;
      if (someChar == '5')
        nextHeadingTimes10 = 5 + nextHeadingTimes10 * 10;
      if (someChar == '6')
        nextHeadingTimes10 = 6 + nextHeadingTimes10 * 10;
      if (someChar == '7')
        nextHeadingTimes10 = 7 + nextHeadingTimes10 * 10;
      if (someChar == '8')
        nextHeadingTimes10 = 8 + nextHeadingTimes10 * 10;
      if (someChar == '9')
        nextHeadingTimes10 = 9 + nextHeadingTimes10 * 10;
    
      if (decimalPointReceived) {   // the decimal point was received before
                                  // so someChar is now 
                                  // the first decimal. We will not get more numbers
                                  // in this line.
        headingReceived = 1; // do not search for more heading information in this line!                   
        if ((nextHeadingTimes10 >= 0) && (nextHeadingTimes10 <= 3600)) {
            oldHeading = heading;
            heading = nextHeadingTimes10 / 10.0;
        } // if
      } // if
    } // else
  }; // if (!headingReceived)

}

