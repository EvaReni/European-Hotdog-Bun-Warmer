#include <math.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "songDefs.h"

//Use LGT8F328

bool played = false;

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Adjust address if needed


// === Constants ===
const float seriesResistor = 100000.0;
const float nominalResistance = 100000.0;
const float nominalTemp = 25.0;
const float beta = 3950.0;
const float adcMax = 1023.0;


bool useBangBang = false;  // Set to true to use bang-bang control instead of PID
const float hysteresis = 2.0;  // Hysteresis band in degrees Celsius  - “ὑστέρησις” (hysterēsis)



// === Setpoints (°C) ===
float setpoint1 = 80.0;
float setpoint2 = 80.0;

float prevSetpoint1 = setpoint1;
float prevSetpoint2 = setpoint2;

const float maxSetpoint = 120.0;
const float minSetpoint = 30.0;

const float minStartupTemp = 40.0;  // Minimum temperature to start the PID control

const float integralLimit = 100.0;  // Was 1000.0
float Kp1 = 50.0, Ki1 = 1.5, Kd1 = 2.0;  // Was 0.1
float Kp2 = 50.0, Ki2 = 1.5, Kd2 = 2.0;

const float controlInterval = 1;  // in seconds

// === Pins ===
const int sensorPin1 = A0;
const int sensorPin2 = A1;

const int heaterPin1 = 9;   //D9
const int heaterPin2 = 10;   //D10

const int buttonPlusPin = 3;
const int buttonMinusPin = 4;

const int backlightPin = 6;    // D6 - PWM pin
const int brightness = 255;    // 0–255


// === Variables for Heater 1 ===
float input1 = 0, error1 = 0, lastError1 = 0, integral1 = 0, derivative1 = 0, output1 = 0;

// === Variables for Heater 2 ===
float input2 = 0, error2 = 0, lastError2 = 0, integral2 = 0, derivative2 = 0, output2 = 0;

// === Timing ===
unsigned long lastTime = 0;
float dt = 0.0;

unsigned long lastLCDUpdate = 0;
float updateLCDsec = 3.0;  // You can change this to 1.0, 2.0, 3.0 etc.

#define NUM_SAMPLES 20

float tempBuffer1[NUM_SAMPLES] = {0};
float tempBuffer2[NUM_SAMPLES] = {0};
int sampleIndex = 0;

float ema1 = 0;
float ema2 = 0;
const float alpha = 0.2;  // Smoothing factor (adjust 0.1 to 0.3 to tune responsiveness)

const int disableHeaterDuration = 20;   //Disable the heater after 120 seconds

int heater1InactivityDuration = 0;
int heater2InactivityDuration = 0;

float prevT1 = 0; 
float prevT2 = 0;

bool haltH1 = false;
bool haltH2 = false;
bool notify = true;

const unsigned long safetyCheckWindow = 60;  // 10 seconds
unsigned long lastSafetyCheck = 0;

const float minRiseThreshold = 0.3;

const unsigned long contUpdateDuration = 100;
const unsigned long contStartDuration = 700;

float temp1WindowStart = 0;
float temp2WindowStart = 0;

unsigned long temp1WindowStartTime = 0;
unsigned long temp2WindowStartTime = 0;

unsigned long lastLCDRefresh = 0;
const unsigned long lcdRefreshTime = 50;

bool debug = false;

bool playing = false;
bool playedDone = false;

float getAverage(float* buffer) {
  float sum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sum += buffer[i];
  }
  return sum / NUM_SAMPLES;
}

