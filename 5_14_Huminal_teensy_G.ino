//***it stops on head8() moving the head, and head7()moving the right leg, but completes its cycle and goes on with head(=6() moving the left leg


// Head motor https://www.pololu.com/product/2267
// Right and Left motor https://www.pololu.com/product/1477 , current limit voltage: .3-.4V
// Encoders https://www.mouser.com/datasheet/2/670/amt10-v-1310331.pdf .4-.5V
/* https://www.pjrc.com/teensy/teensyLC.html
 * Teensy LC pinout
 * 
 * 3 Right motor STEP
 * 4 Left motor STEP
 * 5 Head motor STEP  
 * 6 Right motor DIR
 * 7 Left motor DIR
 * 8 Head motor DIR
 * 9 Right encoder A (white)
 * 10 Right encoder B (yellow)
 * 11 Left encoder A (white)
 * 12 Left encoder B (yellow)
 * 13 Onboard LED
 * 14 Head encoder B (yellow)
 * 15 Head encoder A (white) 
 * 16 
 * 17 Light circuit trigger
 * 18 
 * 19 
 * 20 Input trigger from Attiny85 or radio
 */
//https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html#af4d3818e691dad5dc518308796ccf154
#include <AccelStepper.h>
//https://github.com/PaulStoffregen/Encoder
#include <Encoder.h>

int distanceToUse(int sensorNum);
float whichSensor();

// Onboard led used for diagnostics
int led = 13;

// Input trigger from Attiny85 to turn on lights
int triggerPin = 20; 
// Flag to move head
volatile bool moveHead = false;
// Trigger high to turn on lights
int lightOn = 17; 

float sensor0 = 16;
float sensor1 = 17;
float sensor2 = 18;
float sensor3 = 19;
float sensor4 = 20;
float sensor5 = 21;

// Speed variables
float speedR = 50;
float speedL = 50;
float speedH = 10;

// Acceleration variables
float accelFullR = 50;
float accelFullL = 50;
float accelFullH = 40;

// Distance to turn head and initial head position
int headTurnDis = 50;
int headPosition = 0;

// Microstep settings for feet and head
float microStep = 16;
float microStepH = 4;

// Define maximum step speed by multiplying speed and microstep settings
int maxStepSpeedR = round(speedR*microStep);
int maxStepSpeedL = round(speedL*microStep);
int maxStepSpeedH = round(speedH*microStepH);

// Define maximum step acceleration by multiplying speed and microstep settings
int acclR = round(accelFullR*microStep);
int acclL = round(accelFullL*microStep);
int acclH = round(accelFullH*microStepH);

// Cutoff steps so that there is no deaccleration (AccelStepper library has acceleration and deacceleration built in)
int earlyStopSteps = 800; // cutoff steps so that there is no deaccleration

// Define motors
AccelStepper Rmot(1, 3, 6); // pin 3 = step, pin 6 = direction
AccelStepper Lmot(1, 4, 7); // pin 4 = step, pin 7 = direction
AccelStepper Hmot(1, 5, 8); // pin 5 = step, pin 8 = direction

// Define encoders
Encoder encR(9, 10); // pin 9 = right encoder A, pin 10 = right encoder B
Encoder encL(11, 12); // pin 11 = left encoder A, pin 12 = left encoder B
Encoder encH(15, 14); // pin 15 = head encoder A, pin 14 = head encoder B

float turnRatio = .6805; // How slow the inner motor goes during a turn

// Arrays that determine path
// 200 stepper motor steps = 1 revolution, 800 encoder steps = 1 revolution
float straight = 1000*microStep;
float turnLeft = 1872.5*microStep;
float turnRight=-1872.5*microStep;
float turnHead = 1000*microStepH;

float lengths[] = {straight, straight, straight, straight};
float len;
float steps;
char motions[] = {'S', 'S', 'S', 'S'};
char motion;

int numComs = 4; // number of commands

// Indice variable
int ind = 0;

