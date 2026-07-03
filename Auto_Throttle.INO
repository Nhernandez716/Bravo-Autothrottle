/*
  Boeing Auto Throttle Sim Kit
  AutoThrottle.ino
  Uses Hall Effect sensors as limit detection (reserved, currently commented)
  Uses A4988 Stepper Motor Driver module
  Communicates with SPAD.neXt via CmdMessenger over Serial
*/

#include <CmdMessenger.h>
CmdMessenger messenger(Serial);

const String authkey = "d4908fd34123c5d12f043398d1b48052";
const String guid    = "{977a0e68-435f-4aeb-b317-cd9212449610}";

// ── Pin Definitions ───────────────────────────────────────────
// Left stepper
#define LEFT_DIR      7
#define LEFT_STEP     6
#define L_ENABLE      4
#define LEFT_MS_PINS  5

// Right stepper
#define RIGHT_DIR     12
#define RIGHT_STEP    11
#define R_ENABLE      9
#define RIGHT_MS_PINS 10

// Misc I/O  (Power_Switch removed; Parking_Indicator moved to unused pin 3)
#define PANIC_BUTTON       19
#define PARKING_INDICATOR   3   // was conflicting with RIGHT_DIR (12) — moved to pin 3
#define PARKING_BRAKE_SWITCH 18

// Hall effect limit sensors (reserved for future use)
// #define LEFT_HALL_SENSOR  2
// #define RIGHT_HALL_SENSOR 20

// ── Speed Thresholds (µs pulse delay — lower = faster) ────────
const int SPEED_FASTEST = 100;
const int SPEED_FASTER  = 200;
const int SPEED_NORMAL  = 500;
const int SPEED_SLOW    = 1000;

// ── State Variables ───────────────────────────────────────────
bool autoThrottleActive = HIGH; // HIGH = motors disabled on boot
bool stepApproved       = false;
bool isStarted          = false;

int  prkBrkSwitchLast = -1;

// ── Throttle Positions ────────────────────────────────────────
volatile int leftThrottleSPAD  = 0;  // Current sim position (left)
volatile int rightThrottleSPAD = 0;  // Current sim position (right)
volatile int leftThrottleRef   = 0;  // Target position (left)
volatile int rightThrottleRef  = 0;  // Target position (right)

// ── Helpers ───────────────────────────────────────────────────

// Returns the pulse delay (µs) based on how far away the target is
int speedForDiff(int diff) {
  if (diff >= 10) return SPEED_FASTEST;
  if (diff >= 3)  return SPEED_FASTER;
  if (diff == 2)  return SPEED_NORMAL;
  return SPEED_SLOW;
}

// Sends one step pulse on the given STEP pin with the given delay
void pulseStep(uint8_t stepPin, int pulseDelay) {
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(pulseDelay);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(pulseDelay);
}

// Enable or disable both motor drivers
void setMotorEnable(bool enable) {
  // A4988 ENABLE is active-LOW: LOW = enabled, HIGH = disabled
  uint8_t val = enable ? LOW : HIGH;
  digitalWrite(L_ENABLE, val);
  digitalWrite(R_ENABLE, val);
}

// ── Limit Switch ISRs (reserved) ──────────────────────────────
// void leftThrottleLimit()  { /* reverse or halt left motor */ }
// void rightThrottleLimit() { /* reverse or halt right motor */ }

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // Stepper outputs
  pinMode(LEFT_DIR,      OUTPUT);
  pinMode(LEFT_STEP,     OUTPUT);
  pinMode(RIGHT_DIR,     OUTPUT);
  pinMode(RIGHT_STEP,    OUTPUT);
  pinMode(LEFT_MS_PINS,  OUTPUT);
  pinMode(RIGHT_MS_PINS, OUTPUT);
  pinMode(L_ENABLE,      OUTPUT);
  pinMode(R_ENABLE,      OUTPUT);

  // Misc outputs
  pinMode(PARKING_INDICATOR, OUTPUT);


  // Inputs
  pinMode(PANIC_BUTTON,        INPUT);
  pinMode(PARKING_BRAKE_SWITCH, INPUT_PULLUP);

  // Hall effect interrupts (reserved)
  // pinMode(LEFT_HALL_SENSOR,  INPUT);
  // pinMode(RIGHT_HALL_SENSOR, INPUT);
  // attachInterrupt(digitalPinToInterrupt(LEFT_HALL_SENSOR),  leftThrottleLimit,  FALLING);
  // attachInterrupt(digitalPinToInterrupt(RIGHT_HALL_SENSOR), rightThrottleLimit, FALLING);

  // Microstepping: set MS pins HIGH (1/16 step or per your driver config)
  digitalWrite(LEFT_MS_PINS,  HIGH);
  digitalWrite(RIGHT_MS_PINS, HIGH);

  // Start with motors disabled
  setMotorEnable(false);

  attachCommandCallbacks();
}

