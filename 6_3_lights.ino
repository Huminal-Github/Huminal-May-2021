// Test code 6/3/21 - LEDs only

 // Define global variables
 int redLight = 5; // pin number of red light
 int whiteLight = 9; // pin number of white light - used PWM pin to enable fading (breathing)


void setup() {
  // set pins to output
  pinMode(redLight, OUTPUT);
  pinMode(whiteLight, OUTPUT);
}


void loop() {
  digitalWrite(whiteLight, HIGH);
  digitalWrite(redLight, LOW);
  
  delay(5000);

  digitalWrite(whiteLight, LOW);
  digialWrite(redLight, HIGH);

  delay(5000);
 
}
