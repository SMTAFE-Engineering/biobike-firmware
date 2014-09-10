
/*
BioBike V0.2 Firmware
 Daniel Harmsworth, 2014-09
 Challenger Institute of Technology
 */

#include <cstdlib>
#include <EEPROM.h>

#define ACTUATOR_COUNT 4

#define MAXCHAR 20
#define COMMANDDEPTH 10
#define NEWLINE 10

#define A1_DIR 8
#define A1_PWM 9
#define A1_REF A3
#define A1_DEZ 5

#define A2_DIR 7
#define A2_PWM 6
#define A2_REF A2
#define A2_DEZ 5

#define A3_DIR 4
#define A3_PWM 5
#define A3_REF A1
#define A3_DEZ 5

#define A4_DIR 2
#define A4_PWM 3
#define A4_REF A0
#define A4_DEZ 5

int actuatorDirPins[] = {
  A1_DIR, A2_DIR, A3_DIR, A4_DIR};
int actuatorPWMPins[] = {
  A1_PWM, A2_PWM, A3_PWM, A4_PWM};
int actuatorRefPins[] = {
  A1_REF, A2_REF, A3_REF, A4_REF};
int actuatorDeadzones[] = {
  A1_DEZ, A2_DEZ, A3_DEZ, A4_DEZ};

int actuatorMinimums[] = {
  0, 0, 0, 0};
int actuatorMaximums[] = {
  255, 255, 255, 255};

int actuatorHardLimitsUpper[] = {
  255, 255, 255, 255};
int actuatorHardLimitsLower[] = {
  0, 0, 0, 0};

int actuatorTargets[ACTUATOR_COUNT];
int actuatorErrors[ACTUATOR_COUNT];

char command[COMMANDDEPTH][MAXCHAR];

int stringPos = 0;
int commandPos = 0;


void setup() {
  Serial.begin(9600);
  Serial.println("BioBike Control System v0.2 starting...");
  Serial.print("Configuring I/O... ");
  for ( int curActuator = 0; curActuator < ACTUATOR_COUNT; curActuator++ )
  {
    pinMode(actuatorDirPins[curActuator], OUTPUT);
    pinMode(actuatorPWMPins[curActuator], OUTPUT);
    pinMode(actuatorRefPins[curActuator], INPUT);
    actuatorTargets[curActuator] = getActuatorPosition(actuatorRefPins[curActuator]); //Set the target position to the actuators current position, stops the machine moving around on boot.
  }
  Serial.println("Done!");

  Serial.print("Loading configuration from EEPROM... ");
  loadSettings();  //Load persistent settings from EEPROM
  Serial.println("Done!");

  showMenu();
}

void loop() {
  moveActuators();
  if (Serial.available() > 0) {
    char incomingByte = Serial.read();
    if (incomingByte != NEWLINE)
    {
      if ( incomingByte == ' ' ) { commandPos++; stringPos = 0; }

      command[commandPos][stringPos] = incomingByte;
      stringPos++;
    }
    else
    {      
      if ( strcmp(command[0],"showpos") == 0 ) { 
        showPositions();
      }

      //Actuator movement commands
      if ( strcmp(command[0], "move") == 0 )
      {
        int actuatorToMove = atoi(command[1]);
        int newPos = atoi(command[2]);
        Serial.print("Moving Actuator ");
        Serial.print(actuatorToMove);
        Serial.print(" to new position ");
        Serial.println(newPos);
        actuatorTargets[actuatorToMove - 1] = newPos;
      }

      if ( strcmp(command[0], "save") == 0 ) { 
        saveSettings(); 
      }
      
      for ( int i = 0; i < COMMANDDEPTH; i++ ) { memset(command[i], 0, MAXCHAR); }
      commandPos = 0;
      stringPos = 0;
    }
  }
}

void moveActuators() {
  for ( int curActuator = 0; curActuator < ACTUATOR_COUNT; curActuator++ )
  {
    int curPosition = getActuatorPosition(curActuator);
    int curError;// = abs(actuatorTargets[curActuator]-curPosition);
    int curErDir;
    if (curPosition < actuatorTargets[curActuator]) { 
      curErDir = HIGH; 
      curError = abs(actuatorTargets[curActuator] - curPosition); 
    } 
    else { 
      curErDir = LOW; 
      curError = abs(curPosition - actuatorTargets[curActuator]); 
    }
    actuatorErrors[curActuator] = curError;

    if ( curError >= actuatorDeadzones[curActuator])
    {
      digitalWrite(actuatorDirPins[curActuator], curErDir);
      int scaler = curError; //Seriously, replace this with a less horrible acceleration curve. YOUR CODE IS BAD AND YOU SHOULD FEEL BAD!
      if (scaler < 64 ) { 
        scaler = 64; 
      }
      analogWrite(actuatorPWMPins[curActuator], scaler);
    }
    else
    {
      analogWrite(actuatorPWMPins[curActuator], 0);
    }
  }
}

