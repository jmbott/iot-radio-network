/*
  IOT Project: JSJC

  Main controller for radio node network

  03.23.2017 - init & programming controller
  03.26.2017 - basic transmission code added, RH_RF95 class
  05.09.2017 - Add lightOS

*/

#include "lightOS.h"    // LightOS
#include "Timer.h"
#include <Ethernet2.h>  // Webserver
//#include <SPI.h>
#include <SPI.h>        // Radio
#include <RH_RF95.h>


/********************************* lightOS init **********************************/

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


#define WIZ_CS 10
#define REQ_BUF_SZ   50

// Ethernet module MAC address
byte mac[] = { 0x98, 0x76, 0xB6, 0x10, 0x56, 0xed };

// Set the static IP address, dns, gateway, subnet
byte ip[] = { 192, 168, 1, 177 };
byte dnServer[] = { 192, 168, 1, 1 };
byte gateway[] = { 192, 168, 1, 1 };
byte subnet[] = { 255, 255, 255, 0 };

// Initialize the Ethernet port
// (port 80 is default for HTTP):
EthernetServer server(8080);

char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer

String readString;

int meter1 = 0;
int meter2 = 0;
int meter3 = 0;

/****************************** ethernet init end *******************************/

/********************************* WAN Time Sync *********************************/

// To Do

/******************************* WAN Time Sync end *******************************/

/********************************** echo task ***********************************/