int currentSpeedR = 0;
int currentSpeedL = 0;

void setup() {
  Serial.begin(57600);
  // setup interupt 
  //attachInterrupt(digitalPinToInterrupt(triggerPin), triggered, RISING);

  // Set diagnostic led as output
  pinMode(led, OUTPUT);
  
  // Set pin as output
  pinMode(lightOn, OUTPUT);

  pinMode(sensor0, INPUT);
  pinMode(sensor1, INPUT);
  pinMode(sensor2, INPUT);
  pinMode(sensor3, INPUT);
  pinMode(sensor4, INPUT);
  pinMode(sensor5, INPUT);
  
  // Invert left motor
  Lmot.setPinsInverted(true,false,false);
  
  // Define head turn steps taking into account microstep settings
  headTurnDis = headTurnDis*microStepH;

  // Setup motors, passing pulsewidth in microseconds
  setupMotors(50);

  // Start moving with acceleration, acceleration is required to overcome initial torque
  startupAccel(lengths[0]);
}

void loop() {
  // Loop through motion and lengths defined above, set encoder position to 0
  motion = motions[ind];
  len = lengths[ind];
  encR.write(0);
  encL.write(0);

  // Switch through which motion to perform
  switch (motion)
  {
    case 'S': //takes 5 seconds
      Serial.println("straight");
      straightMove(len);
      break;
    case 'R': //takes 25 seconds
      Serial.println("right");
      right(len);
      break;
    case 'L': //takes 47 seconds (why twice as long, should also be 25 secongs)
      Serial.println("left");
      left(len);
      break;
    case 'H': //doesn't stop
      Serial.println("head"); //(takes 10 seconds)
      head11(len);
      break;
    default:
      break;
  }

  Serial.print("Sensor reading: ");
  Serial.print(whichSensor());
  Serial.println();
  if(whichSensor()) {
    head11(len);
  }

  // After motion is complete, increment ind and reset ind to 0 if last command in array was performed (ind >= numComs)
  ind = ind + 1;
  if(ind >= numComs)
  {
    ind = 0;
  }
}

void startupAccel(float len)
{
  // Reset encoder position
  encR.write(0);
  encL.write(0);

  // Move len plus earlyStopSteps with acceleration
  Rmot.moveTo(len + earlyStopSteps);
  Lmot.moveTo(len + earlyStopSteps);

  // With earlyStopSteps left to go, end motion to avoid any deaccleration
  while(Rmot.distanceToGo() > earlyStopSteps && Lmot.distanceToGo() > earlyStopSteps)
  {
    // If moveHead flag is set, stop motion and turn head, continue motion with acceleration
    if(moveHead)
    {
      headMotion();
      Rmot.moveTo(len + earlyStopSteps - Rmot.distanceToGo());
      Lmot.moveTo(len + earlyStopSteps - Rmot.distanceToGo());
    }
    // Needs to be called continously and quickly in order for stepper motors to turn
    Rmot.run();
    Lmot.run();
  }
  // Set ind to 1 to move onto next motion
  ind = 1;
  
  // Reset right and left motors position
  Rmot.setCurrentPosition(0);
  Lmot.setCurrentPosition(0);
}

void straightMove(float len)
{
  float encLen = len;
  // Convert stepper motor len (# of steps) to enc len (# of encoder ticks)
  encLen = ((encLen/microStep));
  
  // Reset encoder position
  encR.write(0);
  encL.write(0);
  
  // Set left and right motor speed
  Rmot.setSpeed(maxStepSpeedR);
  Lmot.setSpeed(maxStepSpeedL);

  // While left encoder has yet to reach positon, continue moving
      Serial.print("encL"); 
    Serial.print(encL.read());
    if(encL.read() >= -encLen) // change this back to while later once the encoder is working
  {
    Serial.print("encLSTRAIT"); 
    Serial.print(encL.read());
    Serial.println(); 
    Serial.print("-encLen "); 
    Serial.print(-encLen);
    Serial.println();
    // If moveHead flag is set, stop motion and turn head, continue motion with acceleration
    if(moveHead)
    {
      headMotion();
      Rmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
      Lmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
    }
    // Needs to be called continously and quickly in order for stepper motors to turn
    Rmot.runSpeed();
    Lmot.runSpeed();
  }
  
  // Reset right and left motors position
  Rmot.setCurrentPosition(0);
  Lmot.setCurrentPosition(0);
}