int getActuatorPosition(int actuatorNumber) {
  int actuatorPos = analogRead(actuatorRefPins[actuatorNumber]); 
  return(map(actuatorPos, 0, 1023, 0, 255)); // Discard the LSBs as there be dragons. Also WAAAAY too much noise, f**k it i'll add caps to clean it up later.
}

int getActuatorPositionScaled(int actuatorNumber) {
  int actuatorPos = getActuatorPosition(actuatorNumber); 
  return(map(actuatorPos, actuatorMinimums[actuatorNumber], actuatorMaximums[actuatorNumber], 0, 100));
}

void setActuatorPositionScaled(int actuatorNumber, int positionSetting)
{

}

void printHelp() {
  Serial.println("BioBike Control System v0.1");
}

void showPositions() {
  for ( int curActuator = 0; curActuator < ACTUATOR_COUNT; curActuator++ )
  {
    Serial.print("Actuator ");
    Serial.print(curActuator);
    Serial.print(": Position: "); 
    Serial.print(getActuatorPositionScaled(curActuator));
    Serial.print(": Raw Position: "); 
    Serial.print(getActuatorPosition(curActuator));
    Serial.print(" Setpoint: ");
    Serial.print(actuatorTargets[curActuator]);
    Serial.print(" Error: ");
    Serial.println(actuatorErrors[curActuator]);
  }
}

void showMenu()
{
  Serial.println("Fix Me");
}

void loadSettings()
{
  for ( int curActuator = 0; curActuator < ACTUATOR_COUNT; curActuator++ )
  {
    int eepromOffset = curActuator * 5;
    actuatorMinimums[curActuator] = EEPROM.read(eepromOffset);
    actuatorMaximums[curActuator] = EEPROM.read(eepromOffset + 1);
    actuatorDeadzones[curActuator] = EEPROM.read(eepromOffset + 2);
    actuatorHardLimitsUpper[curActuator] = EEPROM.read(eepromOffset + 3);
    actuatorHardLimitsLower[curActuator] = EEPROM.read(eepromOffset + 4);
  }
}

void saveSettings()
{
  Serial.print("Saving settings to EEPROM... ");
  for ( int curActuator = 0; curActuator < ACTUATOR_COUNT; curActuator++ )
  {
    int eepromOffset = curActuator * 4;
    EEPROM.write(eepromOffset, actuatorMinimums[curActuator]);
    EEPROM.write(eepromOffset + 1, actuatorMaximums[curActuator]);
    EEPROM.write(eepromOffset + 2, actuatorDeadzones[curActuator]);
    EEPROM.write(eepromOffset + 3, actuatorHardLimitsUpper[curActuator]);
    EEPROM.write(eepromOffset + 4, actuatorHardLimitsLower[curActuator]);
  }
  Serial.println("Done!");
}

void autoProbeLimits()
{
  for ( int curActuator = 0; curActuator < ACTUATOR_COUNT; curActuator++ )
  {
    int lastMillis = millis();
    Serial.print("Probing minimum for actuator ");
    Serial.print(curActuator);
    Serial.print("...");
    int lastReading = 255;      //Set the last reading to as far from the target as possible
    bool stillMoving = true;
    actuatorTargets[curActuator] = actuatorHardLimitsLower[curActuator];
    while (stillMoving) {
      moveActuators();
      //Only do a delta ref check if its been 1000mS since we last checked
      if ( (millis() - lastMillis) >= 1000 ) {
        if ( getActuatorPosition(actuatorRefPins[curActuator]) ==  lastReading ) { 
          stillMoving = false; 
        }
      }
    }
    actuatorMinimums[curActuator] = getActuatorPosition(actuatorRefPins[curActuator]);
    Serial.println(" Done!");

    lastMillis = millis();
    Serial.print("Probing maximum for actuator ");
    Serial.print(curActuator);
    Serial.print("...");
    lastReading = 0;      //Set the last reading to as far from the target as possible
    stillMoving = true;
    actuatorTargets[curActuator] = actuatorHardLimitsUpper[curActuator];
    while (stillMoving) {
      moveActuators();
      //Only do a delta ref check if its been 1000mS since we last checked
      if ( (millis() - lastMillis) >= 1000 ) {
        if ( getActuatorPosition(actuatorRefPins[curActuator]) ==  lastReading ) { 
          stillMoving = false; 
        }
      }
    }
    actuatorMaximums[curActuator] = getActuatorPosition(actuatorRefPins[curActuator]);
    Serial.println(" Done!");
  }
}