void setup() {
  Serial.begin(115200);
  pinMode(sensorPin1, INPUT);
  pinMode(sensorPin2, INPUT);
  pinMode(heaterPin1, OUTPUT);
  pinMode(heaterPin2, OUTPUT);

  pinMode(buttonPlusPin, INPUT_PULLUP);
  pinMode(buttonMinusPin, INPUT_PULLUP);

  initPlaySong();
  played = false;

  setSong(DONE);

  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Hotdog Warmer :)");
  delay(1500);
  lcd.clear();

  // Initialize EMA with first readings to avoid startup jump
  ema1 = readTemperature(sensorPin1);
  ema2 = readTemperature(sensorPin2);

  temp1WindowStart = ema1;
  temp2WindowStart = ema2;
  temp1WindowStartTime = millis();
  temp2WindowStartTime = millis();

    // Set Phase Correct PWM mode on Timer1
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM10);
  TCCR1B = _BV(CS12) | _BV(CS10); // Prescaler = 1024

  // _BV(bit) is a macro that sets the bit to 1. 
  // Implemetation: 
  //    #define _BV(bit) (1 << (bit))

  //Timer clock after prescaler: 15,625 Hz
  //PWM frequency (Phase Correct 8-bit): ~30.5 Hz

  /*
      CS bits & Prescaler values (Timer1)
      CS12	CS11	CS10	Prescaler	Timer Clock Frequency
        0   	0	    0   	No clock (timer stopped)	
        0   	0	    1   	1 (no prescaler)	
        0	    1	    0   	8	
        0	    1	    1   	64	
        1	    0     0   	256	
        1	    0	    1   	1024	
  */
}

void loop() 
{
    unsigned long now = millis(); 
    dt = (now - lastTime) / 1000.0;

    handleButtons(now);

    if(playing)  playing = !playSong();

    if(haltH1 || haltH2)
    {
        if(!notify)
          return;

        if(haltH1)
        {
            lcd.setCursor(10, 0);  // Set cursor to the second position on the second line
            lcd.print("H1:--- ");
        } 

        if(haltH2)
        {
            lcd.setCursor(10, 1);  // Set cursor to the second position on the second line
            lcd.print("H2:--- ");
        } 


        if(haltH1 && haltH2)
        {
          lcd.setCursor(0, 1);  // Set cursor to the second line, first column
          lcd.print("-System halted!-");
          notify=!notify;
          setSong(FAIL);
          playing = true;
          return;
        }
    }


    updateDisplay(now);
  
  if (dt >= controlInterval) 
  {


    // Read raw temps from sensors
    float rawTemp1 = readAverageTemperature(sensorPin1);
    float rawTemp2 = readAverageTemperature(sensorPin2);

    // Update EMA temps
    ema1 = alpha * rawTemp1 + (1 - alpha) * ema1;
    ema2 = alpha * rawTemp2 + (1 - alpha) * ema2;

    // Use EMA temps as input for PID
    input1 = ema1;
    input2 = ema2;

    if( input1>= setpoint1 && input2>=setpoint2 && !playing && !playedDone)
    {
      setSong(DONE);
      playing = true;
      playedDone = true;
    }



    // === PID Loop 1 ===
    error1 = setpoint1 - input1;

    if (input1 < minStartupTemp) {
      integral1 = 0;  // Reset integral for Heater 1 if below startup temp
    } else 
        {
          // Only accumulate integral if not saturated or if error would reduce output
          if (!(output1 == 255 && error1 > 0)) 
          {
            integral1 += error1 * dt;
            integral1 = constrain(integral1, -integralLimit, integralLimit);  // Limit integral
          }
        }

    // Anti-windup for integral
    integral1 = constrain(integral1, -integralLimit, integralLimit);

    derivative1 = (error1 - lastError1) / dt;
    output1 = Kp1 * error1 + Ki1 * integral1 + Kd1 * derivative1;

    // === Deadband ===
    if (abs(error1) < 0.5) {
      output1 = 0;
    }

    // === Constrain final output ===
    output1 = constrain(output1, 0, 255);

    if(!haltH1)
    {
        if (useBangBang) {
            static bool heater1On = false;
            
            heater1On = (input1 < (setpoint1 - hysteresis)) ? true :
                        (input1 > (setpoint1 + hysteresis)) ? false : heater1On;


            analogWrite(heaterPin1, heater1On ? 255 : 0);
            output1 = heater1On ? 255 : 0;  // for LCD/debug
        } else {
            analogWrite(heaterPin1, output1);  // PID control
        }
    }

    lastError1 = error1;



    // === PID Loop 2 ===
    error2 = setpoint2 - input2;
    
    if (input2 < minStartupTemp) {
      integral2 = 0;  // Reset integral for Heater 2 if below startup temp
    } else 
        {
          // Only accumulate integral if not saturated or if error would reduce output
          if (!(output2 == 255 && error2 > 0)) 
          {
            integral2 += error2 * dt;
            integral2 = constrain(integral2, -integralLimit, integralLimit);  // Limit integral
          }
        }
    // Anti-windup for integral
    integral2 = constrain(integral2, -integralLimit, integralLimit);

    derivative2 = (error2 - lastError2) / dt;
    output2 = Kp2 * error2 + Ki2 * integral2 + Kd2 * derivative2;

    // === Deadband ===
    if (abs(error2) < 0.5) {
      output2 = 0;
    }

    // === Constrain final output ===
    output2 = constrain(output2, 0, 255);

    if(!haltH2)
    {
        if (useBangBang) 
        {
          static bool heater2On = false;

          heater2On = (input2 < (setpoint2 - hysteresis)) ? true : 
                (input2 > (setpoint2 + hysteresis)) ? false : heater2On;

          analogWrite(heaterPin2, heater2On ? 255 : 0);
          output2 = heater2On ? 255 : 0;
        } else {
            analogWrite(heaterPin2, output2);  // PID control
        }
    }

    lastError2 = error2;

    //Safety checks
    //performSafetyCheck(now);
 
    if(debug)
    {
        // === Debug Output ===
        Serial.print("T1: "); Serial.print(input1); Serial.print(" °C | ");
        Serial.print("PWM1: "); Serial.print(output1); Serial.print(" | ");
        Serial.print("T2: "); Serial.print(input2); Serial.print(" °C | ");
        Serial.print("PWM2: "); Serial.println(output2);
    }

    if ((now - lastLCDUpdate) >= (updateLCDsec * 1000)) 
    {

       updateLCD();

        lastLCDUpdate = now;
        lastTime = now;       
    }
  }

}

