//https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html#af4d3818e691dad5dc518308796ccf154
#include <AccelStepper.h>
//https://github.com/PaulStoffregen/Encoder
#include <Encoder.h>

// Function declarations
void lights();
float whichSensor();
void startupAccel(float len);
void straightMove(float len);
void right(float len);
void left(float len);
void setupMotors(int);
void headMotion(int &current, int);
void moveHeadWithEnc_CW(int);
void moveHeadWithEnc_CCW(int);
int convertEncToStep(int);
int distanceToUse(int sensorNum);

// Assigns sensors to analog pins
int triggerPin = 20; 
int sensor0 = 16;
int sensor1 = 17;
int sensor2 = 18;
int sensor3 = 19;
int sensor4 = 20;
int sensor5 = 21;

int sensorArray[] = {A0, A1, A2, A3, A4, A5};

int redLight = 5;
int whiteLight = 9;

// Speed variables
float speedR = 50;
float speedL = 50;
float speedH = 10;
 
// Acceleration variables
float accelFullR = 50;
float accelFullL = 50;
float accelFullH = 40;

// Distance to turn head and initial head position
int headTurnDis = 33; // Changed this from 50 so that it can now move 1/6 of the way around (to the next sensor) instead of 1/4 of the way using 50 out of 200
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
float straight = 1000*microStep; // 16,000
float turnLeft = 1872.5*microStep;
float turnRight = 1872.5*microStep; /* IS THIS RIGHT? Is it supposed to be positive or negative? */
float lengths[] = {straight, turnRight, straight, turnRight}; // change to right
float len;
char motions[] = {'S', 'S', 'S', 'S'}; // Now we have an option for each of the four motor movement options (left, right, straight, head) /* this can be changed to SRLH for testing, RRRR would be a circle */
char motion;

// Indice variable
int ind = 0;

int currentSpeedR = 0;
int currentSpeedL = 0;

int currIdx = 0; // Start with the head currently facing forward

int maxDistance = 20;

void setup() {
  Serial.begin(9600);
  Serial.println("setup");

  // Invert left motor
  Lmot.setPinsInverted(true,false,false);

  // Define head turn steps taking into account microstep settings
  headTurnDis = headTurnDis*microStepH;

  // Setup motors, passing pulsewidth in microseconds
  setupMotors(50);

  // Start moving with acceleration, acceleration is required to overcome initial torque
  startupAccel(lengths[0]);

  // Start with white lights on, red lights off
  digitalWrite(whiteLight, HIGH);
  digitalWrite(redLight, LOW);
}

void loop() {
   Serial.println("************************");

  // Loop through motion and lengths defined above
  motion = motions[ind]; // Get the direction from the motions array
  len = lengths[ind]; // Get appropriate length from the lengths array

  // Set all encoders to zero
  encR.write(0);
  encL.write(0);
  encH.write(0);
 
  // Switch through which motion to perform
  switch (motion) {
    case 'S':
      Serial.println("straight"); // Feedback
      straightMove(len); // Actually runs the motors
      break;
    case 'R':
      Serial.println("right"); // Feedback
      right(len); // Actually runs the motors
      break;
    case 'L':
      Serial.println("left"); // Feedback
      left(len); // Actually runs the motors
      break;
    case 'H':
      Serial.println("head"); // Feedback
      moveHeadWithEnc_CW(200); // This will swivel the head all the way around - for testing
      break;
    default :
      break;
  }


  // After motion is complete, increment ind and reset ind to 0 if last command in array was performed (ind >= sizeof(motions))
  ind = ind + 1;
  if(ind >= sizeof(motions))
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
  Rmot.moveTo(len + earlyStopSteps); /* (preparing for) initial steps when first moving */
  Lmot.moveTo(len + earlyStopSteps);

  // With earlyStopSteps left to go, end motion to avoid any deaccleration
  int rdist = Rmot.distanceToGo();
  int ldist = Lmot.distanceToGo();
   while(ldist < earlyStopSteps && rdist < earlyStopSteps) { // while the remaining distane until target is more than the number of steps it takes to get up to speed 
    Serial.println("true");
    // If moveHead flag is set, stop motion and turn head, continue motion with acceleration
    moveHead(currIdx);
    // Needs to be called continously and quickly in order for stepper motors to turn
    Rmot.run();
    Lmot.run(); // take a step --> 
  // Reset right and left motors position
    Rmot.setCurrentPosition(0);
    Lmot.setCurrentPosition(0);
  }
  // Set ind to 1 to move onto next motion
  ind = 1;
}


