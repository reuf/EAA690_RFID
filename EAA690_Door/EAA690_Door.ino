/************
 * Overview *
 ************
 This sketch is intended for the Arduino Mini found in each door controller.
 The tasks are:
   1. Illuminate the tri-color LED as appropriate
   2. Reset and read the RFID chip
   3. Send power to the door lock as appropriate
   
 Simple schematic of overall system.  This sketch is for the "[Door Controller]" portion of the diagram.
 
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
         ||||  [Door Controller]      [New Card Assignment]
                                     +------------------+
         ////                        | +--------------+ |
                                     | | Arduino MINI | |
         ||||  [Main Controller]     | +--------------+ |
   +----------------------+          | +------+         |
   |   | RJ45 |  +------+ |          | | RFID |         |
   |   +------+  | RFID | |          | +------+         |
   |    LM7805   +------+ |          | +--------------+ |
   | +-------------+----+ |          | | Raspberry Pi | |
   | | Arduino UNO | SD | |          | +--------------+ |
   | +-------------+----+ |          +------------------+
   | | Ethernet    |      |
   +----------------------+         
         ||||
*/

#include <Wire.h>
#include <EasyTransferI2C.h>

/********************
 * GLOBAL VARIABLES *
 ********************/
int ID = 1;
int LOCK = 2;
int RED = 5;
int GREEN = 6;
int BLUE = 7;
int RFIDResetPin = 8;
int I2C = 2;

// Color Modes
int COLOR_MODE_RED = 1;
int COLOR_MODE_GREEN = 2;
int COLOR_MODE_BLUE = 3;
int COLOR_MODE_PURPLE = 4;
int COLOR_MODE_TEAL = 5;
int COLOR_MODE_ORANGE = 6;
int COLOR_MODE_WHITE = 7;
int COLOR_MODE_OFF = 0;

EasyTransferI2C ET;

struct DATA_STRUCTURE {
  int access = 0;
  int door = I2C;
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

String tag = "";

/************************************************
 * Setup                                        *
 *                                              *
 * This is 1 of 2 required functions for all    *
 * arduinos.  It is called upon startup (once)  *
 * and initializes any variables and settings   *
 ************************************************/
void setup() {
  // Setup Arduino PINs
  pinMode(RFIDResetPin, OUTPUT);
  
  // Initialize the RFID reader for use
  digitalWrite(RFIDResetPin, HIGH);
  
  Wire.begin(I2C);
  ET.begin(details(tagData), &Wire);
  Wire.onReceive(receive);

  Serial.begin(9600);  // set up Serial library at 9600 bps
  
  setLEDMode(COLOR_MODE_OFF);
}

/************************************************
 * Main Loop                                    *
 *                                              *
 * This is the 2nd required functions for all   *
 * arduinos.  This function will be called      *
 * repeatedly until the arduino is powered off  *
 ************************************************/
void loop() {    
  char val = 0; // variable to store the data from the serial port

  int count = 0;
  // Check to see if a tag needs to be checked
  if (tag == "") {
    while (Serial.available() > 0) {
      val = Serial.read();
      if (count == 1) tagData.char1 = val;
      if (count == 2) tagData.char2 = val;
      if (count == 3) tagData.char3 = val;
      if (count == 4) tagData.char4 = val;
      if (count == 5) tagData.char5 = val;
      if (count == 6) tagData.char6 = val;
      if (count == 7) tagData.char7 = val;
      if (count == 8) tagData.char8 = val;
      if (count == 9) tagData.char9 = val;
      if (count == 10) tagData.char10 = val;
      if (count == 11) tagData.char11 = val;
      if (count == 11) tagData.char12 = val;
      if (count > 0) tag += val;
      count++;
    }
    if (tag != "") {
      //tagData.tag.replace("\n", "");
      //tagData.tag = tagData.tag.substring(1, tagData.tag.length() - 1);
      //Serial.println("Read tag " + tagData.tag);
      ET.sendData(0);
    }
  }
  
  if (ET.receiveData()) {
    if (tagData.access > 0) {
      openDoor();
    } else {
      accessDenied();
    }
    clearTagData();
  }

  //reset the RFID reader
  digitalWrite(RFIDResetPin, LOW);
  digitalWrite(RFIDResetPin, HIGH);
    
  // A short delay...
  delay(150);
}

void clearTagData() {
    tag = "";
    tagData.access = 0;
    tagData.char1 = '\0';
    tagData.char2 = '\0';
    tagData.char3 = '\0';
    tagData.char4 = '\0';
    tagData.char5 = '\0';
    tagData.char6 = '\0';
    tagData.char7 = '\0';
    tagData.char8 = '\0';
    tagData.char9 = '\0';
    tagData.char10 = '\0';
    tagData.char11 = '\0';
    tagData.char12 = '\0';
}

// Called by Main Controller to let us know if access is granted.
void receive(int howMany) {}

/**
 * Handles the "access denied" state
 */
void accessDenied() {
  setLEDMode(COLOR_MODE_RED);
  delay(5000);
  setLEDMode(COLOR_MODE_OFF);
}

/**
 * Handles the "access granted" state
 */
void openDoor() {
  setLEDMode(COLOR_MODE_GREEN);
  digitalWrite(LOCK, HIGH);
  delay(10000);
  digitalWrite(LOCK, LOW);
  setLEDMode(COLOR_MODE_OFF);
}

/**
 * Sets the color of the tri-color LED
 */
void setLEDMode(int mode) {
  // RED
  if (mode == COLOR_MODE_RED) {
    digitalWrite(RED, HIGH);
    digitalWrite(GREEN, LOW);
    digitalWrite(BLUE, LOW);
  }

  // GREEN
  if (mode == COLOR_MODE_GREEN) {
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, HIGH);
    digitalWrite(BLUE, LOW);
  }

  // BLUE
  if (mode == COLOR_MODE_BLUE) {
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, LOW);
    digitalWrite(BLUE, HIGH);
  }

  // PURPLE (RED+BLUE)
  if (mode == COLOR_MODE_PURPLE) {
    analogWrite(RED, 127);
    analogWrite(GREEN, 0);
    analogWrite(BLUE, 127);
  }

  // TEAL (BLUE+GREEN)
  if (mode == COLOR_MODE_TEAL) {
    analogWrite(RED, 0);
    analogWrite(GREEN, 127);
    analogWrite(BLUE, 127);
  }

  // ORANGE (GREEN+RED)
  if (mode == COLOR_MODE_ORANGE) {
    analogWrite(RED, 127);
    analogWrite(GREEN, 127);
    analogWrite(BLUE, 0);
  }

  // WHITE (RED+GREEN+BLUE)
  if (mode == COLOR_MODE_WHITE) {
    analogWrite(RED, 85);
    analogWrite(GREEN, 85);
    analogWrite(BLUE, 85);
  }

  // OFF
  if (mode == COLOR_MODE_OFF) {
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, LOW);
    digitalWrite(BLUE, LOW);
  }
}