void right(float len)
{
  float encLen = (float)len;
  // Convert stepper motor len (# of steps) to enc len (# of encoder ticks)
  encLen = ((encLen / microStep) * 4);

  // Reset encoder position
  encR.write(0);
  encL.write(0);

  // Set left and right motor speed
  Rmot.setSpeed(maxStepSpeedR * turnRatio / 1.25);
  Lmot.setSpeed(maxStepSpeedL / 1.25);

  // While left encoder has yet to reach positon, continue moving
  while (encL.read() <= encLen)
  {
    Serial.print("encRIGHTleft"); 
    Serial.print(encL.read());
    Serial.println(); 
    Serial.print("encLen "); 
    Serial.print(encLen);
    Serial.println(); 
    // If moveHead flag is set, stop motion and turn head, continue motion with acceleration
    if (moveHead)
    {
      headMotion();
      Rmot.moveTo(len + earlyStopSteps - (encL.read()*microStep) / 4);
      Lmot.moveTo(len + earlyStopSteps - (encL.read()*microStep) / 4);
    }
    // Needs to be called continously and quickly in order for stepper motors to turn
    Rmot.runSpeed();
    Lmot.runSpeed();
  }

  // Reset right and left motors position
  Rmot.setCurrentPosition(0);
  Lmot.setCurrentPosition(0);
}



void left(float len)
{
  float encLen = (float)len;
  // Convert stepper motor len (# of steps) to enc len (# of encoder ticks)
  encLen = ((encLen/microStep)*4);

  // Reset encoder position
  encR.write(0);
  encL.write(0);

  // Set left and right motor speed
  Rmot.setSpeed(maxStepSpeedR/1.25);
  Lmot.setSpeed((maxStepSpeedL*turnRatio)/1.25);

  // While left encoder has yet to reach positon, continue moving
  while(encL.read() >= (-encLen*turnRatio))
  {
    Serial.print("encLEFT"); 
    Serial.print(encL.read());
    Serial.println(); 
    Serial.print("encLen "); 
    Serial.print(encLen);
    Serial.println(); 
    // If moveHead flag is set, stop motion and turn head, continue motion with acceleration
    if(moveHead)
    {
      headMotion();
      Rmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
      Lmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
    }
    // Needs to be called continously and quickly in order for stepper motors to turn
    Rmot.runSpeed();
    Lmot.runSpeed();
  }
  
  // Reset right and left motors position
  Rmot.setCurrentPosition(0);
  Lmot.setCurrentPosition(0);
}

// Interrupt function called when input is recieved from attiny85
void triggered()
{
  moveHead = true;
  digitalWrite(lightOn, HIGH);
}

// diagnostic function to rapidly blink onboard led
void blinkLED()
{
  digitalWrite(led, HIGH);
  delay(100);
  digitalWrite(led, LOW);
  delay(100);
}

// Set pulsewidth, max speed and acceleration of motors
void setupMotors(int pulsewidth)
{
  Rmot.setMinPulseWidth(pulsewidth);
  Lmot.setMinPulseWidth(pulsewidth);
  Hmot.setMinPulseWidth(pulsewidth);
  
  Rmot.setMaxSpeed(maxStepSpeedR);
  Lmot.setMaxSpeed(maxStepSpeedL);
  Hmot.setMaxSpeed(maxStepSpeedH);

  Rmot.setAcceleration(acclR);
  Lmot.setAcceleration(acclL);
  Hmot.setAcceleration(acclH);
}

