/*
  Boeing Auto Throttle Sim Kit
  xxx.ino
  Uses Hall Effect sensors as limit detection
  Uses A4988 Stepper Motor Driver module
  
*/

#include <CmdMessenger.h>
CmdMessenger messenger(Serial);

String authkey = "d4908fd34123c5d12f043398d1b48052";
String guid = "{977a0e68-435f-4aeb-b317-cd9212449610}"; // generated 10/10/2025

// Define connections
#define LEFT_HALL_SENSOR 18
#define RIGHT_HALL_SENSOR 19
#define LEFT_DIR 3
#define LEFT_STEP 2
#define RIGHT_DIR 5
#define RIGHT_STEP 4
#define ENABLE 23
#define LEFT_ms1 25
#define LEFT_ms2 27
#define LEFT_ms3 29
#define RIGHT_ms1 31
#define RIGHT_ms2 33
#define RIGHT_ms3 35
#define PANIC_Button 21
#define Power_Switch 13

#define AP_Disconnect 12

// Variables
int pd = 500;       // Pulse Delay period
bool Leftsetdir = LOW; // Set Direction
bool Rightsetdir = LOW; // Set Direction
bool AutoThrottleActive = LOW; // Set INI Motor State // HIGH = disabled

// Logic
bool Perform_Test = false; // Set INI var for test
bool hard_disable = false;
bool reset_needed = false;
bool spadInitReceived = 0;
bool isStarted = false;

int displayBrightnessLevel;

// Throttle Var's
int Left_Throttle_Ref = 0;
int Right_Throttle_Ref = 0;
int Left_Throttle_SPAD = 0;
int Right_Throttle_SPAD = 0;

// Interrupt Handlers

void Left_Throttle_Limit (){

  // Reverse motor
  Leftsetdir = !Leftsetdir;
  //Rightsetdir = !Rightsetdir;
  
}

void Right_Throttle_Limit (){

  // Reverse motor
  //Leftsetdir = !Leftsetdir;
  Rightsetdir = !Rightsetdir;
  
}



void setup() {

  Serial.begin(115200);
  
  // Setup the stepper controller pins as Outputs
  pinMode(LEFT_DIR,OUTPUT); 
  pinMode(LEFT_STEP,OUTPUT);
  pinMode(RIGHT_DIR,OUTPUT); 
  pinMode(RIGHT_STEP,OUTPUT);
  pinMode(LEFT_ms1,OUTPUT);
  pinMode(LEFT_ms2,OUTPUT);
  pinMode(LEFT_ms3,OUTPUT);
  pinMode(RIGHT_ms1,OUTPUT);
  pinMode(RIGHT_ms2,OUTPUT);
  pinMode(RIGHT_ms3,OUTPUT);
  pinMode(ENABLE,OUTPUT);
  pinMode(AP_Disconnect, OUTPUT);
  
  // Setup the Hall Effect switches as Inputs
  pinMode(LEFT_HALL_SENSOR, INPUT);
  pinMode(RIGHT_HALL_SENSOR, INPUT);
  pinMode(PANIC_Button, INPUT);
  pinMode(Power_Switch, INPUT);
  
  // Attach interrupt pins to handlers
  attachInterrupt(digitalPinToInterrupt(LEFT_HALL_SENSOR), Left_Throttle_Limit, FALLING);
  attachInterrupt(digitalPinToInterrupt(RIGHT_HALL_SENSOR), Right_Throttle_Limit, FALLING);

  // Digital Write
  digitalWrite(LEFT_ms1, HIGH);
  digitalWrite(LEFT_ms2, HIGH);
  digitalWrite(LEFT_ms3, HIGH);
  digitalWrite(RIGHT_ms1, HIGH);
  digitalWrite(RIGHT_ms2, HIGH);
  digitalWrite(RIGHT_ms3, HIGH);
  digitalWrite(ENABLE, AutoThrottleActive);

   attachCommandCallbacks();
}

void attachCommandCallbacks()
{
    messenger.sendCmd(3, "ATTACHING CALLBACKS!");
    messenger.attach(0, onIdentifyRequest); 
    messenger.attach(2, onSpadEvent);       
    messenger.attach(3, onUnknownCommand);
    messenger.attach(6, onDeviceLed);  
}

