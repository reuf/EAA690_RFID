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
#include <SPI.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetUdp.h>
#include <util.h>

/********************
 * GLOBAL VARIABLES *
 ********************/
unsigned long lastTime = 0;
  
// Arduino PINs
int SS_MICROSD = 4;
int RFIDResetPin = 8;
int I2C_1 = 3;
int I2C_2 = 5;
int I2C_3 = 6;
int I2C_4 = 7;
int I2C_5 = 9;
int I2C_6 = 11;

// Network
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char server[] = "www.brianmichael.org";
IPAddress ip(192,168,0,107);
EthernetClient client;

// Testing...
String validcard = "554948485052706651505767";

// Transfer Object
EasyTransferI2C ET; 

struct RECEIVE_DATA_STRUCTURE {
  int id;
  int i2c;
  String tag;
  boolean accessGranted;
};

// Give a name to the group of data
RECEIVE_DATA_STRUCTURE transferData;

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
  
  Wire.begin(I2C_1);
  Wire.begin(I2C_2);
  Wire.begin(I2C_3);
  Wire.begin(I2C_4);
  Wire.begin(I2C_5);
  Wire.begin(I2C_6);
  
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
  // Get the data once an hour
  EthernetUDP udp;
  unsigned long time = ntpUnixTime(udp);

  if (time >= lastTime + 3600) {
    getUserData();
    lastTime = time;
  }
  
  delay(1000);
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
void checkTag(int numBytes) {
  EthernetUDP udp;
  unsigned long nowLong = ntpUnixTime(udp);
  
  if (transferData.tag == "") {
    transferData.accessGranted = false; //empty, no need to continue
  } else if (transferData.tag == validcard) {
    transferData.accessGranted = true; // Override this particular card
  } else {
    // 1. Find the record in the database file (on the SD card)
    // 2. Look for the "Door ID" value
    // 3. If it is a "1", then access is granted
    if (getValue(getRecord(transferData.tag), ',',  transferData.id + 1) == "1") {
      transferData.accessGranted = true;
    }
  }
  
  // Send response back to door
  ET.sendData(transferData.i2c);

  File logFile = SD.open("system.log", FILE_WRITE);
  if (logFile) {
    logFile.println(nowLong + "," + transferData.tag + "," + "," + transferData.id + "," + transferData.accessGranted);
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
