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
#include <EasyTransferI2C.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetUdp.h>

/********************
 * GLOBAL VARIABLES *
 ********************/
unsigned long lastTime = 0;
  
// Arduino PINs
int SS_MICROSD = 10;
int RFIDResetPin = 8;
int I2C = 0;

// Time - NTP Servers:
IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov
//const int timeZone = 1;     // Central European Time
const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)
time_t prevDisplay = 0; // when the digital clock was displayed
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

// Network
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0xF1, 0xD1 };
char server[] = "www.brianmichael.org";
//IPAddress ip(192,168,0,107);
EthernetClient client;
EthernetUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
const int chipSelect = 4;    

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
  
  initSD();
  
  // Start the Ethernet connection
  if (Ethernet.begin(mac) == 0) {
    // Try to configure using IP address instead of DHCP
    //Ethernet.begin(mac, ip);
  }
  
  // Give the Ethernet shield a second to initialize
  delay(1000);
  
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  
  Wire.begin(0);
  ET.begin(details(tagData), &Wire);
  Wire.onReceive(receive);

  Serial.println("Setup complete.");
  //getUserData();
}

/************************************************
 * Main Loop                                    *
 *                                              *
 * This is the 2nd required functions for all   *
 * arduinos.  This function will be called      *
 * repeatedly until the arduino is powered off  *
 ************************************************/
void loop() {
  if (ET.receiveData()) {
    Serial.print("Tag data [");
    Serial.print(getTag());
    Serial.print("] found at door #");
    Serial.println(tagData.door);
    checkTag();
    ET.sendData(tagData.door);
  }
  delay(5000);
}

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

void initSD() {
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin 
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output 
  // or the SD library functions will not work. 
  //pinMode(10, OUTPUT);     // change this to 53 on a mega
  pinMode(SS_MICROSD, OUTPUT); // SD SS pin

  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    //Serial.println("initialization failed.");
    return;
  } else {
   //Serial.println("Wiring is correct and a card is present."); 
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    //Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    return;
  }
}

/**
 * Retrieve user database data (CSV) and store it 
 * on the SD card.
 */
void getUserData() {
  String webData = connectAndRead();
  if (webData != "") {
    //if (SD.remove("database.csv")) {
      //Serial.println("database.csv removed");
    //}
    File dbFile = SD.open("database.csv", FILE_WRITE);
    if (dbFile) {
      dbFile.println(webData);
      dbFile.flush();
      dbFile.close();
    } else {
      Serial.println("dbFile was not opened");
    }
    client.stop();
  }
}

String connectAndRead(){
  //connect to the server
  //port 80 is typical of a www page
  if (client.connect(server, 80)) {
    //Serial.println("connected");
    client.print("GET /csv.php");
    client.println(" HTTP/1.1");
    String hostString = "Host: ";
    hostString.concat(server);
    client.println(hostString);
    client.println("User-Agent: Arduino");
    client.println("Accept: text/html");
    client.println("Connection: close");
    client.println();
     
    delay(500);
    //Connected - Read the page
    return readPage(); //go and read the output
  } else {
    return "";
  }
}

String readPage(){
  //read the page, and capture & return everything between '@' and '@'
  String data = ""; // string for incoming serial data
  boolean startRead = false; // is reading?
  while (true) {
    if (client.available()) {
      char c = client.read();
      if (c == '@' ) { //'@' is our begining character
        startRead = true; //Ready to start reading the part 
      } else if (startRead) {
        if (c != '@') { //'@' is our ending character
          data += c;
        } else {
          //got what we need here! We can disconnect now
          startRead = false;
          client.stop();
          client.flush();
          //Serial.println("disconnecting.");
          return data;
        }
      }
    }
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
    //if (getValue(getRecord(tag), ',',  door + 1) == "1") {
    if (getTag() == "710024FB3299") {
      tagData.access = 1;
    }
  //}
  
  //if (timeStatus() != timeNotSet) {
  //  if (now() != prevDisplay) { //update the display only if time has changed
  //    prevDisplay = now();
  //  }
  //}
  
  File logFile = SD.open("system.log", FILE_WRITE);
  if (logFile) {
    Serial.println(digitalClockDisplay() + "," + tagData.char1 + "," + "," + tagData.door + "," + tagData.access);
    logFile.println(digitalClockDisplay() + "," + tagData.char1 + "," + "," + tagData.door + "," + tagData.access);
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

/*-------- NTP code ----------*/

String digitalClockDisplay() {
  String timeStr = "";
  // digital clock display of the time
  timeStr += hour();
  timeStr += printDigits(minute());
  timeStr += printDigits(second());
  timeStr += " ";
  timeStr += day();
  timeStr += " ";
  timeStr += month();
  timeStr += " ";
  timeStr += year(); 
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
