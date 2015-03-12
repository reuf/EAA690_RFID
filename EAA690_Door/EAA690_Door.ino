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

/********************
 * GLOBAL VARIABLES *
 ********************/
int RFIDResetPin = 8;

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
  
  Serial.begin(9600);  // set up Serial library at 9600 bps
}

/************************************************
 * Main Loop                                    *
 *                                              *
 * This is the 2nd required functions for all   *
 * arduinos.  This function will be called      *
 * repeatedly until the arduino is powered off  *
 ************************************************/
void loop() {
  // Check to see if a tag needs to be checked
  String tagString = "";
  int index = 0;
  boolean reading = false;

  // Reads the RFID from the card scanned
  while (Serial.available()) {
    int readByte = Serial.read(); //read next available byte
    if(readByte == 2) reading = true; //begining of tag
    if(readByte == 3) reading = false; //end of tag
    if(reading && readByte != 2 && readByte != 10 && readByte != 13){
      tagString = tagString + readByte;
      index ++;
    }
  }
  if (tagString == "") { // No tag data read
    delay(1000); // Delay 1 second
  } else {
    Serial.println(tagString); // Check if access is allowed

    //reset the RFID reader
    digitalWrite(RFIDResetPin, LOW);
    digitalWrite(RFIDResetPin, HIGH);
    delay(150);
  }
}
