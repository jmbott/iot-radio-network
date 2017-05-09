/*
  IOT Project: JSJC

  Main controller for radio node network

  03.23.2017 - init & programming controller
  03.26.2017 - basic transmission code added, RH_RF95 class
  05.09.2017 - Add lightOS

*/

/********************************* lightOS init **********************************/

#include "lightOS.h"
#include "Timer.h"

// Timer name
Timer t;

#define OS_TASK_LIST_LENGTH 4

//****task variables****//
OS_TASK *Echo;
OS_TASK *SwitchOff;
OS_TASK *SwitchOn;
//OS_TASK *ListenRadio;
OS_TASK *WebServer;

// system time
unsigned long systime = 0;
unsigned long sys_start_time = 0;

#define MAX_RETRY    6
#define NEXT_COMMAND_INTERVAL     OS_ST_PER_100_MS

/******************************* lightOS init end ********************************/

/********************************** radio init ***********************************/

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

//#define MESSAGE     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

//                  Start x3,Length,Version,Function,MeterID,Status,ID,ID,ID,ID,Check,Check,end x3
//                  (Length only includes non standard bits.)
#define BEACON            0
#define READ_METER        1
#define SWITCH_COIL_ON    2
#define SWITCH_COIL_OFF   3

/*
#define beacon {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x01, 0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF}
#define read_meter {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x02, 0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF}
#define switch_on {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x03, 0x06, 0x01, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF}
#define switch_off {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x03, 0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF}
*/

// Define reply TIMEOUT
#define TIMEOUT_REPLY 1000

int16_t packetnum = 0;  // packet counter, we increment per transmission

/******************************** radio init end *********************************/

/******************************** ethernet init *********************************/

#include <SPI.h>
#include <Ethernet2.h>

#define WIZ_CS       10
#define REQ_BUF_SZ   50

// Ethernet module MAC address
byte mac[] = { 0x98, 0x76, 0xB6, 0x10, 0x56, 0xED };

// Add More

/****************************** ethernet init end *******************************/

/********************************* WAN Time Sync *********************************/

// To Do

/******************************* WAN Time Sync end *******************************/

/********************************** echo task ***********************************/

unsigned int echo(int opt) {
  if (rf95.available())
  {
    // Receive incomming message
    // uint8_t is a type of unsigned integer of length 8 bits
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    // if received signal is something
    if (rf95.recv(buf, &len))
    {
      // turn led on to indicate received signal
      digitalWrite(LED, HIGH);
      RH_RF95::printBuffer("Received: ", buf, len);
      // print in the serial monitor what was recived
      Serial.print("Got: ");
      Serial.println((char*)buf);
      // and the signal strength
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
      delay(10);
      // Send a reply
      //uint8_t data[] = REPLY; // predestined response
      //rf95.send(data, sizeof(data)); // predestined response
      rf95.send(buf, sizeof(buf)); // echo back
      rf95.waitPacketSent();
      // print in serial monitor the action
      Serial.println("Sent a reply");
      digitalWrite(LED, LOW);
    }
    // if received signal is nothing
    else
    {
      Serial.println("Receive failed");
    }
  }
  return 1;
}

/******************************** echo task end *********************************/

/******************************** Send RF tasks *********************************/

unsigned int radio_transmit(int option) {

  byte MESSAGE[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  if (option == BEACON) {
    //MESSAGE = beacon;
    byte MESSAGE[16] = {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x01, 0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF};
  }
  else if (option == READ_METER) {
    //MESSAGE = read_meter;
    byte MESSAGE[16] = {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x02, 0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF};
  }
  else if (option == SWITCH_COIL_ON) {
    //MESSAGE = switch_on;
    byte MESSAGE[16] = {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x03, 0x06, 0x01, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF};
  }
  else if (option == SWITCH_COIL_OFF) {
    //MESSAGE = switch_off;
    byte MESSAGE[16] = {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x03, 0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF};
  }

  // Send a message to rf95_server
  Serial.println("Sending to rf95_server");

  // length specified here 8B max? for rf95
  char radiopacket[16] = MESSAGE;
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
  if (rf95.waitAvailableTimeout(TIMEOUT_REPLY)) {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len)) {
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
    else {
      Serial.println("Receive failed");
    }
  }
  // if no reply
  else {
    Serial.println("No reply, is there a listener around?");
  }
  return 1;
}

/****************************** Send RF tasks end *******************************/

void setup() {

  // radio setup ****************************************************************/
  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // Set baud rate
  Serial.begin(115200);
  delay(100);

  Serial.println("System initializing ... ");

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

  // Web setup ********************************************************************/



  // lightOS setup ****************************************************************/
  osSetup();
  Serial.println("OS Setup OK !");

  /* initialize application task
  OS_TASK *taskRegister(unsigned int (*funP)(int opt),unsigned long interval,unsigned char status,unsigned long temp_interval)
  taskRegister(FUNCTION, INTERVAL, STATUS, TEMP_INTERVAL); */
  //Echo = taskRegister(echo, OS_ST_PER_SECOND*10, 1, 0);
  SwitchOn = taskRegister(radio_transmit(2), OS_ST_PER_SECOND*10, 1, OS_ST_PER_SECOND*5);
  SwitchOff = taskRegister(radio_transmit(3), OS_ST_PER_SECOND*10, 1, 0);

  // ISR or Interrupt Service Routine for async
  t.every(5, onDutyTime);  // Calls every 5ms

  Serial.println("All Task Setup Success !");
}

// the loop function runs over and over again forever
void loop() {
  os_taskProcessing();
  constantTask();
}

void constantTask() {
  t.update();
  //echo(0);
}

// support Function onDutyTime: support function for task management
void onDutyTime() {
  static int a = 0;
  a++;
  if (a >= 200) {
    systime++;
    a = 0;
    Serial.print("Time Now is : "); Serial.println(systime);
  }
  _system_time_auto_plus();
}

extern void _lightOS_sysLogCallBack(char *data) {
  Serial.print("--LightOS--  "); Serial.print(data);
}
