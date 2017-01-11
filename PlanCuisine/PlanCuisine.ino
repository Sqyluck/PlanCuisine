#include <SharpIR.h>
#include <SPI.h>
#include <PN532_SPI.h>
#include "PN532.h"
#include <string.h>

#define IR_PIN A0
#define RELAY_UP 4 // Up yellow brown
#define RELAY_DOWN 7 // Down violet red
#define IR_MODEL 20150
#define UID_MAX_LENGTH 15
#define HEIGHT_TOLERANCE 1

// SHARP IR
// first parameter : the pin where your sensor is attached
// second parameter : the number of readings the library will make before calculating a mean distance
// third parameter : the difference between two consecutive measurements to be taken as valid
// model : an int that determines your sensor : 1080 for GP2Y0A21Y
//                                              20150 for GP2Y0A02Y
//                                              (working distance range according to the datasheets)
SharpIR sharp(IR_PIN, 30, 40, IR_MODEL);
int height=-1;
// -------

// NFC SHIELD
PN532_SPI pn532spi(SPI, 10);
PN532 nfc(pn532spi);
uint8_t uid[]={ 0, 0, 0, 0, 0, 0, 0 };
uint8_t uidLength=0;
char uidString[UID_MAX_LENGTH]="";
char authorizedIDs[][15] = { // Fabien, Cédric, Walid, Nacim, Luc, PA, Alan, Arthur, Gaetane
  "e40657ec", "6a2cd90b", "6a6dde0b", "261c589c", "543257ec", "5adbda0b", "543357ec", "34a456ec", "245156ec"
};
int associatedHeight[9] = {
  73, 73, 73, 73, 70, 67, 73, 67, 60
};
int targetHeight=-1;
boolean newUid=false;
boolean checkMove=false;
boolean relayActivated=false;
boolean goingUp=true;
// -------

void setup(void) {
  Serial.begin(9600);
  
  // SHARP IR
  pinMode(IR_PIN, INPUT);
  // -------
  
  // NFC SHIELD
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board !");
    while(1);
  }
  nfc.SAMConfig();
  // -------
  
  
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  
  
  // RELAY CARD
  pinMode(RELAY_UP, OUTPUT);
  pinMode(RELAY_DOWN, OUTPUT);
  // -------  
  digitalWrite(RELAY_UP, LOW);
  digitalWrite(RELAY_DOWN, HIGH);
  delay(3000);
  digitalWrite(RELAY_UP, HIGH);
  digitalWrite(RELAY_DOWN, LOW);
  delay(3000);
  digitalWrite(RELAY_UP, HIGH);
  digitalWrite(RELAY_DOWN, HIGH);

Serial.println("SETUP");
}

void readHeight(void) {
  
  Serial.print("Current height : ");
  Serial.print(height);
  Serial.println("cm");
  unsigned int averageHeight=0;
  for(unsigned int i=0; i<5; i++) {
    averageHeight += sharp.distance();
  }
  averageHeight /= 5;
  height = averageHeight;
}

void convertUIDtoString(void) {
  if (!uidLength) {
    return;
  }
  
  memset(uidString, '\0', UID_MAX_LENGTH);
  int i;
  for (int i=0; i < uidLength; i++) {
    sprintf(&uidString[2 * i], "%02x", uid[0 + i]);
  }
}

void readUid(void) {
  int uidLengthSav=uidLength;
  char uidStringSav[UID_MAX_LENGTH]="";
  strncpy(uidStringSav, uidString, uidLength);
  
  uidLength=0;
  nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  if (!uidLength) {
    newUid=false;
    return;
  }
  convertUIDtoString();
  
  if (uidLength!=uidLengthSav || strncmp(uidString, uidStringSav, uidLength)) {
    Serial.println("New UID read");
    newUid=true;
  }
  
  free(uidStringSav);
}

int getAssociatedHeight(char* uidString) {
  int i;
  for (i = 0; i<sizeof(authorizedIDs)/15; i++) {
    if (!strncmp(uidString, authorizedIDs[i], strlen(uidString))) {
      return associatedHeight[i];
    }
  }
  return -1;
}

void printUid(void) {
  Serial.print("UID : ");
  Serial.println(uidString);
}

void printTargetHeight(boolean found) {
  if (found) {
    Serial.print("Target height : ");
    Serial.print(targetHeight);
    Serial.println("cm");
  } else {
    Serial.println("Target height not found");
  }
}

void printMove() {
  if (goingUp) {
    Serial.print("Moving up ");
    digitalWrite(RELAY_DOWN, LOW);
    digitalWrite(RELAY_UP, HIGH);
    delay(1000);
  } else {
    Serial.print("Moving down ");
    digitalWrite(RELAY_DOWN, HIGH);
    digitalWrite(RELAY_UP, LOW);
    delay(1000);
  }
  Serial.print(height);
  Serial.print("/");
  Serial.println(targetHeight);
}

void razRelay() {
  digitalWrite(RELAY_DOWN, HIGH);
  digitalWrite(RELAY_UP, HIGH);
}

/*
void onMove() {
  if (targetHeight>height+HEIGHT_TOLERANCE) {
    if (!relayActivated || (relayActivated && !goingUp)) {
      relayActivated=true;
      goingUp=true;
      razRelay();
      digitalWrite(RELAY_DOWN, HIGH);
      digitalWrite(RELAY_UP, LOW);
    }
    printMove();
  } else if (targetHeight<height-HEIGHT_TOLERANCE) {
    if (!relayActivated || (relayActivated && goingUp)) {
      relayActivated=true;
      goingUp=false;
      razRelay();
      digitalWrite(RELAY_UP, HIGH);
      digitalWrite(RELAY_DOWN, LOW);
    }
    printMove();
  }
  else {
    razRelay();
    relayActivated=false;
    checkMove=false;
    Serial.println("Stop moving");
  }
}
*/

void onMove() {
  if (targetHeight>height+HEIGHT_TOLERANCE) {
    if (!relayActivated || (relayActivated && !goingUp)) {
      if (relayActivated && !goingUp)
      relayActivated=true;
      goingUp=true;
      razRelay();
    }
    printMove();
  } else if (targetHeight<height-HEIGHT_TOLERANCE) {
    if (!relayActivated || (relayActivated && goingUp)) {
      relayActivated=true;
      goingUp=false;
      razRelay();
    }
    printMove();
  }
  else {
    razRelay();
    relayActivated=false;
    checkMove=false;
    Serial.println("Stop moving");
  }
}

void onNewUid() {
  printUid();
  targetHeight=getAssociatedHeight(uidString);
  Serial.println(targetHeight);
  if (targetHeight!=-1) {
    razRelay();
    printTargetHeight(true);
    checkMove=true; // Move the workbench
  } else {
    printTargetHeight(false);
    checkMove=false; // Move the workbench
  }
  newUid=false;
}

void loop(void) {
  readUid();
  
  readHeight();
  
  if (newUid) {
    onNewUid();
  }
  
  if (checkMove) {
    onMove();
  }
}
