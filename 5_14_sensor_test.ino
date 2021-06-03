// Function prototypes
float whichSensor();
int distanceToUse(int sensorNum);

// Global variables
int redLight = 9; // pin 9
int whiteLight = 5; // pin 5

float sensor0 = 16;
float sensor1 = 17;
float sensor2 = 18;
float sensor3 = 19;
float sensor4 = 20;
float sensor5 = 21;
/* 3/31 - changed these from int to float just to see what happens */

int maxDistance; // How far the sensor should reach/register movement
int sensorNum; // Determines which sensor is triggered

void setup() {

  Serial.begin(9600);
  
  pinMode(redLight, OUTPUT);
  pinMode(whiteLight, OUTPUT);
  pinMode(sensor0, INPUT);
  pinMode(sensor1, INPUT);
  pinMode(sensor2, INPUT);
  pinMode(sensor3, INPUT);
  pinMode(sensor4, INPUT);
  pinMode(sensor5, INPUT);
  
  // Choose the farthest distance sensor should trigger from
   maxDistance = 20; // example value - this is in inches

   digitalWrite(whiteLight, HIGH);
   digitalWrite(redLight, LOW);
}

void loop() {

  if(whichSensor()) {
    digitalWrite(whiteLight, LOW);
    digitalWrite(redLight, HIGH);
  }
  else {
    digitalWrite(whiteLight, HIGH);
    digitalWrite(redLight, LOW);
  }

  delay(100);
}
  
// This function determines which sensor was triggered, returns that sensor's pin number
float whichSensor() {

    Serial.println("*****************************************");
    Serial.print("Sensor 0 = ");
    Serial.print(distanceToUse(sensor0));
    Serial.println();
    Serial.print("Sensor 1 = ");
    Serial.print(analogRead(sensor1));
    Serial.println();
    Serial.print("Sensor 2 = ");
    Serial.print(analogRead(sensor2));
    Serial.println();
    Serial.print("Sensor 3 = ");
    Serial.print(analogRead(sensor3));
    Serial.println();
    Serial.print("Sensor 4 = ");
    Serial.print(analogRead(sensor4));
    Serial.println();
    Serial.print("Sensor 5 = ");
    Serial.print(analogRead(sensor5));
    Serial.println();
    Serial.println("*********");
    if(distanceToUse(sensor0) <= 40) {
      Serial.print("Using sensor 0:");
      Serial.print(distanceToUse(sensor0));
      Serial.println();
      return sensor0;
    }
    else if(distanceToUse(sensor1) <= 40) {
      Serial.print("Using sensor 1:");
      Serial.print(distanceToUse(sensor1));
      Serial.println();
      return sensor1;
    }
    else if(distanceToUse(sensor2) <= 40) {
      Serial.print("Using sensor 2: ");
      Serial.print(distanceToUse(sensor2));
      Serial.println();
      return sensor2;
    }
    else if(distanceToUse(sensor3) <= 40) {
      Serial.print("Using sensor 3:");
      Serial.print(distanceToUse(sensor3));
      Serial.println();
      return sensor3;
    }
    else if(distanceToUse(sensor4) <= 40) {
      Serial.print("Using sensor 4: ");
      Serial.print(distanceToUse(sensor4));
      Serial.println();
      return sensor4;
    }
    else if(distanceToUse(sensor5) <= 40) {
      Serial.print("Using sensor 5:");
      Serial.print(distanceToUse(sensor5));
      Serial.println();
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
