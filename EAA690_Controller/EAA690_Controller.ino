/************
 * Overview *
 ************
 This sketch is intended for the Arduino UNO which acts as a controller for the entire system.
 The tasks are:
   // General tasks
   1. Keep track of time.
   
   // Event tasks (stored on an SD card)
   2. Record access events noting the time each occurs.
   3. Provide access to event activity upon request.
   4. Provide the ability to purge access data.

   // Database tasks (stored on an SD card)
   5. Maintain (CRUD operations) a database of RFID cards and their validity for a given door.
   6. Respond to "Door Controller" requests with card accessibility.
   7. Provide the ability to force a refresh of the database.
   8. Perform a database refresh on a timed basis.

   // Network tasks
   9. Maintain a network connection to the EAA 690 membership database.
   
 Simple schematic of overall system.  This sketch is for the "[Main Controller]" portion of the diagram.
 
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
   |   | RJ45 |           |         | | RFID |         |
   |   +------+           |         | +------+         |
   |    LM7805            |         | +--------------+ |
   | +-------------+----+ |         | | Raspberry Pi | |
   | | Arduino UNO | SD | |         | +--------------+ |
   | +-------------+----+ |         +------------------+
   | | Ethernet    |      |
   +----------------------+         
         ||||
*/

#include <Wire.h>
#include <SD.h>
#include <Time.h>
#include <SPI.h>
#include <Dhcp.h>
#include <Dns.h>
#include <EasyTransferI2C.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetUdp.h>

/********************
 * GLOBAL VARIABLES *
 ********************/
// Time - NTP Servers:
IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov
const int timeZone = -5;  // Eastern Standard Time (USA)
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

// Network
EthernetClient client;
EthernetUDP Udp;
boolean networkEnabled = false;

EasyTransferI2C ET;

struct DATA_STRUCTURE {
  int access = 0;
  int door = -1;
  char char1;
  char char2;
  char char3;
  char char4;
  char char5;
  char char6;
  char char7;
  char char8;
  char char9;
  char char10;
  char char11;
  char char12;
};

DATA_STRUCTURE tagData;

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
  
  // Initialize the SD card
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  // See if the card is present and can be initialized:
  if (!SD.begin(4)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  
  // Start the Ethernet connection
  byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0xF1, 0xD1 };
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Try to configure using IP address instead of DHCP
    //Ethernet.begin(mac, ip);
  } else {
    networkEnabled = true;
  }
  
  // Give the Ethernet shield a second to initialize
  delay(1000);
  
  if (networkEnabled) {
    unsigned int localPort = 8888;  // local port to listen for UDP packets
    Udp.begin(localPort);
    setSyncProvider(getNtpTime);
  }
  
  Wire.begin(0);
  ET.begin(details(tagData), &Wire);
  Wire.onReceive(receive);

  Serial.println("Controller setup complete.");
}

/************************************************
 * Main Loop                                    *
 *                                              *
 * This is the 2nd required functions for all   *
 * arduinos.  This function will be called      *
 * repeatedly until the arduino is powered off  *
 ************************************************/
void loop() {
  if (networkEnabled) {
    // Retrieve the user access database every hour
    if (minute() == 0 && second() >= 0 && second() <= 2) {
      getUserData();
    }
    // Post usage data to the server every 6 hours
    if ((hour() == 0 || hour() == 6 || hour() == 12 || hour() == 18) && 
        minute() == 0 && second() >= 0 && second() <= 2) {
      postUsageData();
    }
  }
  
  // Handle door activity
  if (ET.receiveData()) {
    //Serial.print("Tag data [");
    //Serial.print(getTag());
    //Serial.print("] found at door #");
    //Serial.println(tagData.door);
    checkTag();
    ET.sendData(tagData.door);
  }
}

// Convenience method to retrieve the tag as a string
String getTag() {
  String tag = "";
  tag += tagData.char1;
  tag += tagData.char2;
  tag += tagData.char3;
  tag += tagData.char4;
  tag += tagData.char5;
  tag += tagData.char6;
  tag += tagData.char7;
  tag += tagData.char8;
  tag += tagData.char9;
  tag += tagData.char10;
  tag += tagData.char11;
  tag += tagData.char12;
  return tag;
}

// Called by Door Controller to let us know a tag has been read
void receive(int howMany) {}

/**
 * Retrieve user database data (CSV) and store it 
 * on the SD card.
 */
