// Monday, May 3 --> simplified code for testing sensors

int sensor0 = 16;
int sensor1 = 17;
int sensor2 = 18;
int sensor3 = 19;
int sensor4 = 20;
int sensor5 = 21;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(sensor0, INPUT);
  pinMode(sensor1, INPUT);
  pinMode(sensor2, INPUT);
  pinMode(sensor3, INPUT);
  pinMode(sensor4, INPUT);
  pinMode(sensor5, INPUT);
  
}

void loop() {
Serial.println("whichSensor");

  Serial.println("***************************");
//  Serial.print("Sensor 0 =");
//  Serial.print(distanceToUse(sensor0));
  Serial.print("Analog read for Sensor 0 = ");
  Serial.print(analogRead(sensor0));
  Serial.println();
  delay(10);
//  Serial.print("Sensor 1 =");
//  Serial.print(distanceToUse(sensor1));
  Serial.print("Analog read for Sensor 1 = ");
  Serial.print(analogRead(sensor1));
  Serial.println();
  delay(10);
//  Serial.print("Sensor 2 =");
//  Serial.print(distanceToUse(sensor2));
  Serial.print("Analog read for Sensor 2 = ");
  Serial.print(analogRead(sensor2));
  Serial.println();
  delay(10);
//  Serial.print("Sensor 3 =");
//  Serial.print(distanceToUse(sensor3));
  Serial.print("Analog read for Sensor 3 = ");
  Serial.print(analogRead(sensor3));
  Serial.println();
  delay(10);
//  Serial.print("Sensor 4 =");
//  Serial.print(distanceToUse(sensor4));
  Serial.print("Analog read for Sensor 4 = ");
  Serial.print(analogRead(sensor4));
  Serial.println();
  delay(10);
//  Serial.print("Sensor 5 =");
//  Serial.print(distanceToUse(sensor5));
  Serial.print("Analog read for Sensor 5 = ");
  Serial.print(analogRead(sensor5));
  Serial.println();
  delay(10);

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