// Head motion to be performed when triggered
void headMotion()
{
  delay(500);
  moveHeadWithEnc_CW(headTurnDis);
  delay(500);
  moveHeadWithEnc_CCW(headTurnDis);
  delay(500);
  moveHeadWithEnc_CCW(headTurnDis);
  delay(500);
  moveHeadWithEnc_CW(headTurnDis);
  delay(500);
  moveHead = false;
  digitalWrite(lightOn, LOW);
}

// Uses .move for startup acceleration and sets speed continously as head movement can be loose and problematic
void moveHeadWithEnc_CW(int headTurnDis)
{
  float speedMotor = 0;

  // Reset encoder, and motor position
  encH.write(0);
  headPosition = 0;
  Hmot.setCurrentPosition(0);

  // Start moving
  Hmot.move(headTurnDis);

  // Move until desired head position is passed
  while((headTurnDis - headPosition) > 0)
  {
    // Needs to be called continously in order to run
    Hmot.run();
    
    // Read head position and convert to steps
    headPosition = convertEncToStep(encH.read());
    
    // Capture current speed of motor
    speedMotor = Hmot.speed();

    // Move command again
    Hmot.move(headTurnDis - headPosition);

    // Set speed
    Hmot.setSpeed(speedMotor);
  }
}

void moveHeadWithEnc_CCW(int steps)
{
  float speedMotor = 0;

  // Reset encoder, and motor position
  encH.write(0);
  headPosition = convertEncToStep(encH.read());
  Hmot.setCurrentPosition(0);
  
  // Start moving
  Hmot.move(-steps);

  // Move until desired head position is passed
  while((-steps - headPosition) < 0)
  {
    // Needs to be called continously in order to run
    Hmot.run();

    // Read head position and convert to steps
    headPosition = convertEncToStep(encH.read());
    
    // Capture current speed of motor
    speedMotor = Hmot.speed();

    // Move command again
    Hmot.move(-steps - headPosition);

    // Set speed
    Hmot.setSpeed(speedMotor);
  }
}

void head6(float len)
{
  float encLen = len;
  // Convert stepper motor len (# of steps) to enc len (# of encoder ticks)
  encLen = (encLen/microStep);
  
  // Reset encoder position
  encL.write(0);
  
  // Set left and right motor speed
  Lmot.setSpeed(maxStepSpeedL);

  // While left encoder has yet to reach positon, continue moving
      Serial.print("encL"); 
    Serial.print(encL.read());
  while(encL.read() >= -encLen)
  {
   // If moveHead flag is set, stop motion and turn head, continue motion with acceleration
    if(moveHead)
    {
      headMotion();
      Rmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
      Lmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
    }
    
    // Needs to be called continously and quickly in order for stepper motors to turn
    Lmot.runSpeed();
  }
  
  // Reset right and left motors position
  Lmot.setCurrentPosition(0);

  
}

void head7(float len)
{
  float encLen = len;
  // Convert stepper motor len (# of steps) to enc len (# of encoder ticks)
  encLen = ((encLen/microStep));
  
  // Reset encoder position
  encR.write(0);
  
  // Set left and right motor speed
  Rmot.setSpeed(maxStepSpeedR);

  // While left encoder has yet to reach positon, continue moving
  while(encR.read() <= encLen)

  {
    Serial.print("encRIGHT"); 
    Serial.print(encR.read());
    Serial.println(); 
    Serial.print("encLen "); 
    Serial.print(encLen);
    Serial.println(); 

    // If moveHead flag is set, stop motion and turn head, continue motion with acceleration
    if(moveHead)
    {
      headMotion();
      Rmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
      Lmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
    }
    // Needs to be called continously and quickly in order for stepper motors to turn
    Rmot.runSpeed();
  }
  
  // Reset right and left motors position
  Rmot.setCurrentPosition(0);
  Lmot.setCurrentPosition(0);
}

