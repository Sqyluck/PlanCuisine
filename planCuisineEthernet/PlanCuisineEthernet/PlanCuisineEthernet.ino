
#include <SharpIR.h>
#include <string.h>
#include <Ethernet.h>

#define IR_PIN A0
#define RELAY_UP 4 // Up yellow brown
#define RELAY_DOWN 7 // Down violet red
#define IR_MODEL 20150
#define UID_MAX_LENGTH 15
#define HEIGHT_TOLERANCE 1


//ethernet:
String readString = String(100); //string for fetching data from address

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 177);
EthernetServer server(80);


int ir_pin = A0;
SharpIR sharp(IR_PIN, 30, 40, IR_MODEL);
int height=-1;
// -------

boolean checkMove=false;
boolean relayActivated=false;
boolean goingUp=true;
// -------

void setup(void) {
  //ethernet
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  

  // SHARP IR
  pinMode(IR_PIN, INPUT);
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


void razRelay() {
  digitalWrite(RELAY_DOWN, HIGH);
  digitalWrite(RELAY_UP, HIGH);
}


void goUp() {
  razRelay();
  digitalWrite(RELAY_DOWN, LOW);
  digitalWrite(RELAY_UP, HIGH);
  Serial.println("Moving up");
  delay(1000);
}

void goDown() {
  razRelay();
  digitalWrite(RELAY_DOWN, HIGH);
  digitalWrite(RELAY_UP, LOW);
  Serial.println("Moving down");
  delay(1000);
}

void onMove() {
  if(goingUp) {
    goUp();
  }else{
    goDown();
  }
}

void readClient() {
  EthernetClient client = server.available();
  if (client) {
    Serial.println(".");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        //Serial.write(c);
        readString += c;
        //Serial.println(readString);
        if (c == '\n' && currentLineIsBlank) {
          //Serial.println(readString);
          if(readString.indexOf("http://192.168.1.177/up") > 0){
            checkMove = true;
            //relayActivated = true;
            goingUp = true;
            Serial.println("up");
          }else if(readString.indexOf("http://192.168.1.177/down") > 0){
            checkMove = true;
            //relayActivated = true;
            goingUp = false;
            Serial.println("down");
          }else if(readString.indexOf("http://192.168.1.177/nop")){
            razRelay();
            checkMove = false;
          }
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          //client.println("Refresh: 2");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<head>");
          client.println("<link rel=\"stylesheet\" href=\"https://unpkg.com/purecss@0.6.2/build/pure-min.css\">");
          client.println("</head>");
          client.println("<html>");
          client.println("<a class=\"pure-button\" href=\"http://192.168.1.177/up\">Monter</a>");
          client.println("<a class=\"pure-button\" href=\"http://192.168.1.177/down\">Descendre</a>");
          client.println("<a class=\"pure-button\" href=\"http://192.168.1.177/nop\">stop!</a>");
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    readString = "";
    Serial.println("...");
  }
}



void loop(void) {
  readClient();

  if (checkMove) {
    onMove();
  }
}
