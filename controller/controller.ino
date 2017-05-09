/*
  IOT Project: JSJC

  Main controller for radio node network

  3.23.2017 - init & programming controller
  3.26.2017 - basic transmission code added, RH_RF95 class

*/

#include <SPI.h>
#include <RH_RF95.h>

/* for feather m0 */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 433.0

// Set the transmission power
#define POWER 23

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Blink on receipt
#define LED 13

// Define default message
//#define MESSAGE "message to node #    "
//#define MESSAGE "1234567890abcdefghijklmnopqrstuvwxyz #" // recieves up to i

#define MESSAGE     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

//                  Start x3,Length,Version,Function,MeterID,Status,ID,ID,ID,ID,Check,Check,end x3
//                  (Length only includes non standard bits.)
#define BEACON      {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x01, 0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF}
#define READ_METER  {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x02, 0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF}
#define SWITCH_COIL_ON {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x03, 0x06, 0x01, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF}
#define SWITCH_COIL_OFF {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x03, 0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF}

// Define reply TIMEOUT
#define TIMEOUT 1000

/************************************************ WAN Time Sync *********************************************/



/************************************************ WAN Time Sync *********************************************/

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);
  delay(100);

  Serial.println("Feather LoRa TX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(POWER, false);
}

int16_t packetnum = 0;  // packet counter, we increment per xmission

void loop()
{
  Serial.println("Sending to rf95_server");
  // Send a message to rf95_server

  // length specified here 8B max? for rf95
  char radiopacket[16] = SWITCH_COIL_ON;
  // number position specify, must be within packetlength and can't be spaced with none
  itoa(packetnum++, radiopacket+16, 16);
  Serial.print("Sending "); Serial.println(radiopacket);
  radiopacket[16] = 0;

  Serial.println("Sending..."); delay(10);
  // actual max sent length specified
  rf95.send((uint8_t *)radiopacket, 16);

  Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  Serial.println("Waiting for reply..."); delay(10);
  // if recieved signal is something
  if (rf95.waitAvailableTimeout(TIMEOUT))
  {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len))
   {
      // turn led on to indicate recieved signal
      digitalWrite(LED, HIGH);
      // print in the serial monitor what was recived
      RH_RF95::printBuffer("Received: ", buf, len);
      //Serial.print("Got reply: ");
      //Serial.println((char*)buf);
      // and the signal strength
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
      digitalWrite(LED, LOW);
    }
    // if recieved signal is nothing
    else
    {
      Serial.println("Receive failed");
    }
  }
  // if no reply
  else
  {
    Serial.println("No reply, is there a listener around?");
  }
  delay(5000);
}
