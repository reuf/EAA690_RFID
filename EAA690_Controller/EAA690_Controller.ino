/************
 * Overview *
 ************
 This sketch is intended for the Arduino UNO which acts as a controller for the entire system.
 The tasks are:
   1. Keep track of time, and record events (on an SD card) noting the time each occurs
   2. Maintain a database (on an SD card) of RFID cards and their validity for a given door.
   3. Respond to door controller (represented by [Inside] in the diagram below) requests with card accessibility.
   4. Maintain a database (on an SD card) via a network connection to the EAA 690 membership database.
   5. Publish event activity on a regular basis to an external system.
   
 Simple schematic of overall system.  This sketch is for the "[Controller]" portion of the diagram.
 
            ^  ^
           +|  |- [Inside]
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
         ||||
      
         ////
      
         ||||  [Controller]
   +----------------------+
   |   | RJ45 |  +------+ |
   |   +------+  | RFID | |
   |             +------+ |
   | +-------------+      |
   | | Arduino UNO |      |
   | +-------------+      |
   +----------------------+
   
*/

#include <Wire.h>
#include <SD.h>
#include <EasyTransferI2C.h>
#include <Time.h>
#include <WiFi.h>
#include <WiFiUdp.h>

/********************
 * GLOBAL VARIABLES *
 ********************/

// Arduino PINs
int SS_MICROSD = 4;
int RFIDResetPin = 8;

// Network
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,0,107);
WiFiClient client;
boolean wifiEnabled = true;
boolean passRequired = false;
char ssid[] = "EAA690"; // EAA690 your network SSID (name) // EAA690
char pass[] = "PASSWORD";  // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;

// SD Card
boolean sdEnabled = true;
char dbDelimiter = ',';

char server[] = "www.brianmichael.org";
String validcard = "554948485052706651505767";

// NTP (time)
unsigned long lastCheck = 0;
unsigned long epoch = 0;
long checkDelay = 600000;
unsigned int localPort = 2390; // local port to listen for UDP packets
IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE];
WiFiUDP Udp;

// Transfer Object
EasyTransferI2C ET; 

struct RECEIVE_DATA_STRUCTURE{
  int id;
  String tag;
  boolean accessGranted;
};

// Give a name to the group of data
RECEIVE_DATA_STRUCTURE transferData;

//define slave i2c address
#define I2C_SLAVE_ADDRESS 9

/************************************************
 * Setup                                        *
 *                                              *
 * This is 1 of 2 required functions for all    *
 * arduinos.  It is called upon startup (once)  *
 * and initializes any variables and settings   *
 ************************************************/
void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);   // Init the computer console
  
  // Setup Arduino PINs
  pinMode(SS_MICROSD, OUTPUT); // SD SS pin
  
  // Init the network
  initNetwork();
  
  // Check the SD card
  if (!SD.begin(SS_MICROSD)) {
    sdEnabled = false;
  }
  
  Wire.begin(I2C_SLAVE_ADDRESS);
  
  // Start the library, pass in the data details and the name of the serial port. 
  // Can be Serial, Serial1, Serial2, etc. 
  ET.begin(details(transferData), &Wire);

  // Define handler function on receiving data
  Wire.onReceive(checkTag); 
}

/************************************************
 * Main Loop                                    *
 *                                              *
 * This is the 2nd required functions for all   *
 * arduinos.  This function will be called      *
 * repeatedly until the arduino is powered off  *
 ************************************************/
void loop() {
  // Get the data once per hour, at the top of the hour
  unsigned long nowLong = now();
  //if ((nowLong % 86400) / 60 == 0) {
  //  getUserData();
  //}

  String timeStr = getTimeAsString();
}

/**
 * Initialize the network so user and NTP data 
 * can be transmitted/received
 */
void initNetwork() {
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    // don't continue:
    wifiEnabled = false;
  } 
  
  // attempt to connect to Wifi network:
  if (wifiEnabled) {
    while (status != WL_CONNECTED) { 
      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:    
      if (passRequired) {
        status = WiFi.begin(ssid, pass);
      } else {
        status = WiFi.begin(ssid);
      }
  
      // wait 1 second for connection:
      delay(1000);
    } 
    Udp.begin(localPort);
  }
}

/**
 * Retrieve user database data (CSV) and store it 
 * on the SD card.
 */
void getUserData() {
  //Serial.println(tag); //read out any unknown tag
  if (wifiEnabled && sdEnabled && client.connect(server, 80)) {
    // Make a HTTP request:
    String query = "GET /csv.php";
    query.concat(" HTTP/1.1");
    //Serial.println(query);
    client.println(query);
    String hostString = "Host: ";
    hostString.concat(server);
    client.println(hostString);
    client.println("Connection: close");
    client.println();
    // Get HTTP response
    File dbFile = SD.open("database.csv", FILE_WRITE);
    while (client.available()) { 
      char c = client.read();
      //Serial.print(c);
      dbFile.print(c);
    }
    dbFile.close();
    client.stop();
  }
}

/**
 * Updates the arduino's "clock" with the current time from 
 * time.nist.gov
 */
void updateTime() {
  unsigned long currentMillis = millis();
  if (epoch <= 0 || currentMillis - lastCheck > checkDelay) {
    lastCheck = currentMillis;
    sendNTPpacket(timeServer);
    if (Udp.parsePacket()) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      const unsigned long seventyYears = 2208988800UL;
      epoch = secsSince1900 - seventyYears;
      time_t ntptime = epoch;
      setTime(ntptime);   // Sync Arduino clock to the time received via NTP
    } 
  }
}

/**
 * Formats the arduino's "clock" time to a human readable format
 */
String getTimeAsString() {
  updateTime();
  unsigned long nowLong = now();
  String timeStr = "";
  timeStr.concat((nowLong % 86400) / 3600);
  timeStr.concat(":");
  if (((nowLong % 3600) / 60) <  10) {
    timeStr.concat("0");
  }
  timeStr.concat((nowLong % 3600) / 60);
  timeStr.concat(":");
  if ((nowLong % 60) <  10) {
    timeStr.concat("0");
  }
  timeStr.concat(nowLong % 60);
  timeStr.concat("Z");
  return timeStr;
}

/**
 * Helper method for updateTime().  
 * Sends a UDP packet to time.nist.gov to get a NTP packet
 */
unsigned long sendNTPpacket(IPAddress& address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

/**
 * Check the read tag against known tags
 */
void checkTag(int numBytes) {
  if (transferData.tag == "") transferData.accessGranted = false; //empty, no need to contunue
  if (transferData.tag == validcard) transferData.accessGranted = true; // Override this particular card

  String record = getRecord(transferData.tag);
  String access = getValue(record, dbDelimiter,  4);
  if (true) { //TOOD
    transferData.accessGranted = true;
  }
}

/**
 * Retrieves the user entry from the SD card for the tag ID provided
 */
String getRecord(String tag) {
  String returnLine;
  if (sdEnabled) {
    if (SD.exists("database.csv")) {
      File dbFile = SD.open("database.csv");
      String line = ""; // = "554948485052706651505767|1|0|0|0|0|0";
      int i = 0;
      while (dbFile.available()) {
        char c = dbFile.read();
        if (c == '\n') {
          String id = getValue(line, dbDelimiter,  0);
          if (id == tag) {
            returnLine = line;
            break;
          }
          line = "";
        } else {
          line += c;
        }
      }
      dbFile.close();
    }
  }
  return returnLine;
}

/**
 * Helper routine for getRecord, and other functions
 * Parses a line, and returns the value at the specified location.
 */
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}