void getUserData() {
  char server[] = "www.brianmichael.org";
  if (client.connect(server, 80)) {
    // Make a HTTP request:
    client.println("GET /database.csv HTTP/1.1");
    client.println("Host: www.brianmichael.org"); // This has to be defined this way :(
    client.println("Connection: close");
    client.println();
    if (SD.exists("database.csv")) {
      if (SD.remove("database.csv")) {
        //Serial.println("database.csv removed");
      }
    }
    File dbFile = SD.open("database.csv", FILE_WRITE);
    // Give the buffer a chance to get some data
    delay(2000);
    boolean returnAndLF = false;
    boolean prevCharIsReturn = false;
    boolean atFile = false;
    while (client.available()) {
      char c = client.read();
      if (atFile) {
        //Serial.print(c);
        if (dbFile) {
          dbFile.print(c);
        }
      // Do some spiffy detection logic to figure out where 
      // the header stops and the actual file starts
      } else if (c == '\n' && prevCharIsReturn && returnAndLF) {
        atFile = true;
      } else if (c == '\n' && prevCharIsReturn) {
        returnAndLF = true;
      } else if (c == '\r') {
        prevCharIsReturn = true;
      } else {
        prevCharIsReturn = false;
        returnAndLF = false;
      }
    }
    // if the server's disconnected, stop the client:
    if (!client.connected()) {
      if (dbFile) {
        dbFile.flush();
        dbFile.close();
      }
      client.stop();
    }
  }
}

void postUsageData() {
  // Send data to server
  if (SD.exists("system.log")) { // Only send usage if it exists
    File logFile = SD.open("system.log");
    String line = ""; // = "Timestamp,tag,door,access;
    while (logFile.available()) {
      char c = logFile.read();
      if (c == '\n') { // We've reached the end of the line, send it
        char server[] = "www.brianmichael.org";
        if (client.connect(server, 80)) {
          client.println("POST /addUsage.php HTTP/1.1");
          client.println("HOST: www.brianmichael.org"); // This has to be defined this way :(
          client.println("Content-Type: application/x-www-form-urlencoded");
          client.print("Content-Length: ");
          client.println(line.length());
          client.println();
          //Serial.print("Sending [");
          //Serial.print(line);
          //Serial.println("] to server");
          client.print(line);
        }
        if (client.connected()) {
          client.stop();
        }
        line = ""; // Clear the line buffer for the next line
      } else {
        line += c; // We have not yet reached the end of the line.  
                   // Add the char to the buffer and continue reading.
      }
    }
    logFile.close();
  }  
  // Clear usage data from SD card
  if (SD.remove("system.log")) {
    //Serial.println("system.log removed");
  }
}

/**
 * Check the read tag against known tags
 */
void checkTag() {
  //if (sizeof(tag) > 2) {
    // 1. Find the record in the database file (on the SD card)
    // 2. Look for the "Door ID" value
    // 3. If it is a "1", then access is granted
  if (getValue(getRecord(getTag()), ',',  tagData.door + 1) == "1") {
    tagData.access = 1;
  } else if (getTag() == "710024FB3299") { // Master card
    tagData.access = 1;
  } else {
    tagData.access = 0;
  }
  
  File logFile = SD.open("system.log", FILE_WRITE);
  if (logFile) {
    //Serial.println(digitalClockDisplay() + "," + getTag() + "," + tagData.door + "," + tagData.access);
    logFile.println(digitalClockDisplay() + "," + getTag() + "," + tagData.door + "," + tagData.access);
    logFile.flush();
    logFile.close();
  }
}

/**
 * Retrieves the user entry from the SD card for the tag ID provided
 */
String getRecord(String tag) {
  String returnLine;
  if (SD.exists("database.csv")) {
    File dbFile = SD.open("database.csv");
    String line = ""; // = "554948485052706651505767,1,0,0,0,0,0";
    int i = 0;
    while (dbFile.available()) {
      char c = dbFile.read();
      if (c == '\n') {
        String id = getValue(line, ',', 0);
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
  //Serial.print("getRecord() returning: ");
  //Serial.println(returnLine);
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

/*-------- NTP code ----------*/

String digitalClockDisplay() {
  String timeStr = "";
  // digital clock display of the time
  timeStr += year(); 
  timeStr += "-";
  timeStr += month();
  timeStr += "-";
  timeStr += day();
  timeStr += " ";
  timeStr += hour();
  timeStr += printDigits(minute());
  timeStr += printDigits(second());
  return timeStr;
}

String printDigits(int digits) {
  String digitsStr = ":";
  // utility for digital clock display: prints preceding colon and leading 0
  if (digits < 10)
    digitsStr += "0";
  digitsStr += digits;
  return digitsStr;
}

time_t getNtpTime() {
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  //Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      //Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  //Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
