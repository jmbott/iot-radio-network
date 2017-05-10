/*
  Web Server
 */

#include <SPI.h>
#include <Ethernet2.h>

// Feather M0 LoRa CS pin
#define WIZ_CS 10

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

// last time you connected to the server, in milliseconds
unsigned long lastConnectionTime = 0;
// delay between updates, in milliseconds
// the "L" is needed to use long type numbers
const unsigned long postingInterval = 10L * 1000L;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  delay(1500);

  // switch from radio to ethernet
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);

  Serial.println("\nHello World");

  // pin for feather init
  Ethernet.init(WIZ_CS);

  // give the ethernet module time to boot up:
  delay(1000);

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip, dnServer, gateway, subnet);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}


void loop() {

  // if ten seconds have passed since your last connection,
  // then connect again and send data:
  if (millis() - lastConnectionTime > postingInterval) {
    httpRequest();
  }
}

// this method makes a HTTP connection to the server:
void httpRequest() {

  // listen for incoming clients
  EthernetClient client = server.available();

  if (client) {
    Serial.println("client exists");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
           client.println("<!DOCTYPE HTML>");
           client.println("<HTML>");
           client.println("<HEAD>");
           client.println("<meta name='apple-mobile-web-app-capable' content='yes' />");
           client.println("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />");
           client.println("<link rel='stylesheet' type='text/css' href='http://randomnerdtutorials.com/ethernetcss.css' />");
           client.println("<TITLE>Project of EEECS E6765: Energy Usage Monitoring</TITLE>");
           client.println("</HEAD>");
           client.println("<BODY>");
           client.println("<H1>Project of EECS E6765: Energy Usage Monitoring</H1>");
           client.println("<hr />");
           client.println("<br />");  
           client.println("<H2>Arduino with Ethernet Shield</H2>");
           client.println("<br />");  
           client.println("<a href=\"/?meter1\"\">Moniter Meter1</a>");
           client.println("<a href=\"/?meter2\"\">Moniter Meter2</a><br />");   
           client.println("<br />");     
           client.println("<br />"); 
           client.println("<a href=\"/?dataon\"\">Send Data /On(Auto)</a>");
           client.println("<a href=\"/?dataoff\"\">Send Data /Off</a><br />"); 
           client.println("<br />");
           client.println("<p>Group ID: JSJC</p>");  
           client.println("<br />");  
           client.println("<br />"); 
           client.println("</BODY>");
           client.println("</HTML>");

           delay(1);
                             
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

    // note the time that the connection was made:
    lastConnectionTime = millis();

    // give the web browser time to receive the data
    delay(1);

  }
}

