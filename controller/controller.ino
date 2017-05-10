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
//OS_TASK *SwitchOnMeter;
//OS_TASK *SwitchOffMeter;
OS_TASK *RFReadMeter;
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

int rf_message_type = -1;

// Define reply TIMEOUT
#define TIMEOUT_REPLY 1000

int16_t packetnum = 0;  // packet counter, we increment per transmission

/******************************** radio init end *********************************/

/********************************* memory init **********************************/

// coil status
#define METER_STATUS_POWER_OFF  0
#define METER_STATUS_POWER_ON   1

#define meter_count 3
byte meter_num[meter_count] = {0x06,0x05,0x0B};

// Structure for stored data
typedef struct {
  uint32_t voltage;
  uint32_t amp;
  uint32_t frequency;
  uint32_t watt;
  uint32_t power_factor;
  uint32_t kwh;
  bool relay_status;
  uint32_t temp;
  uint32_t warnings;
  uint8_t flag;
} METER_TYPE;

METER_TYPE meter_type[meter_count];

// stored information
void initMeterDataStruct() {
  for (int j = 0; j < meter_count; j++) {
    meter_type[j].voltage = 0;
    meter_type[j].amp = 0;
    meter_type[j].frequency = 0;
    meter_type[j].watt = 0;
    meter_type[j].power_factor = 0;
    meter_type[j].kwh = 0;
    meter_type[j].relay_status = 0;
    meter_type[j].temp = 0;
    meter_type[j].warnings = 1;
  }
}

int meter_select = 0;

/******************************* memory init end ********************************/

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

long convert_two(byte data1, byte data2) {
  byte p[2] = {0x00,0x00};
  p[0] = data2;
  p[1] = data1;

  /*Serial.print(p[0], HEX);
  Serial.print("  ");
  Serial.println(p[1], HEX);*/

  /*Serial.print(p[0], BIN);
  Serial.print("  ");
  Serial.println(p[1], BIN);*/

  long d0 = p[0] << 8;
  long d1 = p[1];
  long d = d0 | d1;

  /*Serial.println(d, BIN);
  Serial.println(d);*/

  return d;
}

long convert_four(byte data1, byte data2, byte data3, byte data4) {
  byte p[4] = {0x00,0x00,0x00,0x00};
  p[0] = data4;
  p[1] = data3;
  p[2] = data2;
  p[3] = data1;

  /*Serial.print(p[0], HEX);
  Serial.print("  ");
  Serial.print(p[1], HEX);
  Serial.print("  ");
  Serial.print(p[2], HEX);
  Serial.print("  ");
  Serial.println(p[3], HEX);*/

  /*Serial.print(p[0], BIN);
  Serial.print("  ");
  Serial.println(p[1], BIN);
  Serial.print("  ");
  Serial.print(p[2], BIN);
  Serial.print("  ");
  Serial.println(p[3], BIN);*/

  long d0 = p[0] << 24;
  long d1 = p[1] << 16;
  long d2 = p[2] << 8;
  long d3 = p[3];
  long d = d0 | d1 | d2 | d3;

  /*Serial.println(d, BIN);
  Serial.println(d);*/

  return d;
}

unsigned int radio_transmit(int rank) {

  //char MESSAGE[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  // Send a message to rf95_server
  Serial.println("Sending to rf95_server");

  if (rf_message_type == BEACON) {
    // length specified here 8B max? for rf95
    char radiopacket[16] = {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x01, meter_num[rank], 0x00, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF};
    //MESSAGE = beacon;
    //char MESSAGE[16] = {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x01, 0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF};
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
  }
  else if (rf_message_type == READ_METER) {
    // length specified here 8B max? for rf95
    char radiopacket[16] = {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x02, meter_num[rank], 0x00, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF};
    //MESSAGE = read_meter;
    //char MESSAGE[16] = {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x02, 0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF};
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
        //Serial.println(buf[7],HEX);
        //Serial.println(buf[8],HEX);
        meter_type[rank].voltage = convert_two(buf[8],buf[7]);
        meter_type[rank].amp = convert_two(buf[10],buf[9]);
        meter_type[rank].frequency = convert_two(buf[12],buf[11]);
        meter_type[rank].watt = convert_two(buf[14],buf[13]);
        meter_type[rank].power_factor = convert_two(buf[16],buf[15]);
        meter_type[rank].kwh = convert_four(buf[20],buf[19],buf[18],buf[17]);
        meter_type[rank].relay_status = convert_two(buf[22],buf[21]);
        meter_type[rank].temp = convert_two(buf[24],buf[23]);
        meter_type[rank].warnings = convert_two(buf[26],buf[25]);
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
  }
  else if (rf_message_type == SWITCH_COIL_ON) {
    // length specified here 8B max? for rf95
    char radiopacket[16] = {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x03, meter_num[rank], 0x01, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF};
    //MESSAGE = switch_on;
    //char MESSAGE[16] = {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x03, 0x06, 0x01, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF};
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
  }
  else if (rf_message_type == SWITCH_COIL_OFF) {
    // length specified here 8B max? for rf95
    char radiopacket[16] = {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x03, meter_num[rank], 0x00, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF};
    //MESSAGE = switch_off;
    //char MESSAGE[16] = {0xAA, 0xAA, 0xAA, 0x01, 0x01, 0x03, 0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0xD8, 0xFF, 0xFF, 0xFF};
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
  }
  return 1;
}

unsigned int rf_read_meter(int opt) {
  rf_message_type = 1;
  for (int i = 0; i < meter_count; i++) {
    radio_transmit(i);
    Serial.println("----------------------------------------------------------------");
    Serial.print("Meter: "); Serial.println(meter_num[i]);
    Serial.print("Voltage: "); Serial.println(meter_type[i].voltage);
    Serial.print("Current: "); Serial.println(meter_type[i].amp);
    Serial.print("Hertz: "); Serial.println(meter_type[i].frequency);
    Serial.print("Watts: "); Serial.println(meter_type[i].watt);
    Serial.print("PF: "); Serial.println(meter_type[i].power_factor);
    Serial.print("Energy: "); Serial.println(meter_type[i].kwh);
    Serial.print("Relay Status: "); Serial.println(meter_type[i].relay_status);
    Serial.print("Temp: "); Serial.println(meter_type[i].temp);
    Serial.print("Warnings: "); Serial.println(meter_type[i].warnings);
    Serial.println("----------------------------------------------------------------");
  }
  return 1;
}

unsigned int switch_on_meter(int opt) {
  rf_message_type = 2;
  radio_transmit(meter_select);
  return 1;
}

unsigned int switch_off_meter(int opt) {
  rf_message_type = 3;
  radio_transmit(meter_select);
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
  //SwitchOnMeter = taskRegister(switch_on_meter, OS_ST_PER_SECOND*120, 1, 0);
  //SwitchOffMeter = taskRegister(switch_off_meter, OS_ST_PER_SECOND*240, 1, 0);
  RFReadMeter = taskRegister(rf_read_meter, OS_ST_PER_SECOND*20, 1, 0);

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
