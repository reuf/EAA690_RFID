// Include libraries
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <Time.h>

/********************
 * GLOBAL VARIABLES *
 ********************/

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

// Temperature
float sensor = 0;
float voltage = 0;
int celsius = 0;
int fahrenheit = 0;

// LCD
SoftwareSerial mySerial = SoftwareSerial(255, 3);

// SD Card
boolean sdEnabled = true;
char dbDelimiter = ',';
String location = "hangar";

char server[] = "www.brianmichael.org";
String validcard = "554948485052706651505767";

// Arduino PINs
int TEMP = 0;
int LOCK = 2;
int RFIDResetPin = 13;
int GREEN = 8;
int BLUE = 6;
int RED = 5;

// NTP (time)
unsigned long lastCheck = 0;
unsigned long epoch = 0;
long checkDelay = 600000;
unsigned int localPort = 2390; // local port to listen for UDP packets
IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE];
WiFiUDP Udp;

// Debug
boolean debug = false;

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
  mySerial.begin(9600); // Init the LCD
  
  // Setup Arduino PINs
  pinMode(10, OUTPUT); // SD SS pin
  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);
  pinMode(RFIDResetPin, OUTPUT);
  pinMode(LOCK, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(RED, OUTPUT);
  pinMode(BLUE, OUTPUT);
  digitalWrite(BLUE, HIGH);
  digitalWrite(RFIDResetPin, HIGH);
  
  if (debug) displayLCD("Starting...", 2000, true);

  // Init the network
  initNetwork();
  
  // Check the SD card
  if (!SD.begin(4)) {
    sdEnabled = false;
  }
}

/************************************************
 * Main Loop                                    *
 *                                              *
 * This is the 2nd required functions for all   *
 * arduinos.  This function will be called      *
 * repeatedly until the arduino is powered off  *
 ************************************************/
void loop() {
  if (debug) displayLCD("Begin loop", 50, false);
    
  // Get the data once per hour, at the top of the hour
  unsigned long nowLong = now();
  if ((nowLong % 86400) / 60 == 0) {
    getUserData();
  }

  // Check to see if a tag needs to be checked
  String tagString = readTag();
  if (tagString == "") {
    String timeStr = getTimeAsString();
    String msg = "Waiting...      " + timeStr;
    Serial.println(msg);
    displayLCD(msg, 2000, false);
  } else {
    if (checkTag(tagString)) { //Check if it is a match
      openDoor(tagString);
    } else {
      accessDenied(tagString);
    }
    resetReader(); //reset the RFID reader
  }
}

/**
 * Initialize the network so user and NTP data 
 * can be transmitted/received
 */
void initNetwork() {
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    if (debug) displayLCD("WiFi shield not present", 1, false); 
    // don't continue:
    wifiEnabled = false;
  } 
  
  // attempt to connect to Wifi network:
  if (wifiEnabled) {
    while (status != WL_CONNECTED) { 
      String msg = "Connecting to   ";
      msg.concat(ssid);
      msg.concat('\0'); // Terminate string, may not be needed, but doesn't hurt
      if (debug) displayLCD(msg, 1, true);
      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:    
      if (passRequired) {
        status = WiFi.begin(ssid, pass);
      } else {
        if (debug) displayLCD("Not passing password", 1, true);
        status = WiFi.begin(ssid);
      }
  
      // wait 1 second for connection:
      delay(1000);
    } 
    if (debug) displayLCD("Connected to    wifi", 2000, true);    
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
    displayLCD("Retrieving user data...", 2000, true);
    File dbFile = SD.open("database.csv", FILE_WRITE);
    while (client.available()) { 
      char c = client.read();
      //Serial.print(c);
      dbFile.print(c);
    }
    dbFile.close();
    client.stop();
  } else {
    // if you didn't get a connection to the server:
    displayLCD("connection failed", 5000, true);
  }
}

/**
 * Reads the RFID from the card scanned
 */
