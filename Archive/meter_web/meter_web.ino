/*
  Web Server
 */

#include <SPI.h>
#include <Ethernet2.h>

#define WIZ_CS 10
#define REQ_BUF_SZ   50

// Ethernet module MAC address
byte mac[] = { 0x98, 0x76, 0xB6, 0x10, 0x56, 0xed };

// Set the static IP address, dns, gateway, subnet
byte ip[] = { 192, 168, 0, 177 };
byte dnServer[] = { 192, 168, 0, 1 };
byte gateway[] = { 192, 168, 0, 1 };
byte subnet[] = { 255, 255, 255, 0 };

// Initialize the Ethernet port
// (port 80 is default for HTTP):
EthernetServer server(8080);

char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer

int meter1 = 0;
int meter2 = 0;
int meter3 = 0;

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
  Ethernet.begin(mac, ip);//, dnServer, gateway, subnet
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  randomSeed(233); //for test
}

void loop() {
    meter1 = random(10,20);
    meter2 = random(40,50);
    meter3 = random(70,80);
    httpRequest();

}

// this method makes a HTTP connection to the server:
void httpRequest() {

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
  }
}

void XML_response(EthernetClient cl)
{
    int analog_val;
    
    cl.print("<?xml version = \"1.0\" ?>");
    cl.print("<inputs>");
    analog_val = meter1; // for test
    cl.print("<analog>");
    cl.print(analog_val);
    cl.print("</analog>");
    analog_val = meter2; // for test
    cl.print("<analog>");
    cl.print(analog_val);
    cl.print("</analog>");
    analog_val = meter3; // for test
    cl.print("<analog>");
    cl.print(analog_val);
    cl.print("</analog>");
    cl.print("</inputs>");
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
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

