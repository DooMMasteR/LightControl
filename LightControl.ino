
#include <SoftTimer.h>
#include <EEPROM.h>
#include <DelayRun.h>

/*
 LightControl

 PWM_PIN does 8 Bit PWM for a driver MOSFET or direct LED.
 UP and DOWN_KEY are buttons that pull down to GND, they dimm the light.
 The current brightness is saved after adjustment and wil be reset on power up.
 There is linear wear leveling (except for address 0) to prevent quick EEPROM wear.
 */

#define PWM_PIN 0
#define UP_KEY_PIN 3
#define DOWN_KEY_PIN 4
#define EEPROM_SIZE 512

int currentPWM = 5;
bool keyUp = false;
bool keyDown = false;
bool justsaved = false;
bool needssave = false;

/*
 simply periodically writes the currentPWM value to the PWM_PIN
 */
void setPwm(Task* me) {
  analogWrite(PWM_PIN, currentPWM);
}
Task tSetPwm(800, setPwm);

/*
 Saves the current dimming value to the first free EEPROM address
 and 'formats' the EEPROM when it is full.
 */
boolean savePwm(Task* me){
  if(EEPROM.read(511) != 0) eeprominit(true);
  for(int i=1; i < EEPROM_SIZE; i++){
    if(EEPROM.read(i) == 0){
      EEPROM.write(i, currentPWM);
      delay(10);
      break;
    }
  }
  needssave = false;
  justsaved = false;
  return false;
}
DelayRun delaySave(5000, savePwm);

/*
 Periodically change the dimming when keys are pressed
 */
void procKeys(Task* me){
  int oldPWM = currentPWM;
  if(keyUp && currentPWM < 255){
    currentPWM++;
    if(currentPWM > 32 && currentPWM < 255){
      currentPWM++;
      if(currentPWM > 128 && currentPWM < 255){
        currentPWM++;
      }
    }
  } else {
    if(keyDown && currentPWM > 1){
      currentPWM--;
      if(currentPWM > 32){
        currentPWM--;
        if(currentPWM > 128){
          currentPWM--;
        }
      }
    }
  }
  if(oldPWM != currentPWM) needssave = true;
}
Task tProcKeys(10000, procKeys);


/*
 Check if key has been pressed and trigger a save when the currentPWM value changed.
 */
void getKeys(Task* me) {
  if(digitalRead(UP_KEY_PIN) == LOW){
    keyUp = true;
  } else {
    keyUp = false;
  }
  if(digitalRead(DOWN_KEY_PIN) == LOW){
    keyDown = true;
  } else {
    keyDown = false;
  }
  if(keyDown || keyUp){
    SoftTimer.add(&tProcKeys);
  } else {
    SoftTimer.remove(&tProcKeys);
    if(needssave && !justsaved){
      delaySave.startDelayed();
      justsaved = true;
    }
  }
}
Task tGetKeys(200, getKeys);

/*
 Read the last brightness from EEPROM and set it
 */
void readLastBrigthness(){
  int cAddr = 1;
  while(cAddr < EEPROM_SIZE){
    int cVal = EEPROM.read(cAddr);
    if(cVal == 0) break;
    currentPWM = cVal;
    cAddr++;
  }
}

void eeprominit(){
  eeprominit(false);
}

/*
 Format the EEPROM and set the 'formatted' flag on address 0.
 @params: force formats the EEPROM even when the 'formatted' flag is set
 */
void eeprominit(bool force){
  if(EEPROM.read(0) != 128 || force){
    for(int i = 1; i < EEPROM_SIZE; i++){
       EEPROM.write(i, 0);
    }
    EEPROM.write(0, 128);
  }
  delay(10);
}

void setup() {
  //turn the PWM_PIN off to limit the 'flash'
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);

  delay(10);
  //init EEPROM and read last currentPWM from it
  eeprominit();
  readLastBrigthness();

  //switch to fastPWM with 4kHz @8MHz core clock
  TCCR0A = 2<<COM0A0 | 2<<COM0B0 | 3<<WGM00;
  TCCR0B = 0<<CS02 | 1<<CS01 | 0<<CS00;

  //setup the input pins and their pullup resistors
  pinMode(UP_KEY_PIN, INPUT_PULLUP);
  pinMode(DOWN_KEY_PIN, INPUT_PULLUP);

  //start our periodic calls to the 'management' functions
  SoftTimer.add(&tSetPwm);
  SoftTimer.add(&tGetKeys);
  
}



