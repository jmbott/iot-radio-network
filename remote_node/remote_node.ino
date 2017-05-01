/*
  IOT Project: JSJC

  Remote node in radio node network

  3.26.2017 - basic reciever code added, RH_RF95 class
  3.27.2017 - echo recieved message
  4.10.2017 - lightOS integration, meter skeleton
  4.29.2017 - Add Meter functions

*/

/********************************* lightOS init **********************************/

#include "lightOS.h"
#include "Timer.h"

// Timer name
Timer t;

#define OS_TASK_LIST_LENGTH 4

//****task variables****//
OS_TASK *ECHO;
OS_TASK *METER_ON;
OS_TASK *METER_OFF;

// system time
unsigned long systime = 0;
unsigned long sys_start_time = 0;

/******************************* lightOS init end ********************************/
/********************************* memory init **********************************/

struct TARIFF {
  uint32_t price1_start_time;
  uint32_t price2_start_time;
  uint16_t price1;
  uint16_t price2;
  uint32_t generate_time;
  uint32_t init_time;
} price_tariff,new_price_tariff;

#define SYSTEM_INIT_ADDR              0   //1
#define METER_NUMBER_ADDR             SYSTEM_INIT_ADDR+1                   //1
#define STATIC_IP_ADDR                METER_NUMBER_ADDR+1                  //4
#define TARIFF_ADDR                   STATIC_IP_ADDR+4                     //24
#define NEW_TARIFF_FLAG_ADDR          TARIFF_ADDR+sizeof(price_tariff)     //1
#define NEW_TARIFF_ADDR               NEW_TARIFF_FLAG_ADDR+1               //24
#define TIME_RECORD_LIST_ADDR         NEW_TARIFF_ADDR+sizeof(price_tariff) //16
#define METER_RECORD_LIST_ADDR        (TIME_RECORD_LIST_ADDR+sizeof(uint32_t)*FLASH_MAX_RECORD+3)&0xfffffffc
#define METER_RECORD_LIST_ADDR_END    METER_RECORD_LIST_ADDR+sizeof(Meter_Box_Type)*FLASH_MAX_RECORD
#define PAYMENT_RECORD_ADDR           METER_RECORD_LIST_ADDR_END+1

void cleanFlashRecord() {
  uint32_t i;
  Serial.println("-- Flash -- Clean config file.");
  for (i = SYSTEM_INIT_ADDR; i < PAYMENT_RECORD_ADDR; i++) {
    Serial.print("Clean Flash: "); Serial.println(i);
    storageWriteByte(i, 0x00);
  }
  Serial.println("-- Flash -- Clean config file success.");
}

/********************************* memory end **********************************/
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

// Define default reply
#define REPLY "message recieved"

/******************************** radio init end *********************************/

/********************************** meter init ***********************************/

#include <Arduino.h>   // required before wiring_private.h
#include "wiring_private.h" // pinPeripheral() function

#define RX 10  //Serial Receive pin
#define TX 11  //Serial Transmit pin

#define TXcontrol 6   //RS485 Direction control

#define RS485Transmit HIGH
#define RS485Receive LOW

int meter_select = 0; // selected meter, init first meter
// meter_count is the number of meters
#define meter_count 2
// byte meter_num[meter_count] = {METER_NUMER_1,METER_NUMER_2,etc...};
byte meter_num[meter_count] = {0x06,0x08};

// Serial2 setup
Uart Serial2 (&sercom1, RX, TX, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}

// Meter Statuses
#define BOX_TASK_IDLE           0
#define BOX_TASK_CONNECTING     1
#define BOX_TASK_READING        2

/******************************** meter init end *********************************/

/********************************** echo task ***********************************/