float readAverageTemperature(int analogPin) {
  const int numSamples = 20;
  float total = 0;

  for (int i = 0; i < numSamples; i++) {
    total += readTemperature(analogPin);
    //delay(5);  // small delay to let ADC stabilize
  }

  return total / numSamples;
}


float readTemperature(int analogPin) {
  int adc = analogRead(analogPin);
  
  if (adc <= 0 || adc >= 1023) {
    return -1000.0;  // Invalid read
  }

  float resistance = seriesResistor / ((adcMax / (float)adc) - 1.0);
  float steinhart = resistance / nominalResistance;
  steinhart = log(steinhart);
  steinhart /= beta;
  steinhart += 1.0 / (nominalTemp + 273.15);
  steinhart = 1.0 / steinhart;
  return steinhart - 273.15;
}


void performSafetyCheck(unsigned long now) {
      // === Heater 1 Safety Check ===
      if(!haltH1)
      {
          if (output1 >= 200) {
            if ((now - temp1WindowStartTime) >= (safetyCheckWindow * 1000UL)) {
              float delta = input1 - temp1WindowStart;

              if (delta < minRiseThreshold) {
                heater1InactivityDuration += safetyCheckWindow;
                Serial.print("H1 inactivity += "); Serial.println(safetyCheckWindow);
              } else {
                heater1InactivityDuration = 0;
                Serial.println("H1 temp rising, inactivity reset.");
              }

              temp1WindowStart = input1;
              temp1WindowStartTime = now;
            }
          } else {
            temp1WindowStart = input1;
            temp1WindowStartTime = now;
            heater1InactivityDuration = 0;
          }

          if (heater1InactivityDuration >= disableHeaterDuration) {
            analogWrite(heaterPin1,0);
            haltH1 = true;
            Serial.println("!!! H1 halted due to sustained inactivity");
          }
      }


      // === Heater 2 Safety Check ===
      if(!haltH2)
      {
          if (output2 >= 200) {
            if ((now - temp2WindowStartTime) >= (safetyCheckWindow * 1000UL)) {
              float delta = input2 - temp2WindowStart;

              if (delta < minRiseThreshold) {
                heater2InactivityDuration += safetyCheckWindow;
                Serial.print("H2 inactivity += "); Serial.println(safetyCheckWindow);
              } else {
                heater2InactivityDuration = 0;
                Serial.println("H2 temp rising, inactivity reset.");
              }

              temp2WindowStart = input2;
              temp2WindowStartTime = now;
            }
          } else {
            temp2WindowStart = input2;
            temp2WindowStartTime = now;
            heater2InactivityDuration = 0;
          }

          if (heater2InactivityDuration >= disableHeaterDuration) {
            haltH2 = true;
            analogWrite(heaterPin2,0);
            Serial.println("!!! H2 halted due to sustained inactivity");
          }
      }
}

