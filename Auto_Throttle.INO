/*
  Stepper Motor Limit Switch Demo
  stepper-limit-demo.ino
  Uses Hall Effect sensors as limit switches
  Uses A4988 Stepper Motor Driver module
  
  DroneBot Workshop 2019
  https://dronebotworkshop.com
*/

// Define connections
#define HALL_SENSOR_A     18
#define HALL_SENSOR_B      19
#define LEFT_DIR      2
#define LEFT_STEP      3
#define RIGHT_DIR      4
#define RIGHT_STEP      5
#define LEFT_ms1       8
#define LEFT_ms2       9
#define LEFT_ms3      10
#define RIGHT_ms1       14
#define RIGHT_ms2       15
#define RIGHT_ms3      16

// Variables
int pd = 2000;       // Pulse Delay period
boolean Leftsetdir = LOW; // Set Direction
boolean Rightsetdir = LOW; // Set Direction

// Interrupt Handlers

void limit_a (){

  // Reverse motor
  Leftsetdir = !Leftsetdir;
  Rightsetdir = !Rightsetdir;
  
}

void limit_b (){

  // Reverse motor
  Leftsetdir = !Leftsetdir;
  Rightsetdir = !Rightsetdir;
  
}


void setup() {
  
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
  
  // Setup the Hall Effect switches as Inputs
  pinMode(HALL_SENSOR_A, INPUT);
  pinMode(HALL_SENSOR_B, INPUT);
  
  // Attach interrupt pins to handlers
  attachInterrupt(digitalPinToInterrupt(HALL_SENSOR_A), limit_a, FALLING);
  attachInterrupt(digitalPinToInterrupt(HALL_SENSOR_B), limit_b, FALLING);

  // Digital Write
  digitalWrite(LEFT_ms1, HIGH);
  digitalWrite(LEFT_ms2, HIGH);
  digitalWrite(LEFT_ms3, HIGH);
  digitalWrite(RIGHT_ms1, HIGH);
  digitalWrite(RIGHT_ms2, HIGH);
  digitalWrite(RIGHT_ms3, HIGH);

   
}

void loop() {
  
    digitalWrite(LEFT_DIR,Leftsetdir);
    digitalWrite(RIGHT_DIR,Rightsetdir);
    digitalWrite(LEFT_STEP,HIGH);
    digitalWrite(RIGHT_STEP,HIGH);
    delayMicroseconds(pd);
    digitalWrite(LEFT_STEP,LOW);
    digitalWrite(RIGHT_STEP,LOW);
    delayMicroseconds(pd);
 
}
