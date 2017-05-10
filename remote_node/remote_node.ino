/*
  IOT Project: JSJC

  Remote node in radio node network

  03.26.2017 - basic receiver code added, RH_RF95 class
  03.27.2017 - echo received message
  04.10.2017 - lightOS integration, meter skeleton
  04.29.2017 - Add Meter functions
  05.06.2017 - Add Main Meter Task
  05.07.2017 - Add Listen for Request Task
  05.08.2017 - Add Listen for Coil Set

*/

/********************************* lightOS init **********************************/

#include "lightOS.h"
#include "Timer.h"

// Timer name
Timer t;

#define OS_TASK_LIST_LENGTH 4

//****task variables****//
OS_TASK *Echo;
//OS_TASK *Meter_On;
//OS_TASK *Meter_Off;
OS_TASK *MeterMainTask;
OS_TASK *ListenRequest;

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

// Define default reply
#define REPLY "message received"

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

// byte meter_num[meter_count] = {METER_NUMER_1,METER_NUMER_2,etc...};


#define meter_count 1
// Checksums
byte meter_num[meter_count] = {0x06};
byte volt_check1[meter_count] = {0x04};
byte volt_check2[meter_count] = {0x7F};
byte amp_check1[meter_count] = {0x55};
byte amp_check2[meter_count] = {0xBF};
byte freq_check1[meter_count] = {0x35};
byte freq_check2[meter_count] = {0xB9};
byte watt_check1[meter_count] = {0x05};
byte watt_check2[meter_count] = {0xBA};
byte pf_check1[meter_count] = {0xB5};
byte pf_check2[meter_count] = {0xBE};
byte kwh_check1[meter_count] = {0x95};
byte kwh_check2[meter_count] = {0xB9};
byte rs_check1[meter_count] = {0x14};
byte rs_check2[meter_count] = {0x7E};
byte temp_check1[meter_count] = {0xCE};
byte temp_check2[meter_count] = {0x79};
byte warn_check1[meter_count] = {0x84};
byte warn_check2[meter_count] = {0x78};
byte off_check1[meter_count] = {0x19};
byte off_check2[meter_count] = {0xBE};
byte on_check1[meter_count] = {0xD8};
byte on_check2[meter_count] = {0x7E};


/*
#define meter_count 2
// Checksums
byte meter_num[meter_count] = {0x05,0x0B};
byte volt_check1[meter_count] = {0x04,0x05};
byte volt_check2[meter_count] = {0x4C,0x62};
byte amp_check1[meter_count] = {0x55,0x54};
byte amp_check2[meter_count] = {0x8C,0xA2};
byte freq_check1[meter_count] = {0x35,0x34};
byte freq_check2[meter_count] = {0x8A,0xA4};
byte watt_check1[meter_count] = {0x05,0x04};
byte watt_check2[meter_count] = {0x89,0xA7};
byte pf_check1[meter_count] = {0xB5,0xB4};
byte pf_check2[meter_count] = {0x8D,0xA3};
byte kwh_check1[meter_count] = {0x95,0x94};
byte kwh_check2[meter_count] = {0x8A,0xA4};
byte rs_check1[meter_count] = {0x14,0x15};
byte rs_check2[meter_count] = {0x4D,0x63};
byte temp_check1[meter_count] = {0xCE,0xCF};
byte temp_check2[meter_count] = {0x4A,0x64};
byte warn_check1[meter_count] = {0x84,0x85};
byte warn_check2[meter_count] = {0x4B,0x65};
byte on_check1[meter_count] = {0xD8,0xD9};
byte on_check2[meter_count] = {0x4D,0x63};
byte off_check1[meter_count] = {0x19,0x18};
byte off_check2[meter_count] = {0x8D,0xA3};
*/

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

// Serial2 setup
Uart Serial2 (&sercom1, RX, TX, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}

// First Task, (Read Voltage)
int item_rolling = 0;

// Main Meter Task
#define METER_TASK_NORMAL_SEND  0
#define METER_TASK_NORMAL_REPLY 1

// coil status
#define METER_STATUS_POWER_OFF  0
#define METER_STATUS_POWER_ON   1

// change the coil status? (initially no)
int coil_set = 0;

// initialize retries at zero
int connect_retry = 0;

// listening to how many more bytes?
int meter_receive = 0;
int meter_should_receive = 0;
int temp_receive = 0;

// received information
byte meter_receive_data[100];

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
    meter_type[j].flag = METER_STATUS_POWER_ON;
  }
}