void head8(float len)
{
  float encLen = len;
  // Convert stepper motor len (# of steps) to enc len (# of encoder ticks)
  encLen = (encLen/microStepH);
  
  // Reset encoder position
  encH.write(0);
  
  // Set left and right motor speed
  Hmot.setSpeed(maxStepSpeedH);

  // While left encoder has yet to reach positon, continue moving
  while(encH.read() >= -encLen)
  {
    Serial.print("encR"); 
    Serial.print(encR.read());
    Serial.println(); 
    Serial.print("-encLen "); 
    Serial.print(-encLen);
    Serial.println();
   // If moveHead flag is set, stop motion and turn head, continue motion with acceleration
    if(moveHead)
    {
      headMotion();
      Rmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
      Lmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
    }
    
    // Needs to be called continously and quickly in order for stepper motors to turn
    Hmot.runSpeed();
  }
  
  // Reset right and left motors position
  Hmot.setCurrentPosition(0);

  
}

void head9(float len)
{
  float encLen = len;
  // Convert stepper motor len (# of steps) to enc len (# of encoder ticks)
  encLen = ((encLen/microStepH));
  
  // Reset encoder position
  encH.write(0);
  
  // Set left and right motor speed
  Hmot.setSpeed(maxStepSpeedH);

  // While left encoder has yet to reach positon, continue moving
  Serial.print("encH "); 
    Serial.print(encH.read());
  while(encH.read() <= encLen)

  {
    Serial.print("encHEAD "); 
    Serial.print(encH.read());
    Serial.println(); 
    Serial.print("encLen "); 
    Serial.print(encLen);
    Serial.println(); 

    // If moveHead flag is set, stop motion and turn head, continue motion with acceleration
    if(moveHead)
    {
      headMotion();
      Rmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
      Lmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
    }
    // Needs to be called continously and quickly in order for stepper motors to turn
    Hmot.runSpeed();
  }
  
  // Reset right and left motors position
  Hmot.setCurrentPosition(0);
}

void head10(float len)
{
  float encLen = len;
  // Convert stepper motor len (# of steps) to enc len (# of encoder ticks)
  encLen = ((encLen/microStep));
  
  // Reset encoder position
  encH.write(0);
  
  // Set left and right motor speed
  Hmot.setSpeed(maxStepSpeedH);

  // While left encoder has yet to reach positon, continue moving
      Serial.print("encHEAD "); 
    Serial.print(encH.read());
      Serial.println(); 
    while(encH.read() <= encLen)
  {
    Serial.print("encH"); 
    Serial.print(encH.read());
    Serial.println(); 
    Serial.print("encLen "); 
    Serial.print(encLen);
    Serial.println();
    // If moveHead flag is set, stop motion and turn head, continue motion with acceleration
    /*if(moveHead)
    {
      headMotion();
      Rmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
      Lmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
    }
    */
    // Needs to be called continously and quickly in order for stepper motors to turn
    Hmot.runSpeed();
  }
  
  // Reset right and left motors position
  Hmot.setCurrentPosition(0);
}

void head11(float len)
{
  float encLen = len;
  // Convert stepper motor len (# of steps) to enc len (# of encoder ticks)
  encLen = ((encLen/microStep));
  
  // Reset encoder position
  encH.write(0);
  
  // Set left and right motor speed
  Hmot.setSpeed(maxStepSpeedH);

  // While left encoder has yet to reach positon, continue moving
  while(encH.read() <= encLen)

  {
    Serial.print("encHEAD"); 
    Serial.print(encH.read());
    Serial.println(); 
    Serial.print("encLen "); 
    Serial.print(encLen);
    Serial.println(); 
  }

     // If moveHead flag is set, stop motion and turn head, continue motion with acceleration
      headMotion();
      Rmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
      Lmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
    // Needs to be called continously and quickly in order for stepper motors to turn
    Hmot.runSpeed();
  
  // Reset right and left motors position
  Rmot.setCurrentPosition(0);
  Lmot.setCurrentPosition(0);
}
int convertEncToStep(int encRead)
{
  return(int((microStepH*float(encRead))/4));
}