void updateDisplay(unsigned long now) {
if (now - lastLCDRefresh >= lcdRefreshTime && (prevSetpoint1!=setpoint1 || prevSetpoint2!=setpoint2)) {
    lastLCDRefresh = now;
    prevSetpoint1 = setpoint1;
    prevSetpoint2 = setpoint2;

    lcd.setCursor(0, 0);  // Set cursor to the first line, first column
    lcd.print("                ");

    lcd.setCursor(0, 0);
    //lcd.print("T1: ");     // Print label 
    lcd.print(input1, 0);  // Print temperature 1 with one decimal place
    lcd.print("C");
    lcd.print("/");
    lcd.print(setpoint1, 0);
    lcd.print("C");

    lcd.setCursor(10, 0);  // Set cursor to the second position on the second line
    lcd.print("H1:        ");
    lcd.setCursor(10, 0);  // Set cursor to the second position on the second line
    lcd.print("H1:");     // Print heater 2 label
    lcd.print((int)output1); // Print Heater 2 PWM value


    lcd.setCursor(0, 1);  // Set cursor to the first line, first column
    lcd.print("                ");

    lcd.setCursor(0, 1);  // Set cursor to the second position on the same line
    //lcd.print("T2: ");     // Print label

    lcd.print(input2, 0); // Print temperature 2 with one decimal place
    lcd.print("C");
    lcd.print("/");
    lcd.print(setpoint2, 0);
    lcd.print("C");

    lcd.setCursor(10, 1);  // Set cursor to the second line, first column
    lcd.print("H2:        ");
    lcd.setCursor(10, 1);  // Set cursor to the second line, first column
    lcd.print("H2:");     // Print heater 1 label
    lcd.print((int)output2);  // Print Heater 1 PWM value

            if(haltH1)
        {
            lcd.setCursor(10, 0);  // Set cursor to the second position on the second line
            lcd.print("H1:--- ");
        } 

        if(haltH2)
        {
            lcd.setCursor(10, 1);  // Set cursor to the second position on the second line
            lcd.print("H2:--- ");
        } 


  }
 }
