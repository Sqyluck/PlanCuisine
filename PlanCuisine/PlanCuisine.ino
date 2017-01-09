#include <SharpIR.h>
#include <SPI.h>
#include <PN532_SPI.h>
#include "PN532.h"
#include <string.h>

// pin capteur infrarouge
#define IR_PIN A0
#define IR_MODEL 20150
// pin pour faire monter la table
#define RELAY_UP 2 // Up
// pin pour faire descendre la table
#define RELAY_DOWN 4 // Down
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
char authorizedIDs[][15] = { // Fabien, Cédric, Walid, Nacim
  "e40657ec", "6a2cd90b", "6a6dde0b", "8a91d90b"
};
int associatedHeight[5] = {
  80, 73, 73, 80
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
  digitalWrite(RELAY_UP, HIGH);
  digitalWrite(RELAY_DOWN, HIGH);
  // -------

  Serial.println("SETUP");
}

void readHeight(void) {

  Serial.print("Current height : ");
  Serial.print(height);
  Serial.println("cm");

  height=sharp.distance();

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

// lecture badge
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

//Hauteur associee badge
int getAssociatedHeight(char* uidString) {
  int i;
  for (i = 0; i<sizeof(authorizedIDs)/15; i++) {
    if (!strncmp(uidString, authorizedIDs[i], strlen(uidString))) {
      return associatedHeight[i];
    }
  }
  return -1;
}

//affiche le badge
void printUid(void) {
  Serial.print("UID : ");
  Serial.println(uidString);
}

//affiche l'Hauteur cible
void printTargetHeight(boolean found) {
  if (found) {
    Serial.print("Target height : ");
    Serial.print(targetHeight);
    Serial.println("cm");
  } else {
    Serial.println("Target height not found");
  }
}

//affiche le mouvement actuel
void printMove() {
  if (goingUp) {
    Serial.print("Moving up ");
  } else {
    Serial.print("Moving down ");
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

void onNewUid() {
  printUid();
  targetHeight=getAssociatedHeight(uidString);
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


// boucle principale
void loop(void) {
  //Lire l'id du badge, si badge il y a
  readUid();
  //Lire la taille
  readHeight();

  if (newUid) {
    onNewUid();
  }

  if (checkMove) {
    onMove();
  }
}