// variable for switching task cases
int meter_task_step = 0;

/******************************** meter init end *********************************/

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

/***************************** listen request task ******************************/

// Initialize registers for read values
uint8_t V[4] = {0x00,0x00,0x00,0x00};
uint8_t C[4] = {0x00,0x00,0x00,0x00};
uint8_t H[4] = {0x00,0x00,0x00,0x00};
uint8_t W[4] = {0x00,0x00,0x00,0x00};
uint8_t PF[4] = {0x00,0x00,0x00,0x00};
uint8_t K[4] = {0x00,0x00,0x00,0x00};
uint8_t R[4] = {0x00,0x00,0x00,0x00};
uint8_t T[4] = {0x00,0x00,0x00,0x00};
uint8_t E[4] = {0x00,0x00,0x00,0x00};
uint8_t F[4] = {0x00,0x00,0x00,0x00};

uint8_t A[4] = {0x00,0x00,0x00,0x00};

void split_four(uint32_t four) {

  //static uint8_t A[4] = {0x00,0x00,0x00,0x00};

  A[3] = four >> 24;
  A[2] = four >> 16;
  A[1] = four >>  8;
  A[0] = four;

  /*
  Serial.print(A[3], HEX);
  Serial.print("  ");
  Serial.print(A[2], HEX);
  Serial.print("  ");
  Serial.print(A[1], HEX);
  Serial.print("  ");
  Serial.println(A[0], HEX);
  */

  //return A;
}

