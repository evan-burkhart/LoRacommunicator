
#include <Wire.h>
#include <SSD1306Wire.h> // install the ESP8266 oled driver for SSD1306 
// by Daniel Eichorn and Fabrice Weinberg t


#include <SPI.h>
#include <LoRa.h>    // this is the one by Sandeep Mistry


// display descriptor
SSD1306Wire display(0x3c, 21, 22);

// definitions
//SPI defs for LoRa radio
#define SS 18
#define RST 14
#define DI0 26
// #define BAND 915mhz bc stupid FCC

// LoRa Settings
#define BAND 915000000.00
#define spreadingFactor 9
// #define SignalBandwidth 62.5E3
#define SignalBandwidth 31.25E3
#define codingRateDenominator 8
#define preambleLength 8

// we also need the following config data:
// GPIO5 — SX1278’s SCK
// GPIO19 — SX1278’s MISO
// GPIO27 — SX1278’s MOSI
// GPIO18 — SX1278’s CS
// GPIO14 — SX1278’s RESET
// GPIO26 — SX1278’s IRQ(Interrupt Request)

// misc vars
String msg;
String displayName;
String sendMsg;
char chr;
int i = 0;

// Helpers func.s for LoRa
String mac2str(int mac)
{
  String s;
  byte *arr = (byte*) &mac;
  for (byte i = 0; i < 6; ++i)
  {
    char buff[3];
    // yea, this is a sprintf... fite me...
    sprintf(buff, "%2X", arr[i]);
    s += buff;
    if (i < 5) s += ':';
  }
  return s;
}

// set up intial for LoRa
void setup() {
  // reset the screen
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW); // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(16, HIGH);
  Serial.begin(115200);
  while (!Serial); //All actions will be through serial atm.

  // Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(5, 5, "LoRa Chat Node");
  display.display();

  SPI.begin(5, 19, 27, 18);
  LoRa.setPins(SS, RST, DI0);
  Serial.println("LoRa Chat Node");
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
    //********you can uncomment this if you are a nerd and like to see things.
  //Serial.print("LoRa Spreading Factor: ");
  //Serial.println(spreadingFactor);
  LoRa.setSpreadingFactor(spreadingFactor);

  //Serial.print("LoRa Signal Bandwidth: ");
  //Serial.println(SignalBandwidth);
  
  LoRa.setSignalBandwidth(SignalBandwidth);

  LoRa.setCodingRate4(codingRateDenominator);

  LoRa.setPreambleLength(preambleLength);

  Serial.println("LoRa Initial OK!");
  display.drawString(5, 20, "LoRaSMS is running!");
  display.display();
  delay(2000);
  Serial.println("Welcome to LoRaSMS!");
  // get MAC as initial nickname or be cool and say callsign *cool guy with glasses emoji*
  int MAC = ESP.getEfuseMac();
  displayName = mac2str(MAC);
  //displayName.trim(); // remove the newline
  Serial.println("TO CHANGE ANTENNA; TURN OFF DEVICE, CAHNGE ANTENNA, THEN TURN DEVICE BACK ON.");
  Serial.println("****Use /n to set callsign****");
  Serial.print("Initial callsign is "); Serial.println(displayName);
  Serial.println("Use /? to list all commands");
  Serial.println(": ");
  display.clear();
  display.drawString(1,0, "Use /n to set callsign");
  display.drawString(5, 20, "Callsign set:");
  display.drawString(20, 30, displayName);
  display.display();
  delay(1000);
}

void loop() {
  // Receive a message first...
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    display.clear();
    display.drawString(3, 0, "Received Message!");
    display.display();
    while (LoRa.available()) {
      String data = LoRa.readString();
      display.drawString(0, 22, data);
      display.display();
      Serial.println(data);
    }
  } // start toread bytes from Serial
  if (Serial.available()) {
    chr = Serial.read();
    Serial.print(chr); // prints the nickname
    if (chr == '\n' || chr == '\r') {
      msg += chr; //msg+='\0'; // i have no idea what i am doing, it should termniate the string.
      if (msg.startsWith("/")) {
        // clean up msg string...
        msg.trim(); msg.remove(0, 1);
        // process command...
        char cmd[1]; msg.substring(0, 1).toCharArray(cmd, 2);
        switch (cmd[0]){
          case '?':
            Serial.println("Supported Commands:");
            Serial.println("h - this message...");
            Serial.println("n - change Tx nickname...");
            Serial.println("d - print Tx nickname...");
            break;
          case 'n':
            displayName = msg.substring(2);
            Serial.print("Your callsign set to: "); Serial.println(displayName);
            break;
          case 'd':
            Serial.print("Your callsign is: "); Serial.println(displayName);
            break;
          default:
            display.drawString(5, 20, "LoRaSMS is running!");
            Serial.println("ill add a gps cords later");
        }
        msg = "";
      }
      else {
        // hope its working for the love of god.
        Serial.print("Me: "); Serial.println(msg);
        // assemble message
        sendMsg += displayName;
        sendMsg += "> ";
        sendMsg += msg;
        // send message
        LoRa.beginPacket();
        LoRa.print(sendMsg);
        LoRa.endPacket();
        display.clear();
        display.drawString(1, 0, sendMsg);
        display.display();
        // clear the strings and start the shit show again.
        msg = "";
        sendMsg = "";
        Serial.print(": ");
      }
    }
    else {
      msg += chr;
    }
  }
}