unsigned int echo(int opt) {
  if (rf95.available())
  {
    // Recieve incomming message
    // uint8_t is a type of unsigned integer of length 8 bits
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    // if recieved signal is something
    if (rf95.recv(buf, &len))
    {
      // turn led on to indicate recieved signal
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
    // if recieved signal is nothing
    else
    {
      Serial.println("Receive failed");
    }
  }
  return 1;
}

/******************************** echo task  end *********************************/

/********************************* meter tasks **********************************/

/*
Baudrate: 9600
Parity: None
DataBits: 8
StopBits: 1
*/

unsigned int voltage(int opt) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.print(meter_num[meter_select],HEX); // Which meter
  byte byteReceived[8] = {meter_num[meter_select], 0x03, 0x00, 0x08, 0x00, 0x01, 0x04, 0x7F};
  Serial.println("Voltage");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int current(int opt) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.print(meter_num[meter_select],HEX); // Which meter
  byte byteReceived[8] = {meter_num[meter_select], 0x03, 0x00, 0x09, 0x00, 0x01, 0x55, 0xBF};
  Serial.println("Current");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int frequency(int opt) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.print(meter_num[meter_select],HEX); // Which meter
  byte byteReceived[8] = {meter_num[meter_select], 0x03, 0x00, 0x17, 0x00, 0x01, 0x35, 0xB9};
  Serial.println("Frequency");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int power(int opt) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.print(meter_num[meter_select],HEX); // Which meter
  byte byteReceived[8] = {meter_num[meter_select], 0x03, 0x00, 0x18, 0x00, 0x01, 0x05, 0xBA};
  Serial.println("Power");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int power_factor(int opt) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.print(meter_num[meter_select],HEX); // Which meter
  byte byteReceived[8] = {meter_num[meter_select], 0x03, 0x00, 0x0F, 0x00, 0x01, 0xB5, 0xBE};
  Serial.println("Power Factor");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int energy(int opt) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.print(meter_num[meter_select],HEX); // Which meter
  byte byteReceived[8] = {meter_num[meter_select], 0x03, 0x00, 0x11, 0x00, 0x02, 0x95, 0xB9};
  Serial.println("Total Energy");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int relay_status(int opt) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.print(meter_num[meter_select],HEX); // Which meter
  byte byteReceived[8] = {meter_num[meter_select], 0x03, 0x00, 0x0D, 0x00, 0x01, 0x14, 0x7E};
  Serial.println("Relay Status");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int temp(int opt) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.print(meter_num[meter_select],HEX); // Which meter
  byte byteReceived[8] = {meter_num[meter_select], 0x03, 0x20, 0x14, 0x00, 0x01, 0xCE, 0x79};
  Serial.println("Temperature");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int warnings(int opt) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.print(meter_num[meter_select],HEX); // Which meter
  byte byteReceived[8] = {meter_num[meter_select], 0x03, 0x00, 0x10, 0x00, 0x01, 0x84, 0x78};
  Serial.println("Warnings");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int meter_off(int opt) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: ");Serial.print(meter_num[meter_select],HEX); // Which meter
  byte byteReceived[8] = {meter_num[meter_select], 0x06, 0x00, 0x0D, 0x00, 0x00, 0x19, 0xBE};
  Serial.println("OFF");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int meter_on(int opt) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.print(meter_num[meter_select],HEX); // Which meter
  byte byteReceived[8] = {meter_num[meter_select], 0x06, 0x00, 0x0D, 0x00, 0x01, 0xD8, 0x7E};
  Serial.println("ON");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int meter_listen(int opt) {
  while (Serial2.available()) {//Look for data from other Arduino
    digitalWrite(LED, HIGH);  // Show activity
    byte Received = Serial2.read();    // Read received byte
    Serial.println("recieved:");
    Serial.print(Received,HEX);        // Show on Serial Monitor
    Serial.println("");
    delay(10);
    digitalWrite(LED, LOW);  // Show activity
  }
  return 1;
}

unsigned int meter_read(int opt, meter_select) {
  voltage(0);
  current(0);
  frequency(0);
  power(0);
  power_factor(0);
  energy(0);
  relay_status(0);
  temp(0);
  warnings(0);
}
/******************************** meter tasks end *********************************/

// the setup function runs on reset
void setup() {

  // radio setup ****************************************************************/
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // Set baud rate
  Serial.begin(115200);
  delay(1000);

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
  // Using RFM95/96/97/98 modules which use the PA_BOOST transmitter pin, you
  // can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(POWER, false);

  // meter setup ****************************************************************/
  Serial.println("Meter communication setup");

  pinMode(TXcontrol, OUTPUT);

  digitalWrite(TXcontrol, RS485Receive);  // Init Transceiver

  Serial2.begin(9600);

  // Assign pins 10 & 11 SERCOM functionality
  pinPeripheral(RX, PIO_SERCOM);
  pinPeripheral(TX, PIO_SERCOM);

  // lightOS setup ****************************************************************/
  osSetup();
  Serial.println("OS Setup OK !");

  /* initialize application task
  OS_TASK *taskRegister(unsigned int (*funP)(int opt),unsigned long interval,unsigned char status,unsigned long temp_interval)
  taskRegister(FUNCTION, INTERVAL, STATUS, TEMP_INTERVAL); */
  //ECHO = taskRegister(echo, OS_ST_PER_SECOND*10, 1, 0);
  METER_ON = taskRegister(meter_on, OS_ST_PER_SECOND*20, 1, 0);
  METER_OFF = taskRegister(meter_off, OS_ST_PER_SECOND*10, 1, 0);

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
  echo(0);
  meter_listen(0);
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