void onUnknownCommand()
{
  messenger.sendCmd(3, "UNKNOWN COMMAND"); 
}


void onSpadEvent()
{
  char *szEvent = messenger.readStringArg();
  if (strcmp(szEvent, "START") == 0) { // SPAD tells device, it's ok to send input now
    isStarted = 1;
    return;
  }
  if (strcmp(szEvent, "END") == 0) { // SPAD tells device it will exit now
    isStarted = 0;
    return;
  }
}

void onIdentifyRequest()
{
  char *szRequest = messenger.readStringArg();

  if (strcmp(szRequest, "INIT") == 0)
    {
        messenger.sendCmdStart(0);
        messenger.sendCmdArg("SPAD");
        messenger.sendCmdArg(guid); // GUID  
        messenger.sendCmdArg("Auto Throttle"); // DEVICE DISPLAY NAME
        messenger.sendCmdArg(2); // SPAD SERIAL VERSION, DON'T CHANGE
        messenger.sendCmdArg("1"); // DEVICE VERSION NUMBER        
        messenger.sendCmdArg("AUTHOR="+authkey); // AUTHOR ID - Edit the variable on top part of the code and remove // at the beginning of this line to activate your  auth code.     
        messenger.sendCmdEnd();
        return;
    }
    
  if (strcmp(szRequest, "SCANSTATE") == 0)
    {
        messenger.sendCmdStart(0);
        messenger.sendCmdArg("STATESCAN");
        messenger.sendCmdEnd();
        return;
    }

  if (strcmp(szRequest, "PING") == 0){
    messenger.sendCmdStart(0);
    messenger.sendCmdArg("PONG");
    messenger.sendCmdArg(messenger.readInt32Arg());
    messenger.sendCmdEnd();
    return;
  }

  if (strcmp(szRequest, "CONFIG") == 0){
        messenger.sendCmdStart(0);
        messenger.sendCmdArg("OPTION");
        messenger.sendCmdArg("ISGENERIC=1");
        messenger.sendCmdArg("PAGESUPPORT=1");
        messenger.sendCmdArg("NO_DISPLAY_CLEAR=1");
        messenger.sendCmdEnd();



//----- CREATE LED -----

        messenger.sendCmdStart(0);
        messenger.sendCmdArg("OUTPUT");
        messenger.sendCmdArg(1);             // This is the LED ID 
        messenger.sendCmdArg("ENABLE");    //SPAD GUI Display name
        messenger.sendCmdArg("LED");   // Type
        messenger.sendCmdArg("SPAD_LED");  // Behaviour
        messenger.sendCmdEnd();

        messenger.sendCmdStart(0);
        messenger.sendCmdArg("OUTPUT");
        messenger.sendCmdArg(2);             // This is the LED ID 
        messenger.sendCmdArg("AUTOBREAK_LED");    //SPAD GUI Display name
        messenger.sendCmdArg("LED");   // Type
        messenger.sendCmdArg("SPAD_LED");  // Behaviour
        messenger.sendCmdEnd();

       	//----- CREATE displays brightness control channel 11
  //messenger.sendCmd(0,F("OUTPUT,4000,D_active2,DISPLAY,SPAD_DISPLAY,LENGTH=7,ROW=1,WIDTH=150,HEIGHT=40,FOREGROUND=#FFFFFF,ORDER=3,UI.ROW=0"));delay(50);
  messenger.sendCmdStart(0);
  messenger.sendCmdArg("OUTPUT");
  messenger.sendCmdArg(1000);
  messenger.sendCmdArg("R_Throttle_REF");
  messenger.sendCmdArg("DISPLAY");
  messenger.sendCmdArg("SPAD_DISPLAY");
  messenger.sendCmdArg("LENGTH=7");
  messenger.sendCmdArg("ROW=1");
  messenger.sendCmdArg("WIDTH=150");
  messenger.sendCmdArg("HEIGHT=40");
  //messenger.sendCmdArg("FOREGROUND=#FFFFFF");
  //messenger.sendCmdArg("ORDER=3");
  //messenger.sendCmdArg("UI.ROW=0");
  messenger.sendCmdEnd();

  //messenger.sendCmd(0,F("OUTPUT,4000,D_active2,DISPLAY,SPAD_DISPLAY,LENGTH=7,ROW=1,WIDTH=150,HEIGHT=40,FOREGROUND=#FFFFFF,ORDER=3,UI.ROW=0"));delay(50);
  messenger.sendCmdStart(0);
  messenger.sendCmdArg("OUTPUT");
  messenger.sendCmdArg(2000);
  messenger.sendCmdArg("R_Throttle_REF");
  messenger.sendCmdArg("DISPLAY");
  messenger.sendCmdArg("SPAD_DISPLAY");
  messenger.sendCmdArg("LENGTH=7");
  messenger.sendCmdArg("ROW=1");
  messenger.sendCmdArg("WIDTH=150");
  messenger.sendCmdArg("HEIGHT=40");
  //messenger.sendCmdArg("FOREGROUND=#FFFFFF");
  //messenger.sendCmdArg("ORDER=3");
  //messenger.sendCmdArg("UI.ROW=0");
  messenger.sendCmdEnd();
// RADIO 6 digit displays 3000, 4000, 5000, 6000 (lenth = 7 to account for decimel)
  //messenger.sendCmd(0,F("OUTPUT,3000,D_active1,DISPLAY,SPAD_DISPLAY,LENGTH=7,ROW=1,WIDTH=150,HEIGHT=40,FOREGROUND=#FFFFFF,ORDER=3,UI.ROW=0"));delay(50);
  messenger.sendCmdStart(0);
  messenger.sendCmdArg("OUTPUT");
  messenger.sendCmdArg(3000);
  messenger.sendCmdArg("L_Throttle_REF");
  messenger.sendCmdArg("DISPLAY");
  messenger.sendCmdArg("SPAD_DISPLAY");
  messenger.sendCmdArg("LENGTH=7");
  messenger.sendCmdArg("ROW=1");
  messenger.sendCmdArg("WIDTH=150");
  messenger.sendCmdArg("HEIGHT=40");
  //messenger.sendCmdArg("FOREGROUND=#FFFFFF");
  //messenger.sendCmdArg("ORDER=3");
  //messenger.sendCmdArg("UI.ROW=0");
  messenger.sendCmdEnd();
  //messenger.sendCmd(0,F("OUTPUT,4000,D_active2,DISPLAY,SPAD_DISPLAY,LENGTH=7,ROW=1,WIDTH=150,HEIGHT=40,FOREGROUND=#FFFFFF,ORDER=3,UI.ROW=0"));delay(50);
  messenger.sendCmdStart(0);
  messenger.sendCmdArg("OUTPUT");
  messenger.sendCmdArg(4000);
  messenger.sendCmdArg("R_Throttle_REF");
  messenger.sendCmdArg("DISPLAY");
  messenger.sendCmdArg("SPAD_DISPLAY");
  messenger.sendCmdArg("LENGTH=7");
  messenger.sendCmdArg("ROW=1");
  messenger.sendCmdArg("WIDTH=150");
  messenger.sendCmdArg("HEIGHT=40");
  //messenger.sendCmdArg("FOREGROUND=#FFFFFF");
  //messenger.sendCmdArg("ORDER=3");
  //messenger.sendCmdArg("UI.ROW=0");
  messenger.sendCmdEnd();
//-----  END CONFIG ----

      messenger.sendCmd(0, "CONFIG");
      return;
    }
}