void moveHead(int& current) {
  digitalWrite(whiteLight, LOW);
  digitalWrite(redLight, HIGH);
  int sensor_num;
  sensor_num = whichSensor();
  Rmot.moveTo(len + earlyStopSteps - (encR.read()*microStep)/4); // Changed to encR from encL
  Lmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4); // These are basically saying stop where you are, "move to your current position" = stop
  headMotion(current, sensor_num); // Moved head to that sensor_num

  while(whichSensor() == sensor_num) { // Placeholder to show that the same sensor is triggered
    whichSensor();
  }
  if(!whichSensor()) { // If immediately after, no sensor is triggered
    headMotion(current, 0); // Move the head from the current position back to the front (0)
    whichSensor();
  }
  if(whichSensor()) { // If immediately after, a different sensor is triggered
    headMotion(current, whichSensor());
  }

  current = whichSensor();
}


void straightMove(float len)
{
  float encLen = len;
  // Convert stepper motor len (# of steps) to enc len (# of encoder ticks)
  encLen = ((encLen/microStep)*4);
  
  // Reset encoder position
  encR.write(0);
  encL.write(0);
  
  // Set left and right motor speed
  Rmot.setSpeed(maxStepSpeedR);
  Lmot.setSpeed(maxStepSpeedL);

  // While left encoder has yet to reach positon, continue moving
  while(encL.read() >= -encLen) /* why is this the comparison we are doing */
  {
    if(whichSensor()) {
      Serial.println(whichSensor());
//        lights(); // changes lights from white to red
//        moveHead(currIdx);
//        lights(); // changes lights from red to white
        Rmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4); // stop moving
        Lmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4); // stop moving
    }
    // Needs to be called continously and quickly in order for stepper motors to turn
    Rmot.runSpeed();
    Lmot.runSpeed();
  }
//
//  digitalWrite(whiteLight, HIGH);
//  digitalWrite(redLight, LOW);
  
  // Reset right and left motors position
  Rmot.setCurrentPosition(0);
  Lmot.setCurrentPosition(0);
}