unsigned int echo(int opt) {
  // Switch from ethernet to radio
  digitalWrite(8, LOW);
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

  // Switch from Ethernet to radio
  digitalWrite(8, LOW);

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

/****************************** Webserver tasks *******************************/

// this method makes a HTTP connection to the server:
unsigned int httpRequest(int opt) {

  // Switch from radio to Ethernet
  digitalWrite(8, HIGH);

  // listen for incoming clients
  EthernetClient client = server.available();

  if (client) {
    boolean currentLineIsBlank = true;

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
      if (req_index < (REQ_BUF_SZ - 1)) {
         HTTP_req[req_index] = c;          // save HTTP request character
         req_index++;
       }
       if (readString.length() < 100) {
                    //store characters to string
                    readString += c;
       }

       // if you've gotten to the end of the line (received a newline
       // character) and the line is blank, the http request has ended,
       // so you can send a reply
       if (c == '\n' && currentLineIsBlank) {
         // send a standard http response header
         client.println("HTTP/1.1 200 OK");
         if (StrContains(HTTP_req, "ajax_inputs")) {
            // send rest of HTTP header
            client.println("Content-Type: text/xml");
            client.println("Connection: keep-alive");
            client.println();
            // send XML file containing input states
            XML_response(client);
          }
          else {
            client.println("Content-Type: text/html");
            client.println("Connection: keep-alive");
            client.println();
            client.println("<!DOCTYPE html>");
            client.println("<HTML>");
            client.println("<head>");
            client.println("<meta charset='UTF-8'>");
            client.println("<meta http-equiv='X-UA-Compatible' content='IE=edge,chrome=1'> ");
            client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
            client.println("<title>HTML5 Canvas</title>");
            client.println("<style type='text/css'>.btn-container{padding: 1em 0;text-align: center;}.container{width: 700px;margin: 50px auto;}");
            client.println("body{background: #494A5F;color: #D5D6E2;font-weight: 500;font-size: 1.05em;font-family: \"Microsoft YaHei\",\"Segoe UI\", \"Lucida Grande\", Helvetica,Arial,sans-serif;}");
            client.println(".zzsc-header{padding: 1em 190px 1em;letter-spacing: -1px;text-align: center;background: #66677c;}");
            client.println("</style>");
            client.println("<script src='https://canvas-gauges.com/download/latest/all/gauge.min.js'></script>");
            client.println("<script>");
            client.println("function GetArduinoInputs() {");
            client.println("nocache = \"&nocache=\"\ + Math.random() * 1000000;");
            client.println("var request = new XMLHttpRequest();");
            client.println("request.onreadystatechange = function() {");
            client.println("if (this.readyState == 4) {");
            client.println("if (this.status == 200) {");
            client.println("if (this.responseXML != null) {");
            client.println("gauge1.value = this.responseXML.getElementsByTagName('analog')[0].childNodes[0].nodeValue;");
            client.println("gauge2.value = this.responseXML.getElementsByTagName('analog')[1].childNodes[0].nodeValue;");
            client.println("gauge3.value = this.responseXML.getElementsByTagName('analog')[2].childNodes[0].nodeValue;");
            client.println("}}}}");
            client.println("request.open(\"GET\", \"ajax_inputs\" + nocache, true);");
            client.println("request.send(null);");
            client.println("setTimeout('GetArduinoInputs()', 1000);");
            client.println("}");
            client.println("</script>");
            client.println("</head>");
            client.println("<div class=\"zzsc-container\">");
            client.println("<header class=\"zzsc-header\">");
            client.println("<div class=\"zzsc-demo center\">");
            client.println("<p style=\"font-size:190%;\">Project of EECS E6765: Energy Usage Monitoring</p>");
            client.println("</div>");
            client.println("</header>");
            client.println("<div style=\"text-align:center;clear:both\">");
            client.println("</div>");
            //client.println("<div class=\"balance1\">");
            //client.println("<p><span style=\"left:900px;\">BALANCE:<span style=\"margin-left:20%;\">BALANCE:<span style=\"margin-left:20%;\">BALANCE:</p>");
            //client.println("</div>");
            client.println("<div class=\"container\">");
            client.println("<canvas id=\"gauge1\"></canvas>");
            client.println("<canvas id=\"gauge2\"></canvas>");
            client.println("<canvas id=\"gauge3\"></canvas>");
            client.println("</div>");
            client.println("</div>");
            client.println("<script>");
            client.println("var gauge1 = new RadialGauge({renderTo: 'gauge1',width: 220,height: 220,units: \"Meter 1\",}).draw();");
            client.println("var gauge2 = new RadialGauge({renderTo: 'gauge2',width: 220,height: 220,units: \"Meter 2\",}).draw();");
            client.println("var gauge3 = new RadialGauge({renderTo: 'gauge3',width: 220,height: 220,units: \"Meter 3\",}).draw();");
            client.println("gauge1.value = 0;");
            client.println("gauge2.value = 0;");
            client.println("gauge3.value = 0;");
            client.println("</script>");
            client.println("METER1:");
            client.println("<a href=\"/?LEDON1\"\">On</a>");
            client.println("<a href=\"/?LEDOFF1\"\">Off</a><br>");
            client.println("METER2:");
            client.println("<a href=\"/?LEDON2\"\">On</a>");
            client.println("<a href=\"/?LEDOFF2\"\">Off</a><br>");
            client.println("METER3:");
            client.println("<a href=\"/?LEDON3\"\">On</a>");
            client.println("<a href=\"/?LEDOFF3\"\">Off</a><br>");
            client.println("<body onload=\"GetArduinoInputs()\">");
            client.println(" <h1 style=\"text-align: center\">Group ID: JSJC</h1>");
            client.println("</body>");
            client.println("</html>");
          }

          Serial.print(HTTP_req);
          // reset buffer index and all buffer elements to 0
          req_index = 0;
          StrClear(HTTP_req, REQ_BUF_SZ);
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }

    // give the web browser time to receive the data
    delay(1);
    client.stop(); // close the connection
    if(readString.indexOf("?LEDON1") > -1) //checks for LEDON
    {
      meter_select = 0;
      switch_on_meter(0);
      //digitalWrite(LED, HIGH); // set pin high
    }
    else {
      if (readString.indexOf("?LEDOFF1") > -1) //checks for LEDOFF
      {
        meter_select = 0;
        switch_off_meter(0);
        //digitalWrite(LED, LOW); // set pin low
      }
    }
    if(readString.indexOf("?LEDON2") > -1) //checks for LEDON
    {
      meter_select = 1;
      switch_on_meter(0);
      //digitalWrite(LED, HIGH); // set pin high
    }
    else {
      if (readString.indexOf("?LEDOFF2") > -1) //checks for LEDOFF
      {
        meter_select = 1;
        switch_off_meter(0);
        //digitalWrite(LED, LOW); // set pin low
      }
    }
    if(readString.indexOf("?LEDON3") > -1) //checks for LEDON
    {
      meter_select = 2;
      switch_on_meter(0);
      //digitalWrite(LED, HIGH); // set pin high
    }
    else {
      if (readString.indexOf("?LEDOFF3") > -1) //checks for LEDOFF
      {
        meter_select = 2;
        switch_off_meter(0);
        //digitalWrite(LED, LOW); // set pin low
      }
    }
    readString="";
  }
  return 1;
}

void XML_response(EthernetClient cl) {
    int analog_val;
    cl.print("<?xml version = \"1.0\" ?>");
    cl.print("<inputs>");
    analog_val = meter1; // for test
    cl.print("<analog>");
    cl.print(meter_type[0].watt);
    cl.print("</analog>");
    analog_val = meter2; // for test
    cl.print("<analog>");
    cl.print(meter_type[1].watt);
    cl.print("</analog>");
    analog_val = meter3; // for test
    cl.print("<analog>");
    cl.print(meter_type[2].watt);
    cl.print("</analog>");
    cl.print("</inputs>");
}

// sets every element of str to 0 (clears array)
unsigned int StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
    return 1;
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);

    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }
    return 0;
}

/****************************** Webserver tasks end *******************************/

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

  // switch from radio to ethernet
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);

  // pin for feather init
  Ethernet.init(WIZ_CS);

  // give the ethernet module time to boot up:
  delay(1000);

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);//, dnServer, gateway, subnet
  pinMode(LED, OUTPUT); //pin selected to control
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  // lightOS setup ****************************************************************/
  osSetup();
  Serial.println("OS Setup OK !");

  /* initialize application task
  OS_TASK *taskRegister(unsigned int (*funP)(int opt),unsigned long interval,unsigned char status,unsigned long temp_interval)
  taskRegister(FUNCTION, INTERVAL, STATUS, TEMP_INTERVAL); */
  //Echo = taskRegister(echo, OS_ST_PER_SECOND*10, 1, 0);
  //SwitchOnMeter = taskRegister(switch_on_meter, OS_ST_PER_SECOND*120, 1, 0);
  //SwitchOffMeter = taskRegister(switch_off_meter, OS_ST_PER_SECOND*240, 1, 0);
  RFReadMeter = taskRegister(rf_read_meter, OS_ST_PER_SECOND*10, 1, 0);

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
  httpRequest(0);
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
