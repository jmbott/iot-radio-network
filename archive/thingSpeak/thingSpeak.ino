
#include "ThingSpeak.h"

#include <SPI.h>
#include <Ethernet2.h>

byte mac[] = { 0x98, 0x76, 0xB6, 0x10, 0x56, 0xed };
EthernetClient client;

#define WIZ_CS 10


/*
  *****************************************************************************************
  **** Visit https://www.thingspeak.com to sign up for a free account and create
  **** a channel.  The video tutorial http://community.thingspeak.com/tutorials/thingspeak-channels/ 
  **** has more information. You need to change this to your channel, and your write API key
  **** IF YOU SHARE YOUR CODE WITH OTHERS, MAKE SURE YOU REMOVE YOUR WRITE API KEY!!
  *****************************************************************************************/
unsigned long myChannelNumber = 268396;
const char * myWriteAPIKey = "9WPARD55UC7GX1M4";

float voltage1 = 3.0;
float voltage2 = 3.0;
float voltage3 = 3.0;
void setup() {

  Serial.begin(9600);
  delay(1500);

  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);

  Ethernet.init(WIZ_CS);
  Ethernet.begin(mac);

  ThingSpeak.begin(client);
  randomSeed(444);
}

void loop() {
  
  voltage1 = random(20,30);
  voltage2 = random(40,50);
  voltage3 = random(70,80);

  ThingSpeak.setField(1, (float)voltage1);
  ThingSpeak.setField(2, (float)voltage2);
  ThingSpeak.setField(3, (float)voltage3);

  // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
  // pieces of information in a channel.  Here, we write to field 1.
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);  


  delay(20000); // ThingSpeak will only accept updates every 15 seconds.
}
