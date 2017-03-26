/*
  IOT Project: JSJC

  Main controller for radio node network

  3.23.2017 - init & programming controller
  
*/

// initialize STATUS_LED
int STATUS_LED = 13;

// the setup function runs on reset
void setup() {
  // set STATUS_LED as an output.
  pinMode(STATUS_LED, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(STATUS_LED, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(STATUS_LED, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
}
