/*
 [REF] https://arduino-info.wikispaces.com/SoftwareSerialRS485Example
   - Connect this unit Pins 10, 11, Gnd
   - Pin 3 used for RS485 direction control
   - To other unit Pins 11,10, Gnd  (Cross over)
   - Open Serial Monitor, type in top window.
   - Same characters sent to smart meter
 [REF] https://learn.adafruit.com/using-atsamd21-sercom-to-add-more-spi-i2c-serial-ports/creating-a-new-serial
   - Hardware Serial Documentation

   Baudrate: 9600
   Parity: None
   DataBits: 8
   StopBits: 1

   Read Voltage from meter 006
   Call: 06 03 00 08 00 01 04 7F
   Resp: 06 03 02 30 CC 19 D1
   Note: 124.92V

   Read Current from meter 006
   Call: 06 03 00 09 00 01 55 BF
   Resp: 06 03 02 00 00 0D 84
   Note: 0.000A

   Read Frequency from meter 006
   Call: 06 03 00 17 00 01 35 B9
   Resp: 06 03 02 17 6B 43 9B
   Note: 59.95Hz

   Read Power from meter 006
   Call: 06 03 00 18 00 01 05 BA
   Resp: 06 03 02 00 00 0D 84
   Note: 0.000kW

   Read Power Factor from meter 006
   Call: 06 03 00 0F 00 01 B5 BE
   Resp: 06 03 02 00 00 0D 84
   Note: 0.00

   Read Total Energy from meter 006
   Call: 06 03 00 11 00 02 95 B9
   Resp: 06 03 04 00 00 00 00 8C F3
   Note: 0.0000kWh

   Read Relay Status from meter 006
   Call: 06 03 00 0D 00 01 14 7E
   Resp: 06 03 02 00 01 CC 44
   Note: ON

   Read Temperature from meter 006
   Call: 06 03 20 14 00 01 CE 79
   Resp: 06 03 02 00 1C 0C 4D
   Note: 28C

   Read Warnings from meter 006
   Call: 06 03 00 10 00 01 84 78
   Resp: 06 03 02 00 00 0D 84
   Note: none

   Read Balance from meter 006
   Call: 06 03 20 18 00 02 4E 7B
   Resp: 06 03 04 00 00 03 E8 8C 4D
   Note: 10.00

   For different meter first byte is meter number and the last byte is different.

   Write Relay Status OFF for meter 006
   Call: 06 06 00 0D 00 00 19 BE
   Resp: 06 06 00 0D 00 00 19 BE

   Write Relay Status ON for meter 006
   Call: 06 06 00 0D 00 01 D8 7E
   Resp: 06 06 00 0D 00 01 D8 7E

*/

#include <Arduino.h>   // required before wiring_private.h
#include "wiring_private.h" // pinPeripheral() function

#define RX               10  //Serial Receive pin
#define TX               11  //Serial Transmit pin

#define TXcontrol        6   //RS485 Direction control

#define RS485Transmit    HIGH
#define RS485Receive     LOW

#define Pin13LED         13

// Declare Variables
int byteReceived;
int byteSend;

Uart Serial2 (&sercom1, RX, TX, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}

void setup()
{
  // Start the built-in serial port, probably to Serial Monitor
  Serial.begin(115200);
  Serial.println("Meter communication");
  Serial.println("Use Serial Monitor, type in upper window, ENTER");

  pinMode(Pin13LED, OUTPUT);
  pinMode(TXcontrol, OUTPUT);

  digitalWrite(TXcontrol, RS485Receive);  // Init Transceiver

  Serial2.begin(4800);

  // Assign pins 10 & 11 SERCOM functionality
  pinPeripheral(10, PIO_SERCOM);
  pinPeripheral(11, PIO_SERCOM);

}

void loop()
{
  digitalWrite(Pin13LED, HIGH);  // Show activity
  if (Serial.available())
  {
    byteReceived = Serial.read();

    digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
    Serial2.write(byteReceived);          // Send byte to Remote Arduino

    digitalWrite(Pin13LED, LOW);  // Show activity
    delay(10);
    digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  }

  if (Serial2.available())  //Look for data from other Arduino
   {
    digitalWrite(Pin13LED, HIGH);  // Show activity
    byteReceived = Serial2.read();    // Read received byte
    Serial.write(byteReceived);        // Show on Serial Monitor
    delay(10);
    digitalWrite(Pin13LED, LOW);  // Show activity
   }

}
