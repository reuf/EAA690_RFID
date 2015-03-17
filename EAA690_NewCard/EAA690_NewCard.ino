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

int RFIDResetPin = 8;

void setup() {
  // Setup Arduino PINs
  pinMode(RFIDResetPin, OUTPUT);
  
  // Initialize the RFID reader for use
  digitalWrite(RFIDResetPin, HIGH);
}

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

    //reset the RFID reader
    //digitalWrite(RFIDResetPin, LOW);
    //digitalWrite(RFIDResetPin, HIGH);
  }
  
  // A short delay...
  delay(150);
}