// ── CmdMessenger Callbacks ────────────────────────────────────
void attachCommandCallbacks() {
  messenger.sendCmd(3, "ATTACHING CALLBACKS!");
  messenger.attach(0, onIdentifyRequest);
  messenger.attach(2, onSpadEvent);
  messenger.attach(3, onUnknownCommand);
  messenger.attach(6, onDeviceLed);
  messenger.attach(7, onDeviceDisplay);
}

void onUnknownCommand() {
  messenger.sendCmd(3, "UNKNOWN COMMAND");
}

void onSpadEvent() {
  char *szEvent = messenger.readStringArg();
  if (strcmp(szEvent, "START") == 0) { isStarted = true;  return; }
  if (strcmp(szEvent, "END")   == 0) { isStarted = false; return; }
}

void onIdentifyRequest() {
  char *szRequest = messenger.readStringArg();

  if (strcmp(szRequest, "INIT") == 0) {
    messenger.sendCmdStart(0);
    messenger.sendCmdArg("SPAD");
    messenger.sendCmdArg(guid);
    messenger.sendCmdArg("Auto Throttle");
    messenger.sendCmdArg(2);
    messenger.sendCmdArg("1");
    messenger.sendCmdArg("AUTHOR=" + authkey);
    messenger.sendCmdEnd();
    return;
  }

  if (strcmp(szRequest, "SCANSTATE") == 0) {
    messenger.sendCmdStart(0);
    messenger.sendCmdArg("STATESCAN");
    messenger.sendCmdEnd();
    return;
  }

  if (strcmp(szRequest, "PING") == 0) {
    messenger.sendCmdStart(0);
    messenger.sendCmdArg("PONG");
    messenger.sendCmdArg(messenger.readInt32Arg());
    messenger.sendCmdEnd();
    return;
  }

  if (strcmp(szRequest, "CONFIG") == 0) {

    // Options
    messenger.sendCmdStart(0);
    messenger.sendCmdArg("OPTION");
    messenger.sendCmdArg("ISGENERIC=1");
    messenger.sendCmdArg("PAGESUPPORT=1");
    messenger.sendCmdArg("NO_DISPLAY_CLEAR=1");
    messenger.sendCmdEnd();

    // INPUT: Parking Brake button
    messenger.sendCmdStart(0);
    messenger.sendCmdArg("INPUT");
    messenger.sendCmdArg(1);
    messenger.sendCmdArg("Parking_Brake");
    messenger.sendCmdArg("PUSHBUTTON");
    messenger.sendCmdArg("SPAD_SIMPLEBUTTON");
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
        messenger.sendCmdArg("PARKINGBREAK_LED");    //SPAD GUI Display name
        messenger.sendCmdArg("LED");   // Type
        messenger.sendCmdArg("SPAD_LED");  // Behaviour
        messenger.sendCmdEnd();

        messenger.sendCmdStart(0);
        messenger.sendCmdArg("OUTPUT");
        messenger.sendCmdArg(3);             // This is the LED ID 
        messenger.sendCmdArg("AUTOBREAK_LED");    //SPAD GUI Display name
        messenger.sendCmdArg("LED");   // Type
        messenger.sendCmdArg("SPAD_LED");  // Behaviour
        messenger.sendCmdEnd();

    // OUTPUT: Left throttle SPAD position (id 1000)
    sendDisplayOutput(1000, "L_Throttle_SPAD");

    // OUTPUT: Right throttle SPAD position (id 2000)
    sendDisplayOutput(2000, "R_Throttle_SPAD");

    // OUTPUT: Left throttle reference/target (id 3000)
    sendDisplayOutput(3000, "L_Throttle_REF");

    // OUTPUT: Right throttle reference/target (id 4000)
    sendDisplayOutput(4000, "R_Throttle_REF");

    messenger.sendCmd(0, "CONFIG");
    return;
  }
}