// This function determines which sensor was triggered, returns that sensor's pin number
float whichSensor() {
//    Serial.println("*****************************************");
//    Serial.print("Sensor 0 = ");
//    Serial.print(distanceToUse(sensor0));
//    Serial.println();
//    Serial.print("Sensor 1 = ");
//    Serial.print(analogRead(sensor1));
//    Serial.println();
//    Serial.print("Sensor 2 = ");
//    Serial.print(analogRead(sensor2));
//    Serial.println();
//    Serial.print("Sensor 3 = ");
//    Serial.print(analogRead(sensor3));
//    Serial.println();
//    Serial.print("Sensor 4 = ");
//    Serial.print(analogRead(sensor4));
//    Serial.println();
//    Serial.print("Sensor 5 = ");
//    Serial.print(analogRead(sensor5));
//    Serial.println();
//    Serial.println("*********");
//    if(distanceToUse(sensor0) <= 40) {
//      Serial.print("Using sensor 0:");
//      Serial.print(distanceToUse(sensor0));
//      Serial.println();
//      return sensor0;
//    }
//    else if(distanceToUse(sensor1) <= 40) {
//      Serial.print("Using sensor 1:");
//      Serial.print(distanceToUse(sensor1));
//      Serial.println();
//      return sensor1;
//    }
//    else if(distanceToUse(sensor2) <= 40) {
//      Serial.print("Using sensor 2: ");
//      Serial.print(distanceToUse(sensor2));
//      Serial.println();
//      return sensor2;
//    }
//    else if(distanceToUse(sensor3) <= 40) {
//      Serial.print("Using sensor 3:");
//      Serial.print(distanceToUse(sensor3));
//      Serial.println();
//      return sensor3;
//    }
//    else if(distanceToUse(sensor4) <= 40) {
//      Serial.print("Using sensor 4: ");
//      Serial.print(distanceToUse(sensor4));
//      Serial.println();
//      return sensor4;
//    }
//    else if(distanceToUse(sensor5) <= 40) {
//      Serial.print("Using sensor 5:");
//      Serial.print(distanceToUse(sensor5));
//      Serial.println();
//      return sensor5;
//    }
  if(distanceToUse(sensor0) <= 40) {
    return sensor0;
  }
  else if(distanceToUse(sensor1) <= 40) {
    return sensor1;
  }
    else if(distanceToUse(sensor2) <= 40) {
    return sensor2;
  }
  else if(distanceToUse(sensor3) <= 40) {
    return sensor3;
  }
  else if(distanceToUse(sensor4) <= 40) {
    return sensor4;
  }
  else if(distanceToUse(sensor5) <= 40) {
    return sensor5;
  }
}


int distanceToUse(int sensorNum) {

  int arraySize = 5;

  int values[arraySize]; // Create array of values

// this is currently a really long delay, so we can change it to be shorter later if we need to (50!)


// populate the array
  values[0] = analogRead(sensorNum); // add first reading to array
  delay(50);
  values[1] = analogRead(sensorNum); // add second reading to array
  delay(50);
  values[2] =  analogRead(sensorNum); // add third reading to array
  delay(50);
  values[3] = analogRead(sensorNum); // add fourth reading to array
  delay(50);
  values[4] = analogRead(sensorNum); // add fifth reading to array
  delay(50);

   int maxItem = values[0]; // max is currently first element
   int maxIdx = 0; // index of max is currently 0

   for(int i = 0; i < arraySize - 1; i++) { // for each item in the array
    if(values[i] > maxItem) {  // if the current item is greater than the current max item
      maxItem = values[i]; // update the max to be the current
      maxIdx = i; // update the max index to be the current index
    }
   }

  values[maxIdx] = 0; // set that array position to zero (instead of dropping it) --> whichever is the max, set to zero

  int sum = 0;
  for(auto item : values) { // add all the items up
    sum += item;
  }

  int avg = sum / (arraySize - 1); // average them (just the non-max ones because max is now set to zero)

  return avg; // return that avg value 

}