void updateLCD()
{
    lcd.setCursor(0, 0);  // Set cursor to the first line, first column
    lcd.print("                ");

    lcd.setCursor(0, 0);
    //lcd.print("T1: ");     // Print label 
    lcd.print(input1, 0);  // Print temperature 1 with one decimal place
    lcd.print("C");
    lcd.print("/");
    lcd.print(setpoint1, 0);
    lcd.print("C");

    lcd.setCursor(10, 0);  // Set cursor to the second position on the second line
    lcd.print("H1:        ");
    lcd.setCursor(10, 0);  // Set cursor to the second position on the second line
    lcd.print("H1:");     // Print heater 2 label
    lcd.print((int)output1); // Print Heater 2 PWM value


    lcd.setCursor(0, 1);  // Set cursor to the first line, first column
    lcd.print("                ");

    lcd.setCursor(0, 1);  // Set cursor to the second position on the same line
    //lcd.print("T2: ");     // Print label

    lcd.print(input2, 0); // Print temperature 2 with one decimal place
    lcd.print("C");
    lcd.print("/");
    lcd.print(setpoint2, 0);
    lcd.print("C");

    lcd.setCursor(10, 1);  // Set cursor to the second line, first column
    lcd.print("H2:        ");
    lcd.setCursor(10, 1);  // Set cursor to the second line, first column
    lcd.print("H2:");     // Print heater 1 label
    lcd.print((int)output2);  // Print Heater 1 PWM value
}

void handleButtons(unsigned long now)
{

  static bool prevPlusState = HIGH;
  static bool prevMinusState = HIGH;

  static bool startContInc = false;
  static bool startContDec = false;
  static unsigned long currentStartDuration = 0;

  static unsigned long lastContPlusDown = 0;
  static unsigned long lastContMinusDown = 0;

  static bool resetToggleArmed = true;  // Prevent rapid toggling



    // Read current button states
  bool plusState = digitalRead(buttonPlusPin);
  bool minusState = digitalRead(buttonMinusPin);

  if(prevPlusState!=plusState){
    if(debug) Serial.println("plusState: " + String(plusState));
    if(prevPlusState==HIGH)
    {
      inc();
      startContInc = false;
      startContDec = false;
    }
    prevPlusState = plusState;
    lastContPlusDown = now;
    currentStartDuration = now;
  }


  if(prevMinusState!=minusState){
    if(debug) Serial.println("minusState: " + String(minusState));
    if(prevMinusState==HIGH)
    {
      dec();
      startContInc = false;
      startContDec = false;
    }
    prevMinusState = minusState;
    lastContMinusDown = now;
    currentStartDuration = now;
  }

  if(plusState==LOW && now - currentStartDuration >= contStartDuration)
    startContInc=true;

  if(minusState==LOW && now - currentStartDuration >= contStartDuration)
    startContDec=true;

  if(plusState==LOW && now - lastContPlusDown >= contUpdateDuration && startContInc)
  {
    lastContPlusDown = now;
    inc();
  }

  if(minusState==LOW  && now - lastContMinusDown >= contUpdateDuration && startContDec)
  {
    lastContMinusDown = now;
    dec();
  }


  //Both buttons pressed action
  if (!plusState && !minusState && haltH1 && haltH2 && resetToggleArmed) 
  {
    resetSafetyState(now);
    resetToggleArmed = false;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Hotdog Warmer :)");
    lcd.setCursor(0, 1);
    lcd.print("-- Resetting --");
    setSong(RESET);
    playing = true;
    delay(1500);


    //Serial.println(debug ? "Debug ON" : "Debug OFF");
  } else if (!plusState && !minusState) {
    resetToggleArmed = true;  // Re-arm once buttons are released
  }

}

void inc() {
    setpoint1 = min(setpoint1 + 1, maxSetpoint);
    setpoint2 = min(setpoint2 + 1, maxSetpoint);
  //updateLCD();
}

void dec() {
    setpoint1 = max(setpoint1 - 1, minSetpoint);
    setpoint2 = max(setpoint2 - 1, minSetpoint);
  //updateLCD();
}

void resetSafetyState(unsigned long now) {
  // Heater 1
    haltH1 = false;
    heater1InactivityDuration = 0;
    temp1WindowStart = input1;
    temp1WindowStartTime = now;

    // Heater 2
    haltH2 = false;
    heater2InactivityDuration = 0;
    temp2WindowStart = input2;
    temp2WindowStartTime = now;

    notify=true;
    playedDone = false;
   
    Serial.println("Safety check state reset.");
}

