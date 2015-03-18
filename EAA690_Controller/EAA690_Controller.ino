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

#include <Wire.h>
#include <SD.h>
#include <Time.h>
#include <SPI.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetUdp.h>

/********************
 * GLOBAL VARIABLES *
 ********************/
unsigned long lastTime = 0;
  
// Arduino PINs
int SS_MICROSD = 4;
int RFIDResetPin = 8;
int I2C = 9;

// Network
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char server[] = "www.brianmichael.org";
IPAddress ip(192,168,0,107);
EthernetClient client;

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
  
  // Start the Ethernet connection
  if (Ethernet.begin(mac) == 0) {
    // Try to configure using IP address instead of DHCP
    Ethernet.begin(mac, ip);
  } 
  
  // Give the Ethernet shield a second to initialize
  delay(1000);
  
  Wire.begin();  
}

/************************************************
 * Main Loop                                    *
 *                                              *
 * This is the 2nd required functions for all   *
 * arduinos.  This function will be called      *
 * repeatedly until the arduino is powered off  *
 ************************************************/
void loop() {
  // Get the data once an hour
  EthernetUDP udp;
  unsigned long time = ntpUnixTime(udp);

  if (time >= lastTime + 3600) {
    getUserData();
    lastTime = time;
  }
  
  for (int i = 1; i < 9; i++) { // Up to 8 Door Controllers
    char val = 0; // variable to store the data from the serial port
    Wire.requestFrom(i, 20);
    String tag = "";
    while (Wire.available()) { // Door Controller may send less than requested
      val = Wire.read();
      tag = tag + val;
    }
    if (tag != "0") {
      Wire.beginTransmission(i); // transmit to device
      Wire.write(checkTag(i, tag)); // send access
      Wire.endTransmission(); // stop transmitting
    }
  }
  delay(500);
}

/**
 * Retrieve user database data (CSV) and store it 
 * on the SD card.
 */
void getUserData() {
  if (client.connect(server, 80)) {
    SD.remove("database.csv");
    // Make a HTTP request:
    client.println("GET /csv.php HTTP/1.1");
    String hostString = "Host: ";
    hostString.concat(server);
    client.println(hostString);
    client.println("Connection: close");
    client.println();
    // Get HTTP response
    File dbFile = SD.open("database.csv", FILE_WRITE);
    if (dbFile) {
      while (client.available()) { 
        char c = client.read();
        dbFile.print(c);
      }
      dbFile.close();
    }
    client.stop();
  }
}

/**
 * Check the read tag against known tags
 */
char checkTag(int door, String tag) {
  char access = 'N';
  EthernetUDP udp;
  unsigned long nowLong = ntpUnixTime(udp);
  
  if (sizeof(tag) > 2) {
    // 1. Find the record in the database file (on the SD card)
    // 2. Look for the "Door ID" value
    // 3. If it is a "1", then access is granted
    if (getValue(getRecord(tag), ',',  door + 1) == "1") {
      access = 'Y';
    }
  }
  
  File logFile = SD.open("system.log", FILE_WRITE);
  if (logFile) {
    logFile.println(nowLong + "," + tag + "," + "," + door + "," + access);
    logFile.close();
  }
  return access;
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
        String id = getValue(line, ',',  0);
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

/*
 * © Francesco Potortì 2013 - GPLv3 - Revision: 1.13
 *
 * Send an NTP packet and wait for the response, return the Unix time
 *
 * To lower the memory footprint, no buffers are allocated for sending
 * and receiving the NTP packets.  Four bytes of memory are allocated
 * for transmision, the rest is random garbage collected from the data
 * memory segment, and the received packet is read one byte at a time.
 * The Unix time is returned, that is, seconds from 1970-01-01T00:00.
 */
unsigned long inline ntpUnixTime (UDP &udp)
{
  static int udpInited = udp.begin(123); // open socket on arbitrary port

  const char timeServer[] = "pool.ntp.org";  // NTP server

  // Only the first four bytes of an outgoing NTP packet need to be set
  // appropriately, the rest can be whatever.
  const long ntpFirstFourBytes = 0xEC0600E3; // NTP request header

  // Fail if WiFiUdp.begin() could not init a socket
  if (! udpInited)
    return 0;

  // Clear received data from possible stray received packets
  udp.flush();

  // Send an NTP request
  if (! (udp.beginPacket(timeServer, 123) // 123 is the NTP port
	 && udp.write((byte *)&ntpFirstFourBytes, 48) == 48
	 && udp.endPacket()))
    return 0;				// sending request failed

  // Wait for response; check every pollIntv ms up to maxPoll times
  const int pollIntv = 150;		// poll every this many ms
  const byte maxPoll = 15;		// poll up to this many times
  int pktLen;				// received packet length
  for (byte i=0; i<maxPoll; i++) {
    if ((pktLen = udp.parsePacket()) == 48)
      break;
    delay(pollIntv);
  }
  if (pktLen != 48)
    return 0;				// no correct packet received

  // Read and discard the first useless bytes
  // Set useless to 32 for speed; set to 40 for accuracy.
  const byte useless = 40;
  for (byte i = 0; i < useless; ++i)
    udp.read();

  // Read the integer part of sending time
  unsigned long time = udp.read();	// NTP time
  for (byte i = 1; i < 4; i++)
    time = time << 8 | udp.read();

  // Round to the nearest second if we want accuracy
  // The fractionary part is the next byte divided by 256: if it is
  // greater than 500ms we round to the next second; we also account
  // for an assumed network delay of 50ms, and (0.5-0.05)*256=115;
  // additionally, we account for how much we delayed reading the packet
  // since its arrival, which we assume on average to be pollIntv/2.
  time += (udp.read() > 115 - pollIntv/8);

  // Discard the rest of the packet
  udp.flush();

  return time - 2208988800ul;		// convert NTP time to Unix time
}
