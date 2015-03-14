/************
 * Overview *
 ************
 This sketch is intended for the Arduino Mini found in each door controller.
 The tasks are:
   1. Illuminate the tri-color LED as appropriate
   2. Reset and read the RFID chip
   3. Send power to the door lock as appropriate
   
 Simple schematic of overall system.  This sketch is for the "[Inside]" portion of the diagram.
 
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
int I2C = 9;

// Color Modes
int COLOR_MODE_RED = 1;
int COLOR_MODE_GREEN = 2;
int COLOR_MODE_BLUE = 3;
int COLOR_MODE_PURPLE = 4;
int COLOR_MODE_TEAL = 5;
int COLOR_MODE_ORANGE = 6;
int COLOR_MODE_WHITE = 7;
int COLOR_MODE_OFF = 0;

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

String VALID_CARD = "710024FB329C";

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

  // Start the library, pass in the data details and the name of the serial port. 
  // Can be Serial, Serial1, Serial2, etc. 
  ET.begin(details(transferData), &Wire);

  // Define handler function on receiving data
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
  String tagString = "";
  char val = 0; // variable to store the data from the serial port

  // Check to see if a tag needs to be checked
  while (Serial.available() > 0) {
    val = Serial.read();
    tagString = tagString + val;
    Serial.print(val);
  }
  
  if (tagString != "") { // Tag data read
    tagString.replace("\n", "");
    tagString = tagString.substring(1, tagString.length() - 1);

    transferData.id = ID;
    transferData.i2c = I2C;
    transferData.tag = tagString;
    transferData.accessGranted = false;
 
    // Check if access is allowed
    ET.sendData(I2C);

    //reset the RFID reader
    //digitalWrite(RFIDResetPin, LOW);
    //digitalWrite(RFIDResetPin, HIGH);
  }
  
  // Check and see if a data packet has come in. 
  if (ET.receiveData()) {
    if (transferData.accessGranted) {
      openDoor();
    } else {
      accessDenied();
    }
  }
  
  // A short delay...
  delay(150);
}

// Placeholder for EasyTransfer struct response
void receive(int numBytes) {}

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
