/************
 * Overview *
 ************
 This sketch is intended for the Arduino MINI which interacts with the RFID reader to assign a card to a user.
 The tasks are:
   1. Read RFID card's ID
   2. Provide RFID value to Raspberry Pi
   
 Simple schematic of overall system.  This sketch is for the "[New Card Assignment]" portion of the diagram.
 
            ^  ^
           +|  |- 
   +----------------+
   |    | to door | |
   |    +---------+ |           [Outside]
   |                |        +----------------+
   |        TIP31AG |        |      +-----+   |
   | +------+       |        |      | LED |   |
   | | A    |   +---|        |---+  +-----+   |
   | | r  M |   | R |-- // --| R |            |
   | | d  I |   | J |-- // --| J |  Resistors |
   | | u  N |   | 4 |-- // --| 4 |            |
   | | i  I |   | 5 |-- // --| 5 |  +------+  |
   | | n    |   +---|        |---+  | RFID |  |
   | | o    |       |        |      +------+  |
   | +------+       |        +----------------+
   |         LM7805 |
   |   +------+     |
   |   | RJ45 |     |
   +----------------+
         ||||  [Door Controller]     [New Card Assignment]
                                    +------------------+
         ////                       | +--------------+ |
                                    | | Arduino MINI | |
         ||||  [Main Controller]    | +--------------+ |
   +----------------------+         | +------+         |
   |   | RJ45 |  +------+ |         | | RFID |         |
   |   +------+  | RFID | |         | +------+         |
   |    LM7805   +------+ |         | +--------------+ |
   | +-------------+----+ |         | | Raspberry Pi | |
   | | Arduino UNO | SD | |         | +--------------+ |
   | +-------------+----+ |         +------------------+
   | | Ethernet    |      |
   +----------------------+         
         ||||
*/
// Include libraries
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>

int RFID_INPUT = 0;
int WIFI_HANDSHAKE = 7; // RESERVED
int RFIDResetPin = 8;
int WIFI_SHIELD_10 = 10; // RESERVED
int WIFI_SHIELD_11 = 11; // RESERVED
int WIFI_SHIELD_12 = 12; // RESERVED
int WIFI_SHIELD_13 = 13; // RESERVED

// Network
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,0,107);
WiFiClient client;
boolean wifiEnabled = true;
boolean passRequired = true;
char ssid[] = "bmichael"; // EAA690 your network SSID (name) // EAA690
char pass[] = "4047355660";  // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;
char server[] = "www.brianmichael.org";

// Debug
boolean debug = true;

void setup() {
  Serial.begin(9600);
  
  // Setup Arduino PINs
  pinMode(RFIDResetPin, OUTPUT);
  
  // Initialize the RFID reader for use
  digitalWrite(RFIDResetPin, HIGH);
  
  // Init the network
  initNetwork();
}

void loop() {
  String tagString = "";
  char val = 0; // variable to store the data from the serial port

  // Check to see if a tag needs to be checked
  while (Serial.available() > 0) {
    val = Serial.read();
    tagString = tagString + val;
    //Serial.print(val);
  }
  
  if (tagString != "") { // Tag data read
    tagString.replace("\n", "");
    tagString = tagString.substring(1, tagString.length() - 1);
    Serial.println(tagString);
    
    //reset the RFID reader
    //digitalWrite(RFIDResetPin, LOW);
    //digitalWrite(RFIDResetPin, HIGH);
  }
  
  // A short delay...
  delay(150);
}

/**
 * Initialize the network so user and NTP data
 * can be transmitted/received
 */
void initNetwork() {
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    if (debug) 
      Serial.println("WiFi shield not present");
    // don't continue:
    wifiEnabled = false;
  }

  // attempt to connect to Wifi network:
  if (wifiEnabled) {
    while (status != WL_CONNECTED) {
      String msg = "Connecting to ";
      msg.concat(ssid);
      if (debug) 
        Serial.println(msg);
      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
      if (passRequired) {
        status = WiFi.begin(ssid, pass);
      } else {
        if (debug) 
          Serial.println("Not passing password");
        status = WiFi.begin(ssid);
      }

      // wait 1 second for connection:
      delay(1000);
    }
    if (debug) 
      Serial.println("Connected to wifi");
  }
}

/**
 * Reads the RFID from the card scanned
 */
String readTag() {
  String tagString = "";
  int index = 0;
  boolean reading = false;

  while (Serial.available()) {
    int readByte = Serial.read(); //read next available byte
    if(readByte == 2) reading = true; //begining of tag
    if(readByte == 3) reading = false; //end of tag
    if(reading && readByte != 2 && readByte != 10 && readByte != 13){
      tagString = tagString + readByte;
      index ++;
    }
  }
  //if (debug) displayLCD(tagString, 1000, true);
  return tagString;
}