// Shared helper to reduce repetition in CONFIG block
void sendDisplayOutput(int id, const char* name) {
  messenger.sendCmdStart(0);
  messenger.sendCmdArg("OUTPUT");
  messenger.sendCmdArg(id);
  messenger.sendCmdArg(name);
  messenger.sendCmdArg("DISPLAY");
  messenger.sendCmdArg("SPAD_DISPLAY");
  messenger.sendCmdArg("LENGTH=7");
  messenger.sendCmdArg("ROW=1");
  messenger.sendCmdArg("WIDTH=150");
  messenger.sendCmdArg("HEIGHT=40");
  messenger.sendCmdEnd();
}

void onDeviceLed() {
  int32_t ledID  = messenger.readInt32Arg();
  int32_t ledVal = messenger.readInt32Arg();

  if (ledID == 1) {
    // Software enable gate for steppers
    ledVal ? enableSteppers() : disableSteppers();
  }
  if (ledID == 2) {
    digitalWrite(PARKING_INDICATOR, ledVal ? HIGH : LOW);
  }
  
}

void enableSteppers() {
  stepApproved = true;
  // Motors will be enabled inside step() only when movement is needed
}

void disableSteppers() {
  setMotorEnable(false);
  stepApproved = false;
}

void onDeviceDisplay() {
  int32_t displayID  = messenger.readInt32Arg();
  int32_t displayRow = messenger.readInt32Arg();  // unused but must be read
  int32_t displayCmd = messenger.readInt32Arg();
  double  displayVal = messenger.readDoubleArg();

  switch (displayID) {
    case 1000:
      if (displayCmd == 2) leftThrottleSPAD  = (int)displayVal;
      break;
    case 2000:
      if (displayCmd == 2) rightThrottleSPAD = (int)displayVal;
      break;
    case 3000:
      if (displayCmd == 2) leftThrottleRef   = (int)displayVal;
      break;
    case 4000:
      if (displayCmd == 2) rightThrottleRef  = (int)displayVal;
      break;
  }
}

// ── Motor Step Logic ──────────────────────────────────────────
void step() {

  // ── LEFT THROTTLE ─────────────────────────────────────────
  int leftDiff = leftThrottleRef - leftThrottleSPAD;

  if (leftDiff != 0) {
    int pd = speedForDiff(abs(leftDiff));
    setMotorEnable(true);
    // Positive diff → ref is higher → lever needs to go UP
    digitalWrite(LEFT_DIR, leftDiff > 0 ? HIGH : LOW);
    pulseStep(LEFT_STEP, pd);
  }

  // ── RIGHT THROTTLE ────────────────────────────────────────
  int rightDiff = rightThrottleRef - rightThrottleSPAD;

  if (rightDiff != 0) {
    int pd = speedForDiff(abs(rightDiff));
    setMotorEnable(true);
    // Positive diff → ref is higher → lever needs to go UP
    // RIGHT motor is physically mirrored, so direction is inverted
    digitalWrite(RIGHT_DIR, rightDiff > 0 ? LOW : HIGH);
    pulseStep(RIGHT_STEP, pd);
  }

  // Disable motors when both throttles are at target (saves heat/power)
  if (leftDiff == 0 && rightDiff == 0) {
    setMotorEnable(false);
  }
}

// ── Parking Brake Button ──────────────────────────────────────
void spadButtonCheck() {
  int currentState = digitalRead(PARKING_BRAKE_SWITCH);

  if (currentState != prkBrkSwitchLast) {
    prkBrkSwitchLast = currentState;

    messenger.sendCmdStart(8);
    messenger.sendCmdArg(1);                            // Input ID
    messenger.sendCmdArg(currentState == LOW ? 1 : 0); // Active-LOW: pressed = LOW = on
    messenger.sendCmdEnd();

    delay(25); // debounce
  }
}

// ── Main Loop ─────────────────────────────────────────────────
void loop() {
  messenger.feedinSerialData();

  if (isStarted) {
    spadButtonCheck();
    if (stepApproved) {
      step();
    }
  }
}
