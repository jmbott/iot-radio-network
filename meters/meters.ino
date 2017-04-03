/*
 [REF] https://arduino-info.wikispaces.com/SoftwareSerialRS485Example
   - Connect this unit Pins 10, 11, Gnd
   - Pin 3 used for RS485 direction control
   - To other unit Pins 11,10, Gnd  (Cross over)
   - Open Serial Monitor, type in top window.
   - Same characters sent to smart meter
*/

#include <SoftwareSerial.h>

#define RX               10  //Serial Receive pin
#define TX               11  //Serial Transmit pin

#define TXcontrol        3   //RS485 Direction control

#define RS485Transmit    HIGH
#define RS485Receive     LOW

#define Pin13LED         13

// Declare objects
SoftwareSerial RS485Serial(RX, TX); // RX, TX

// Declare Variables
int byteReceived;
int byteSend;

void setup()
{
  // Start the built-in serial port, probably to Serial Monitor
  Serial.begin(115200);
  Serial.println("SoftwareSerial meter communication");
  Serial.println("Use Serial Monitor, type in upper window, ENTER");

  pinMode(Pin13LED, OUTPUT);
  pinMode(TXcontrol, OUTPUT);

  digitalWrite(TXcontrol, RS485Receive);  // Init Transceiver

  // Start the software serial port, to another device
  RS485Serial.begin(4800);   // set the data rate

}

void loop()
{
  digitalWrite(Pin13LED, HIGH);  // Show activity
  if (Serial.available())
  {
    byteReceived = Serial.read();

    digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
    RS485Serial.write(byteReceived);          // Send byte to Remote Arduino

    digitalWrite(Pin13LED, LOW);  // Show activity
    delay(10);
    digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  }

  if (RS485Serial.available())  //Look for data from other Arduino
   {
    digitalWrite(Pin13LED, HIGH);  // Show activity
    byteReceived = RS485Serial.read();    // Read received byte
    Serial.write(byteReceived);        // Show on Serial Monitor
    delay(10);
    digitalWrite(Pin13LED, LOW);  // Show activity
   }

}