String readTag() {
  String tagString;
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
  return tagString;
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
    displayLCD("Updating time   from NTP...", 1000, false);
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
boolean checkTag(String tag) {
  boolean valid = false;
  if (tag == "") return false; //empty, no need to contunue
  if (tag == validcard) valid = true; // Override this particular card

  String record = getRecord(tag);
  String date = getValue(record, dbDelimiter,  3);
  String access = getValue(record, dbDelimiter,  4);
  if (record && access == location && checkDate(date)) {
    valid = true;
  }
  return valid;
}

/**
 * Retrieves the user entry from the SD card for the tag ID provided
 */
String getRecord(String tag) {
  String returnLine;
  if (sdEnabled) {
    if (SD.exists("database.csv")) {
      File dbFile = SD.open("database.csv");
      String line = ""; // = "554948485052706651505767|Brian|Michael|01 JAN 2015|hangar|Hello";
      int i = 0;
      while (dbFile.available()) {
        char c = dbFile.read();
        if (c == '\n') {
          String id = getValue(line, dbDelimiter,  0);
          //String firstName = getValue(line, dbDelimiter,  1);
          //String lastName = getValue(line, dbDelimiter,  2);
          //String date = getValue(line, dbDelimiter,  3);
          //String access = getValue(line, dbDelimiter,  4);
          //String message = getValue(line, dbDelimiter,  5);
          //Serial.println("id=["+id+"]; firstName=["+firstName+"]; lastName=["+lastName+"]; date=["+date+"]; access=["+access+"]; message=["+message+"]");
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

/**
 * Compares dates to determine access validity
 */
boolean checkDate(String dateToCheck) {
  boolean ok = true;
  // TODO
  return ok;
}

/**
 * Reset the RFID reader to read again.
 */
void resetReader(){
  digitalWrite(RFIDResetPin, LOW);
  digitalWrite(RFIDResetPin, HIGH);
  delay(150);
}

/**
 * Handles the "access denied" state
 */
void accessDenied(String id) {
  String record = getRecord(id);
  String firstName = getValue(record, dbDelimiter, 1);
  String lastName = getValue(record, dbDelimiter, 2);
  String message = getValue(record, dbDelimiter, 5);
  //Serial.print("Access Denied for ");
  //Serial.print(firstName);
  //Serial.print(" ");
  //Serial.print(lastName);
  //Serial.print(" ");
  //Serial.println(message);
  digitalWrite(BLUE, LOW);
  digitalWrite(RED, HIGH);
  displayLCD(message, 5000, true);
  digitalWrite(RED, LOW);
  digitalWrite(BLUE, HIGH);
}

/**
 * Handles the "access granted" state
 */
void openDoor(String id) {
  String record = getRecord(id);
  String firstName = getValue(record, dbDelimiter, 1);
  String lastName = getValue(record, dbDelimiter, 2);
  String message = getValue(record, dbDelimiter, 5);
  //Serial.print("Opening door for ");
  //Serial.print(firstName);
  //Serial.print(" ");
  //Serial.println(lastName);
  digitalWrite(BLUE, LOW);
  digitalWrite(GREEN, HIGH);
  digitalWrite(LOCK, HIGH);
  displayLCD(message, 5000, true);
  //Serial.println("Closing door");
  displayLCD("Closing door", 5000, true);
  digitalWrite(LOCK, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, HIGH);
}

/**
 */
int celsiusToFahrenheit(int celsius) {
  return ((celsius * 9) / 5) + 32;
}

 
/**
 * Reads the temperature from the TMP35
 * @see http://arduino.cc/documents/datasheets/TEMP-TMP35_36_37.pdf
 */
void getTemps() {
  sensor = analogRead(TEMP);
  voltage = ((sensor * 5000)/1024) - 500;
  celsius = voltage / 10;
  fahrenheit = celsiusToFahrenheit(celsius);
}

 
/**
 * Prints a message to the LCD screen for a specified length of time
 * Effectively a log method
 */
void displayLCD(String msg, int d, boolean backlight) {
  mySerial.write(12);    // Clear             
  if (backlight) {
    mySerial.write(17);  // Turn backlight on
  }
  mySerial.print(msg);   // message
  delay(d);              // Wait "d" seconds
  if (backlight) {
    mySerial.write(18);  // Turn backlight off
  }
}

void setLCDMode(int mode) {
  // RED
  if (mode == 1) {
    digitalWrite(RED, HIGH);
    digitalWrite(GREEN, LOW);
    digitalWrite(BLUE, LOW);
  }

  // GREEN
  if (mode == 2) {
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, HIGH);
    digitalWrite(BLUE, LOW);
  }

  // BLUE
  if (mode == 3) {
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, LOW);
    digitalWrite(BLUE, HIGH);
  }

  // PURPLE (RED+BLUE)
  if (mode == 4) {
    analogWrite(RED, 127);
    analogWrite(GREEN, 0);
    analogWrite(BLUE, 127);
  }

  // TEAL (BLUE+GREEN)
  if (mode == 5) {
    analogWrite(RED, 0);
    analogWrite(GREEN, 127);
    analogWrite(BLUE, 127);
  }

  // ORANGE (GREEN+RED)
  if (mode == 6) {
    analogWrite(RED, 127);
    analogWrite(GREEN, 127);
    analogWrite(BLUE, 0);
  }

  // WHITE (RED+GREEN+BLUE)
  if (mode == 7) {
    analogWrite(RED, 85);
    analogWrite(GREEN, 85);
    analogWrite(BLUE, 85);
  }

  // OFF
  if (mode == 0) {
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, LOW);
    digitalWrite(BLUE, LOW);
  }
}