void onDeviceLed(){ 
  int32_t ledID = messenger.readInt32Arg();       // Led ID
  int32_t ledVal = messenger.readInt32Arg();      // Led value
  
  if(ledID == 1){(ledVal) ? (digitalWrite( ENABLE, LOW) ) : ( digitalWrite( ENABLE, HIGH) );}
  if(ledID == 2){(ledVal) ? (digitalWrite( AP_Disconnect, HIGH) ) : ( digitalWrite( AP_Disconnect, LOW) );}
  //if(ledID == 3){(ledVal) ? (backlight_brightness = 50 ) : ( backlight_brightness = 0); set_backlight();}
  //if(ledID == 4){(ledVal) ? (backlight_brightness = 50 ) : ( backlight_brightness = 0); set_backlight();}
  //if(ledID == 5){(ledVal) ? (BARO_display.showString("std") ) : ( BARO_display.showNumber(BARO_global) ) ;}

}

// ------------------------ PROCESS FUNCTIONS--------------------
void onDeviceData(){ 
  int32_t dataID = messenger.readInt32Arg();       // Data ID
  int32_t dataVal = messenger.readInt32Arg();      // Data value

  // Nothing to do yet
}

void onDeviceControlDisplayBrightness(){ 
  int32_t dataVal = messenger.readInt32Arg();      // Data value on channel 11
  // Led Displays Brightness (0 to 15)
  if (dataVal >= 0 && dataVal < 16)
  {
    //lc_squawk.setIntensity(0,dataVal);
    displayBrightnessLevel = dataVal;
  }
}

