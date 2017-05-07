/*
  IOT Project: JSJC

  Remote node in radio node network

  03.26.2017 - basic reciever code added, RH_RF95 class
  03.27.2017 - echo recieved message
  04.10.2017 - lightOS integration, meter skeleton
  04.29.2017 - Add Meter functions
  05.06.2017 - Add Main Meter Function

*/

/********************************* lightOS init **********************************/

#include "lightOS.h"
#include "Timer.h"

// Timer name
Timer t;

#define OS_TASK_LIST_LENGTH 4

//****task variables****//
OS_TASK *Echo;
OS_TASK *Meter_On;
OS_TASK *Meter_Off;
OS_TASK *Meter_Main_Task;

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

// Meter Statuses
#define BOX_TASK_IDLE           0
#define BOX_TASK_CONNECTING     1
#define BOX_TASK_READING        2

// First Task, (Read Voltage)
uint8_t item_rolling = 0;

// Main Meter Task
#define METER_TASK_IDEL         0
#define METER_TASK_NORMAL_SEND  3
#define METER_TASK_NORMAL_REPLY 4
#define METER_TASK_EXC_COMMAND  5
#define METER_TASK_EXC_REPLY    6
#define METER_TASK_SET_COIL     7

// coil status
#define METER_STATUS_POWER_OFF  0
#define METER_STATUS_POWER_ON   1

// change the coil status? (initially no)
uint8_t coil_set = 0;

// initialize retries at zero
uint8_t connect_retry = 0;

// listening to how many more bytes?
uint8_t meter_receive = 0;
uint8_t meter_should_receive = 0;
uint8_t temp_receive = 0;

// recieved information
uint8_t meter_receive_data[100];

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
uint8_t meter_task_step = 0;

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
  Serial.print("Selected Meter: "); Serial.print(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x00, 0x08, 0x00, 0x01, 0x04, 0x7F};
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
  Serial.print("Selected Meter: "); Serial.print(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x00, 0x09, 0x00, 0x01, 0x55, 0xBF};
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
  Serial.print("Selected Meter: "); Serial.print(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x00, 0x17, 0x00, 0x01, 0x35, 0xB9};
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
  Serial.print("Selected Meter: "); Serial.print(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x00, 0x18, 0x00, 0x01, 0x05, 0xBA};
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
  Serial.print("Selected Meter: "); Serial.print(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x00, 0x0F, 0x00, 0x01, 0xB5, 0xBE};
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
  Serial.print("Selected Meter: "); Serial.print(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x00, 0x11, 0x00, 0x02, 0x95, 0xB9};
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
  Serial.print("Selected Meter: "); Serial.print(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x00, 0x0D, 0x00, 0x01, 0x14, 0x7E};
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
  Serial.print("Selected Meter: "); Serial.print(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x20, 0x14, 0x00, 0x01, 0xCE, 0x79};
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
  Serial.print("Selected Meter: "); Serial.print(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x03, 0x00, 0x10, 0x00, 0x01, 0x84, 0x78};
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
  Serial.print("Selected Meter: ");Serial.print(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x06, 0x00, 0x0D, 0x00, 0x00, 0x19, 0xBE};
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
  Serial.print("Selected Meter: "); Serial.print(meter_num[rank],HEX); // Which meter
  byte byteReceived[8] = {meter_num[rank], 0x06, 0x00, 0x0D, 0x00, 0x01, 0xD8, 0x7E};
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
      int temp_recieve = Serial2.available();
      if (meter_receive + temp_receive >= meter_should_receive) {
        temp_receive = meter_should_receive - meter_receive;
        taskNextDutyDelay(Meter_Main_Task, 0);
      }
      byte received_data = Serial2.read();    // Read received byte
      meter_receive_data[meter_receive] = received_data;
      Serial.println("recieved:");
      Serial.print(received_data,HEX);        // Show on Serial Monitor
      Serial.println("");
      Serial.print("--MeterListening-- In Buffer: ");
      for (int i = 0; i < meter_receive; i++){
        Serial.print(meter_receive_data[i],HEX);
        Serial.print(" ");
      }
      Serial.println(" -- ");
      meter_receive += temp_receive;
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

unsigned meterMainTask(int opt) {
  switch (meter_task_step) {
    case METER_TASK_IDEL : {
        break;
      }
    case METER_TASK_NORMAL_SEND : {
        Serial.println(" ------------------------------------------------------ ");
        Serial.print("--MainTask-- Send query request to meter : ");
        Serial.print(meter_select);
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
          }
          else {
            // if coil does not need to be changed
            item_rolling = 0;
            // meter_select ++;
            // Add content to enable move to next meter if exists **************
            meter_receive = 0;
            break;
          }
        }
        meter_receive = 0;
        if (m_flag == 1) {
          // Send success
          meter_task_step = METER_TASK_NORMAL_REPLY;
          selfNextDutyDelay(OS_ST_PER_100_MS*10);
          Serial.println("--MainTask-- Send Command OK !");
        }
        else {
          // Send Fail
          connect_retry++;
          selfNextDutyDelay(OS_ST_PER_100_MS*5);
          Serial.print("--MainTask-- Send to meter failed, # Retries: ");
          Serial.println(connect_retry);
          if (connect_retry > MAX_RETRY) {
            Serial.println("--MainTask-- Send to meter failed, Max Retries");
            connect_retry = 0;

            item_rolling++;
            if (item_rolling > 9) {
              item_rolling = 0;
              // meter_select++;
              // Add content to enable move to next meter if exists ************
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
          connect_retry++;
          Serial.print("--MainTask-- No reply from meter, Retries: ");
          Serial.println(connect_retry);
          meter_task_step = METER_TASK_NORMAL_SEND;
          selfNextDutyDelay(NEXT_COMMAND_INTERVAL + OS_ST_PER_100_MS * 2);
          if (connect_retry > MAX_RETRY) {
            connect_retry = 0;
            Serial.println("--MainTask-- No reply from meter, Max Retries");
            // empty the meter status.
            item_rolling += 1;
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
  Meter_On = taskRegister(meter_on, OS_ST_PER_SECOND*20, 1, 0);
  Meter_Off = taskRegister(meter_off, OS_ST_PER_SECOND*10, 1, 0);
  Meter_Main_Task = taskRegister(meterMainTask, 1, 1, OS_ST_PER_SECOND);

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
