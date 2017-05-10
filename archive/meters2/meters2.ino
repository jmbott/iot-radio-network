
#include <Arduino.h>   // required before wiring_private.h
#include "wiring_private.h" // pinPeripheral() function

#define RX               10  //Serial Receive pin
#define TX               11  //Serial Transmit pin

#define TXcontrol        6   //RS485 Direction control

#define RS485Transmit    HIGH
#define RS485Receive     LOW

#define Pin13LED         13

// Declare Variables
//int byteReceived;
//int byteSend;

Uart Serial2 (&sercom1, RX, TX, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}

void setup()
{
  // Start the built-in serial port, probably to Serial Monitor
  Serial.begin(115200);
  delay(2000);
  Serial.println("hello");

  pinMode(Pin13LED, OUTPUT);
  pinMode(TXcontrol, OUTPUT);

  digitalWrite(TXcontrol, RS485Receive);  // Init Transceiver

  Serial2.begin(4800);

  // Assign pins 10 & 11 SERCOM functionality
  pinPeripheral(RX, PIO_SERCOM);
  pinPeripheral(TX, PIO_SERCOM);

}


void loop()   /****** LOOP: RUNS CONSTANTLY ******/
{
  //Copy input data to output
  while (Serial2.available())
  {
    byte byteSend = Serial2.read();   // Read the byte

    Serial.print("recieved:");
    Serial.print(byteSend,HEX);
    Serial.println("");

    digitalWrite(Pin13LED, HIGH);  // Show activity
    delay(10);
    digitalWrite(Pin13LED, LOW);

    digitalWrite(TX, RS485Transmit);  // Enable RS485 Transmit
    Serial2.write(byteSend); // Send the byte back
    delay(10);
    digitalWrite(TX, RS485Receive);  // Disable RS485 Transmit
//    delay(100);
  }// End If RS485SerialAvailable

}