void right(float len)
{
  float encLen = (float)len;
  // Convert stepper motor len (# of steps) to enc len (# of encoder ticks)
  encLen = ((encLen/microStep)*4);
  
  // Reset encoder position
  encR.write(0);
  encL.write(0);

  // Set left and right motor speed
  Rmot.setSpeed(maxStepSpeedR*turnRatio/1.25);
  Lmot.setSpeed(maxStepSpeedL/1.25);

  // While left encoder has yet to reach positon, continue moving
  while(encL.read() >= -encLen)
  {
    if(whichSensor()) {
        lights(); // changes lights from white to red
        moveHead(currIdx);
        lights(); // changes lights from red to white
        moveHead(currIdx);
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
    if(whichSensor()) {
      moveHead(currIdx);
      lights(); // changes lights from white to red
      moveHead(currIdx);
      lights(); // changes lights from red to white
      Rmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
      Lmot.moveTo(len + earlyStopSteps - (encL.read()*microStep)/4);
    }
      // Needs to be called continously and quickly in order for stepper motors to turn
      Rmot.runSpeed();
      Lmot.runSpeed();
  
  // Reset right and left motors position
  Rmot.setCurrentPosition(0);
  Lmot.setCurrentPosition(0);
  }
}

// Set pulsewidth, max speed and acceleration of motors
void setupMotors(int pulsewidth)
{
  /* wait pulsewidth microseconds to get the motors going --> GUESS*/
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
void headMotion(int& currIdx, int sensor_number) // Turns the head clockwise starting at sensor # currIdx and ending at sensor # sensor_number
{// idea: whenever it goes back to zero, add 2 steps so that we don't get off re: 198 vs 200
  delay(500);

  int moveHeadIdx; // Create an integer variable to hold the sensor that we want to move to
  
  for(int i = 0; i < sizeof(sensorArray); i++) { // Iterate through all of the sensors
    if(sensorArray[i] == sensor_number) { // If the current sensor is at that position in the array
      moveHeadIdx = i; // Set curr_index to that current index ---> this loop basically translate from A0 to 0, from A1 to 1, etc.
      break;
    }
  }
  
  int totalHeadDist = currIdx - moveHeadIdx; // The total amount we want to move the head based on the desired starting and ending positions
  
  if(currIdx < moveHeadIdx) { // If it's negative, make it positive // moving from 2 to 4 -> negative value -> rotate clockwise
    if(moveHeadIdx == 0) {
      moveHeadWithEnc_CW(totalHeadDist * headTurnDis + 2);
    }
    moveHeadWithEnc_CW(totalHeadDist * headTurnDis); // This will now move the head clockwise as far as we told it to go (in this example, to sensor 3)
    
  }
  else if(currIdx > moveHeadIdx) { // If it's positive, report back --> makeshift absolute value statement (starting on line 394) -> moving from 4 to 2 -> positive value -> rotate counter clockwise
    if(moveHeadIdx == 0) {
      moveHeadWithEnc_CCW(totalHeadDist * headTurnDis + 2);
    }
    moveHeadWithEnc_CCW(totalHeadDist * headTurnDis);
  }

  currIdx = moveHeadIdx; // Set the current index to be the index that it just moved to
}

// Uses .move for startup acceleration and sets speed continously as head movement can be loose and problematic
void moveHeadWithEnc_CW(int steps)
{
  float speedMotor = 0;

  // Reset encoder, and motor position
  encH.write(0);
  headPosition = 0;
  Hmot.setCurrentPosition(0);

  // Start moving
  Hmot.move(steps);

  // Move until desired head position is passed
  while((steps - headPosition) > 0)
  {
    // Needs to be called continously in order to run
    Hmot.run();
    
    // Read head position and convert to steps
    headPosition = convertEncToStep(encH.read());
    
    // Capture current speed of motor
    speedMotor = Hmot.speed();

    // Move command again
    Hmot.move(steps - headPosition);

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

int convertEncToStep(int encRead)
{
  return(int((microStepH*float(encRead))/4));
}

void lights() { // This function changes the light color. If white is on, changes to red, if red is on, changes to white
  if(digitalRead(whiteLight)) {
    digitalWrite(whiteLight, LOW);
    digitalWrite(redLight, HIGH);
  }
  else {
    digitalWrite(redLight, LOW);
    digitalWrite(whiteLight, HIGH);
  }
}

float whichSensor() {
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
  /*
  if(distanceToUse(sensor0) <= maxDistance && analogRead(sensor0) > 0) {
    if(distanceToUse(sensor0) <= 20) {
      Serial.print("Sensor 0 = ");
      Serial.print(distanceToUse(sensor0));
    }
    Serial.println();
    delay(500);
    return sensor0;
  }
  else if(distanceToUse(sensor1) <= maxDistance && analogRead(sensor1) > 0) {
        if(distanceToUse(sensor1) <= 20) {
          Serial.print("Sensor 1 = ");
          Serial.print(distanceToUse(sensor1));
        }
    Serial.println();
    delay(500);
    return sensor1;
  }
  else if(distanceToUse(sensor2) <= maxDistance && analogRead(sensor2) > 0) {
        if(distanceToUse(sensor2) <= 20) {

    Serial.print("Sensor 2 = ");
    Serial.print(distanceToUse(sensor2));
        }
    Serial.println();
    delay(500);
   return sensor2;
  }
  else if(distanceToUse(sensor3) <= maxDistance && analogRead(sensor3) > 0) {
       if(distanceToUse(sensor3) <= 20) {
          Serial.print("Sensor 3 = ");
          Serial.print(distanceToUse(sensor3));
       }
    Serial.println();
    delay(500);
    return sensor3;
  }
  else if(distanceToUse(sensor4) <= maxDistance && analogRead(sensor4) > 0) {
       if(distanceToUse(sensor4) <= 20) {
          Serial.print("Sensor 4 = ");
          Serial.print(distanceToUse(sensor4));
       }
    Serial.println();
    delay(500);
    return sensor4;
  }
  else if(distanceToUse(sensor5) <= maxDistance && analogRead(sensor5) > 0) {
       if(distanceToUse(sensor5) <= 20) {
          Serial.print("Sensor 5 = ");
          Serial.print(distanceToUse(sensor5));
       }
    Serial.println();
    delay(500); 
    return sensor5;
  }
*/
  return 0;
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