unsigned int listen_request(int opt) {
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
      // print in the serial monitor what was recived
      RH_RF95::printBuffer("Received: ", buf, len);
      //for (int i = 0; i < 16; i++) {
      //  Serial.print(buf[i],HEX); Serial.print(" ");
      //}
      //Serial.println("");
      // and the signal strength
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
      delay(10);
      // Echo back Beacon
      if (buf[5] == 1) {
        for (int i = 0; i < meter_count; i++) {
          if (buf[6] == meter_num[i]) {
            // Send a reply
            rf95.send(buf, sizeof(buf)); // echo back
            rf95.waitPacketSent();
            // print in serial monitor the action
            Serial.println("Sent a reply");
            digitalWrite(LED, LOW);
          }
        }
      }
      // Reply with collected meter information
      if (buf[5] == 2) {
        for (int i = 0; i < meter_count; i++) {
          if (buf[6] == meter_num[i]) {
            Serial.print("Meter: "); Serial.println(buf[6]);
            Serial.print("Voltage: "); Serial.println(meter_type[i].voltage);
            Serial.print("Current: "); Serial.println(meter_type[i].amp);
            Serial.print("Hertz: "); Serial.println(meter_type[i].frequency);
            Serial.print("Watts: "); Serial.println(meter_type[i].watt);
            Serial.print("PF: "); Serial.println(meter_type[i].power_factor);
            Serial.print("Energy: "); Serial.println(meter_type[i].kwh);
            Serial.print("Relay Status: "); Serial.println(meter_type[i].relay_status);
            Serial.print("Temp: "); Serial.println(meter_type[i].temp);
            Serial.print("Warnings: "); Serial.println(meter_type[i].warnings);
            Serial.print("Coil Flag: "); Serial.println(meter_type[i].flag);

            // Split each into four for transmission
            split_four(meter_type[i].voltage);
            V[0] = A[0];
            V[1] = A[1];
            split_four(meter_type[i].amp);
            C[0] = A[0];
            C[1] = A[1];
            split_four(meter_type[i].frequency);
            H[0] = A[0];
            H[1] = A[1];
            split_four(meter_type[i].watt);
            W[0] = A[0];
            W[1] = A[1];
            split_four(meter_type[i].power_factor);
            PF[0] = A[0];
            PF[1] = A[1];
            split_four(meter_type[i].kwh);
            K[0] = A[0];
            K[1] = A[1];
            K[2] = A[2];
            K[3] = A[3];
            split_four(meter_type[i].relay_status);
            R[0] = A[0];
            R[1] = A[1];
            split_four(meter_type[i].temp);
            T[0] = A[0];
            T[1] = A[1];
            split_four(meter_type[i].warnings);
            E[0] = A[0];
            E[1] = A[1];
            split_four(meter_type[i].flag);
            F[0] = A[0];
            F[1] = A[1];

            /*
            Serial.println(meter_type[i].voltage,HEX);
            Serial.println(meter_type[i].amp,HEX);
            Serial.println(meter_type[i].frequency,HEX);
            Serial.println(meter_type[i].watt,HEX);
            Serial.println(meter_type[i].power_factor,HEX);
            Serial.println(meter_type[i].kwh,HEX);
            Serial.println(meter_type[i].relay_status,HEX);
            Serial.println(meter_type[i].temp,HEX);
            Serial.println(meter_type[i].warnings,HEX);
            Serial.println(meter_type[i].flag,HEX);
            */

            /*Serial.println("");
            Serial.print(V[1],HEX);Serial.print(" ");Serial.println(V[0],HEX);
            Serial.println("");
            Serial.print(H[1],HEX);Serial.print(" ");Serial.println(H[0],HEX);*/

            // Send a reply
            uint8_t data[37] = {
              0xAA, 0xAA, 0xAA,       // Start Bytes
              0x16, 0x01, 0x02,       // Length, Version, Function Echo
              meter_num[i],
              V[1], V[0],
              C[1], C[0],
              H[1], H[0],
              W[1], W[0],
              PF[1], PF[0],
              K[3], K[2], K[1], K[0],
              R[1], R[0],
              T[1], T[0],
              E[1], E[0],
              F[1], F[0],
              0x01, 0x02, 0x03, 0x04, // Transmission ID, needs random
              0xD8,                   // Checksum, needs to be calculated
              0xFF, 0xFF, 0xFF        // End Bytes
            };

            //uint8_t data[] = REPLY; // predestined response
            rf95.send(data, sizeof(data)); // send response
            //rf95.send(buf, sizeof(buf)); // echo back
            rf95.waitPacketSent();
            // print in serial monitor the action
            Serial.println("Sent a reply");

          }
        }
      }
      // Switch Coil
      if (buf[5] == 3) {
        for (int i = 0; i < meter_count; i++) {
          if (buf[6] == meter_num[i]) {
            coil_set = 1;
            Serial.print("Coil Set: "); Serial.println(coil_set);
            if (buf[7] == 1) {
              Serial.println("Power On Flag Set");
              meter_type[i].flag = METER_STATUS_POWER_ON;
            }
            else if (buf[7] == 0) {
              Serial.println("Power Off Flag Set");
              meter_type[i].flag = METER_STATUS_POWER_OFF;
            }
            // Send a reply
            uint8_t data[16] = {
              0xAA, 0xAA, 0xAA,       // Start Bytes
              0x01, 0x01, 0x03,       // Length, Version, Function Echo
              meter_num[i],           // Meter Number
              buf[7],                 // echo back status to confirm
              0x01, 0x02, 0x03, 0x04, // Transmission ID, needs random
              0xD8,                   // Checksum, needs to be calculated
              0xFF, 0xFF, 0xFF        // End Bytes
            };
            rf95.send(data, sizeof(data)); // send response
            rf95.waitPacketSent();
            // print in serial monitor the action
            Serial.println("Sent a reply");
            digitalWrite(LED, LOW);
            //selfNextDutyDelay(OS_ST_PER_100_MS*3);
          }
        }
      }

      //Serial.println("Got: ");
      //Serial.println((char*)buf);

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

/*************************** listen request task end ****************************/

/********************************* meter tasks **********************************/

/*
Baudrate: 9600
Parity: None
DataBits: 8
StopBits: 1

Read Voltage from meter 006
Call: 06 03 00 08 00 01 04 7F
Resp: 06 03 02 30 CC 19 D1
Note: 124.92V

Call: Meter #, Read/Set, Register High, Register Low, ... , ... , CheckSum, CheckSum
Resp: Meter #, Read/Set, Length, Response 1, Response 2, etc, CheckSum, CheckSum
*/

unsigned int voltage(int rank) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.println(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x00, 0x08, 0x00, 0x01, volt_check1[rank], volt_check2[rank]};
  Serial.println("Voltage");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  meter_should_receive = 7;

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int current(int rank) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.println(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x00, 0x09, 0x00, 0x01, amp_check1[rank], amp_check2[rank]};
  Serial.println("Current");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  meter_should_receive = 7;

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int frequency(int rank) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.println(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x00, 0x17, 0x00, 0x01, freq_check1[rank], freq_check2[rank]};
  Serial.println("Frequency");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  meter_should_receive = 7;

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int power(int rank) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.println(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x00, 0x18, 0x00, 0x01, watt_check1[rank], watt_check2[rank]};
  Serial.println("Power");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  meter_should_receive = 7;

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int power_factor(int rank) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.println(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x00, 0x0F, 0x00, 0x01, pf_check1[rank], pf_check2[rank]};
  Serial.println("Power Factor");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  meter_should_receive = 7;

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int energy(int rank) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.println(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x00, 0x11, 0x00, 0x02, kwh_check1[rank], kwh_check2[rank]};
  Serial.println("Total Energy");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  meter_should_receive = 9;

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int relay_status(int rank) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.println(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x00, 0x0D, 0x00, 0x01, rs_check1[rank], rs_check2[rank]};
  Serial.println("Relay Status");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  meter_should_receive = 7;

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int temp(int rank) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.println(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x20, 0x14, 0x00, 0x01, temp_check1[rank], temp_check2[rank]};
  Serial.println("Temperature");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  meter_should_receive = 7;

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int warnings(int rank) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.println(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x00, 0x10, 0x00, 0x01, warn_check1[rank], warn_check2[rank]};
  Serial.println("Warnings");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  meter_should_receive = 7;

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int meter_off(int rank) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: ");Serial.println(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x06, 0x00, 0x0D, 0x00, 0x00, off_check1[rank], off_check2[rank]};
  Serial.println("OFF");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  meter_should_receive = 8;

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int meter_on(int rank) {
  Serial.println("Meter Task");
  digitalWrite(LED, HIGH);  // Show activity
  Serial.print("Selected Meter: "); Serial.println(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x06, 0x00, 0x0D, 0x00, 0x01, on_check1[rank], on_check2[rank]};
  Serial.println("ON");
  Serial.println("sending:");
  for (int i = 0; i < 8; i++) {
    Serial.print(byteReceived[i],HEX);Serial.print(" ");
  }
  Serial.println("");

  digitalWrite(TXcontrol, RS485Transmit);  // Enable RS485 Transmit
  Serial2.write(byteReceived, 8);          // Send byte to Remote Arduino

  meter_should_receive = 8;

  digitalWrite(LED, LOW);  // Show activity
  delay(10);
  digitalWrite(TXcontrol, RS485Receive);  // Disable RS485 Transmit
  return 1;
}

unsigned int meter_listen(int opt) {
  if (meter_should_receive - meter_receive > 0) {
    while (Serial2.available()) {//Look for data from meter
      digitalWrite(LED, HIGH);  // Show activity
      //int temp_receive = Serial2.available();
      if (Serial2.available()) {
        temp_receive = 1;
      }
      Serial.print("temp_receive before: "); Serial.println(temp_receive);
      if (meter_receive + temp_receive >= meter_should_receive) {
        temp_receive = meter_should_receive - meter_receive;
        taskNextDutyDelay(MeterMainTask, 0);
      }
      Serial.print("temp_receive after: "); Serial.println(temp_receive);
      byte received_data = Serial2.read();    // Read received byte
      meter_receive_data[meter_receive] = received_data;
      Serial.print("just received: ");
      Serial.println(received_data,HEX);        // Show on Serial Monitor
      Serial.print("# Should Receive: "); Serial.println(meter_should_receive);
      Serial.print("# meter_received: "); Serial.println(meter_receive);
      meter_receive += temp_receive;
      Serial.print("--MeterListening-- In Buffer: ");
      for (int i = 0; i < meter_receive; i++){
        //Serial.print("Data Received: ");
        Serial.print(meter_receive_data[i],HEX);
        Serial.print(" ");
      }
      Serial.println(" -- ");
      delay(10);
      digitalWrite(LED, LOW);  // Show activity
    }
  }
  return 1;
}

long convert_two(byte data[]) {
  byte p[2] = {0x00,0x00};
  p[0] = data[3];
  p[1] = data[4];

  Serial.print(p[0], HEX);
  Serial.print("  ");
  Serial.println(p[1], HEX);

  Serial.print(p[0], BIN);
  Serial.print("  ");
  Serial.println(p[1], BIN);

  long d0 = p[0] << 8;
  long d1 = p[1];
  long d = d0 | d1;

  Serial.println(d, BIN);
  Serial.println(d);

  return d;
}

long convert_four(byte data[]) {
  byte p[4] = {0x00,0x00,0x00,0x00};
  p[0] = data[3];
  p[1] = data[4];
  p[2] = data[5];
  p[3] = data[6];

  Serial.print(p[0], HEX);
  Serial.print("  ");
  Serial.print(p[1], HEX);
  Serial.print("  ");
  Serial.print(p[2], HEX);
  Serial.print("  ");
  Serial.println(p[3], HEX);

  Serial.print(p[0], BIN);
  Serial.print("  ");
  Serial.println(p[1], BIN);
  Serial.print("  ");
  Serial.print(p[2], BIN);
  Serial.print("  ");
  Serial.println(p[3], BIN);

  long d0 = p[0] << 24;
  long d1 = p[1] << 16;
  long d2 = p[2] << 8;
  long d3 = p[3];
  long d = d0 | d1 | d2 | d3;

  Serial.println(d, BIN);
  Serial.println(d);

  return d;
}

unsigned meter_main_task(int opt) {
  //Serial.println("Meter Main Task");
  switch (meter_task_step) {
    case METER_TASK_NORMAL_SEND : {
        Serial.println(" ------------------------------------------------------ ");
        Serial.print("--MainTask-- Send query request to meter : ");
        Serial.println(meter_select);
        int m_flag = -1;
        if (item_rolling == 0) {
          // read voltage
          Serial.println("--MainTask-- Read Voltage");
          m_flag = voltage(meter_select);
        }
        else if (item_rolling == 1) {
          // read amp
          Serial.println("--MainTask-- Read AMP");
          m_flag = current(meter_select);
        }
        else if (item_rolling == 2) {
          // read frequency
          Serial.println("--MainTask-- Read Hertz");
          m_flag = frequency(meter_select);
        }
        else if (item_rolling == 3) {
          // read watt
          Serial.println("--MainTask-- Read Watt");
          m_flag = power(meter_select);
        }
        else if (item_rolling == 4) {
          // read power factor
          Serial.println("--MainTask-- Read PF");
          m_flag = power_factor(meter_select);
        }
        else if (item_rolling == 5) {
          // read kwh
          Serial.println("--MainTask-- Read kwh");
          m_flag = energy(meter_select);
        }
        else if (item_rolling == 6) {
          // read relay status
          Serial.println("--MainTask-- Read Relay Status");
          m_flag = relay_status(meter_select);
        }
        else if (item_rolling == 7) {
          // read temperature
          Serial.println("--MainTask-- Read Temp");
          m_flag = temp(meter_select);
        }
        else if (item_rolling == 8) {
          // read warnings
          Serial.println("--MainTask-- Read Warnings");
          m_flag = warnings(meter_select);
        }
        else if (item_rolling == 9) {
          Serial.println("--MainTask-- Control Coil");
          Serial.print("Coil Set: ");Serial.println(coil_set);
          if (coil_set == 1) {
            // if coil need to be changed
            if (meter_type[meter_select].flag == METER_STATUS_POWER_ON) {
              m_flag = meter_on(meter_select);
              Serial.print("--MainTask-- Turn Coil ON, ");
              Serial.println(meter_num[meter_select]);
            }

            else if (meter_type[meter_select].flag == METER_STATUS_POWER_OFF) {
              m_flag = meter_off(meter_select);
              Serial.print("--MainTask-- Turn Coil Off, ");
              Serial.println(meter_num[meter_select]);
            }
            coil_set = 0;
          }
          else {
            // if coil does not need to be changed
            item_rolling = 0;
            if (meter_select >= meter_count - 1) {
              meter_select = 0;
            }
            else {
              meter_select++;
            }
            meter_receive = 0;
            break;
          }
        }
        meter_receive = 0;
        if (m_flag == 1) {
          // Send success
          meter_receive = 0;
          meter_task_step = METER_TASK_NORMAL_REPLY;
          selfNextDutyDelay(OS_ST_PER_100_MS*10);
          Serial.println("--MainTask-- Send Command OK !");
        }
        else {
          // Send Fail
          selfNextDutyDelay(OS_ST_PER_100_MS*10);
          Serial.print("--MainTask-- Send to meter failed, # Retries: ");
          Serial.println(connect_retry);
          connect_retry++;
          if (connect_retry > MAX_RETRY) {
            Serial.println("--MainTask-- Send to meter failed, Max Retries");
            connect_retry = 0;
            item_rolling++;
            if (item_rolling > 9) {
              item_rolling = 0;
              if (meter_select >= meter_count - 1) {
                meter_select = 0;
              }
              else {
                meter_select++;
              }
            }
            Serial.print("--MainTask-- Error check item number: ");
            Serial.println(item_rolling);
          }
        }
        break;
      }
    case METER_TASK_NORMAL_REPLY : {
        Serial.println("--MainTask-- Check meter reply");
        if (meter_receive >= meter_should_receive && meter_should_receive != 0) {
          if (item_rolling == 0) {
            // read voltage
            meter_type[meter_select].voltage = convert_two(meter_receive_data);
            item_rolling++;
            meter_task_step = METER_TASK_NORMAL_SEND;
            selfNextDutyDelay(NEXT_COMMAND_INTERVAL);
          }
          else if (item_rolling == 1) {
            // read amp
            meter_type[meter_select].amp = convert_two(meter_receive_data);
            item_rolling++;
            meter_task_step = METER_TASK_NORMAL_SEND;
            selfNextDutyDelay(NEXT_COMMAND_INTERVAL);
          }
          else if (item_rolling == 2) {
            // read frequency
            meter_type[meter_select].frequency = convert_two(meter_receive_data);
            item_rolling++;
            meter_task_step = METER_TASK_NORMAL_SEND;
            selfNextDutyDelay(NEXT_COMMAND_INTERVAL);
          }
          else if (item_rolling == 3) {
            // read watt
            meter_type[meter_select].watt = convert_two(meter_receive_data);
            item_rolling++;
            meter_task_step = METER_TASK_NORMAL_SEND;
            selfNextDutyDelay(NEXT_COMMAND_INTERVAL);
          }
          else if (item_rolling == 4) {
            // read power factor
            meter_type[meter_select].power_factor = convert_two(meter_receive_data);
            item_rolling++;
            meter_task_step = METER_TASK_NORMAL_SEND;
            selfNextDutyDelay(NEXT_COMMAND_INTERVAL);
          }
          else if (item_rolling == 5) {
            // read kwh
            meter_type[meter_select].kwh = convert_four(meter_receive_data);
            item_rolling++;
            meter_task_step = METER_TASK_NORMAL_SEND;
            selfNextDutyDelay(NEXT_COMMAND_INTERVAL);
          }
          else if (item_rolling == 6) {
            // read relay status
            meter_type[meter_select].relay_status = convert_two(meter_receive_data);
            item_rolling++;
            meter_task_step = METER_TASK_NORMAL_SEND;
            selfNextDutyDelay(NEXT_COMMAND_INTERVAL);
          }
          else if (item_rolling == 7) {
            // read temperature
            meter_type[meter_select].temp = convert_two(meter_receive_data);
            item_rolling++;
            meter_task_step = METER_TASK_NORMAL_SEND;
            selfNextDutyDelay(NEXT_COMMAND_INTERVAL);
          }
          else if (item_rolling == 8) {
            // read warnings
            meter_type[meter_select].warnings = convert_two(meter_receive_data);
            item_rolling++;
            meter_task_step = METER_TASK_NORMAL_SEND;
            selfNextDutyDelay(NEXT_COMMAND_INTERVAL);
          }
          else if (item_rolling == 9) {
            // switch relay %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          }

          Serial.print("--MainTask-- Get meter reply : ");
          // receive success
          for (int ii = 0; ii < meter_receive; ii++) {
            Serial.print(meter_receive_data[ii], HEX);
            Serial.print(" ");
          }
          Serial.println();

          connect_retry = 0;
          meter_task_step = METER_TASK_NORMAL_SEND;
        }
        else {
          // receive error
          Serial.print("--MainTask-- No reply from meter, Retries: ");
          Serial.println(connect_retry);
          connect_retry++;
          meter_task_step = METER_TASK_NORMAL_SEND;
          selfNextDutyDelay(NEXT_COMMAND_INTERVAL + OS_ST_PER_100_MS * 2);
          if (connect_retry > MAX_RETRY) {
            connect_retry = 0;
            Serial.println("--MainTask-- No reply from meter, Max Retries");
            // empty the meter status.
            item_rolling++;
            // add content to roll back to zero here if over max item_rolling
          }
        }
        break;
      }
    default: {
        break;
      }
  }
  return 1;
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
  //Echo = taskRegister(echo, OS_ST_PER_SECOND*10, 1, 0);
  //Meter_On = taskRegister(meter_on, OS_ST_PER_SECOND*20, 1, 0);
  //Meter_Off = taskRegister(meter_off, OS_ST_PER_SECOND*10, 1, 0);
  //MeterMainTask = taskRegister(meter_main_task, 1, 1, OS_ST_PER_SECOND);
  MeterMainTask = taskRegister(meter_main_task, OS_ST_PER_SECOND*5, 1, 0);

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
  listen_request(0);
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