void onDeviceDisplay(){ 
  int32_t displayID = messenger.readInt32Arg();       // Display ID
  int32_t displayRow = messenger.readInt32Arg();      // Display Row
  int32_t displayCmd = messenger.readInt32Arg();      // Display Command (0 turn off, 1 turn on, 2 update)
  double displayVal = messenger.readDoubleArg();      // Display value
  int squack_code;
  // Throttle Var's
  // int Left_Throttle_Ref = 0;
  // int Right_Throttle_Ref = 0;
  // int Left_Throttle_SPAD = 0;
  // int Right_Throttle_SPAD = 0;
  switch (displayID) {
      case 1000:
        if (displayCmd == 2) 
        {//ATC_display.showNumber((int)displayVal);
        Left_Throttle_SPAD = (int)displayVal;}
        else if (displayCmd == 0) ;//ATC_display.clear();
      break;

      case 2000:
        if (displayCmd == 2) {//BARO_display.showNumber(displayVal);
        Right_Throttle_SPAD = (int)displayVal;}  // Floating point number
        else if (displayCmd == 0) ;//BARO_display.clear();
      break;

      case 3000:
        if (displayCmd == 2) 
        {//ATC_display.showNumber((int)displayVal);
        Left_Throttle_Ref = (int)displayVal;}
        else if (displayCmd == 0) ;//ATC_display.clear();
      break;

      case 4000:
        if (displayCmd == 2) {//BARO_display.showNumber(displayVal);
        Right_Throttle_Ref = (int)displayVal;}  // Floating point number
        else if (displayCmd == 0) ;//BARO_display.clear();
      break;
  }
}

void step(){

    // Throttle Var's
  // int Left_Throttle_Ref = 0;
  // int Right_Throttle_Ref = 0;
  // int Left_Throttle_SPAD = 0;
  // int Right_Throttle_SPAD = 0;

    if (hard_disable == true && reset_needed == false){
      digitalWrite(ENABLE, AutoThrottleActive);
      hard_disable = false;
    }
    if (Left_Throttle_SPAD != Left_Throttle_Ref){
      if (Left_Throttle_SPAD < Left_Throttle_Ref){

      }
    }
    digitalWrite(LEFT_DIR,Leftsetdir);
    digitalWrite(RIGHT_DIR,Rightsetdir);
    digitalWrite(LEFT_STEP,HIGH);
    digitalWrite(RIGHT_STEP,HIGH);
    delayMicroseconds(pd);
    digitalWrite(LEFT_STEP,LOW);
    digitalWrite(RIGHT_STEP,LOW);
    delayMicroseconds(pd);
}

void loop() {
  if (isStarted){
    if (digitalRead(Power_Switch) != LOW){
      hard_disable = true;
      reset_needed = false;
      digitalWrite(ENABLE, HIGH);
      }
    else{
    if (digitalRead(PANIC_Button) != HIGH){
      digitalWrite(ENABLE, HIGH);
      reset_needed = true;
    } else
    if (ENABLE == LOW){
    step();}
    }

    step();
 
}

messenger.feedinSerialData();}
