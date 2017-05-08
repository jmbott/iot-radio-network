
int meter_select = 0; // selected meter, init first meter
// meter_count is the number of meters
#define meter_count 3
// byte meter_num[meter_count] = {METER_NUMER_1,METER_NUMER_2,etc...};
byte meter_num[meter_count] = {0x06,0x08,0xD9};

///*
unsigned int meter_off(int opt) {
  Serial.println("Meter Task");
  //digitalWrite(LED, HIGH);  // Show activity
  //byte byteReceived = 0x05;
  Serial.print("Selected Meter: "); Serial.println(meter_num[meter_select],HEX); // Which meter
  byte byteReceived[8] = {meter_num[meter_select], 0x06, 0x00, 0x0D, 0x00, 0x00, 0x19, 0xBE};
  Serial.println("OFF");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  //digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  //Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  //digitalWrite(LED, LOW);  // Show activity
  delay(10);
  //digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}
//*/



// the setup function runs once when you press reset or power the board
void setup() {

  // Set baud rate
  Serial.begin(115200);
  delay(1000);

  Serial.println("System initializing ... ");

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
  Serial.print("meter_count = "); Serial.println(meter_count);

  for (int i = 0; i < meter_count; i++) {
    Serial.println(meter_num[i],HEX);
  }

  Serial.println("");

  meter_off(0);

  //Serial.println(meter_num[0],HEX);
  //Serial.println(meter_num[1],HEX);
  //Serial.println(meter_num[2],HEX);
  //Serial.println(meter_num[3],HEX);
  //Serial.println(meter_num[4],HEX);
}
