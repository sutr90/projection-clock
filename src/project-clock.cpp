#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ezTime.h>

//Variables
int i = 0;
int statusCode;
const char* ssid = "text";
const char* passphrase = "text";
String st;
String content;


//Function Declaration
bool testWifi(void);
void launchWeb(void);
void setupAP(void);

Timezone Prague;

bool tzSynced = false;

//Establishing Local server at port 80 whenever required
ESP8266WebServer server(80);

void setup() {
  initializeModule();
  Serial.begin(115200); //Initialising if(DEBUG)Serial Monitor
  Serial.println();
  Serial.println("Disconnecting previously connected WiFi");
  WiFi.disconnect();
  EEPROM.begin(512); //Initialasing EEPROM
  delay(10);
  Serial.println();
  Serial.println();
  Serial.println("Startup");

  //---------------------------------------- Read EEPROM for SSID and pass
  Serial.println("Reading EEPROM ssid");

  String esid;
  for (int i = 0; i < 32; ++i) {
    esid += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");

  String epass = "";
  for (int i = 32; i < 96; ++i) {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);


  WiFi.begin(esid.c_str(), epass.c_str());
  if (testWifi()) {
    Serial.println("Succesfully Connected!!!");
    return;
  } else {
    Serial.println("Turning the HotSpot On");
    launchWeb();
    setupAP();// Setup HotSpot
  }

  Serial.println();
  Serial.println("Waiting.");

  while ((WiFi.status() != WL_CONNECTED)) {
    Serial.print(".");
    delay(100);
    server.handleClient();
  }
}

void loop() {
  if ((WiFi.status() == WL_CONNECTED)) {
    while (!waitForSync(30) && !tzSynced) {
      Serial.println("Timezone Sync timeout!");
    }

    if (!tzSynced) {
      tzSynced = true;
      Prague.setLocation("Europe/Prague");
    }

    events();
    if (secondChanged()) {
      Serial.println("Prague time: " + Prague.dateTime());
      showTime();
    }


  } else {
    tzSynced = false;
  }
}

int dataPin = D2; // DATA
int clkPin = D3; // WRITE
int latchPin = D4; // CS

inline void writeBitLow() {
  digitalWrite(dataPin, LOW);
  digitalWrite(clkPin, HIGH);
  digitalWrite(clkPin, LOW);
}

inline void writeBitHigh() {
  digitalWrite(dataPin, HIGH);
  digitalWrite(clkPin, HIGH);
  digitalWrite(clkPin, LOW);
}

inline void writeBit(unsigned char bits, unsigned char index) {
  (bits & (1 << index)) == (1 << index) ? writeBitHigh() : writeBitLow();
}

void lcdShift(unsigned char address, unsigned char data) {
  digitalWrite(latchPin, LOW);

  // write header
  writeBitHigh();
  writeBitLow();
  writeBitHigh();
  // write address
  writeBit(address, 5);
  writeBit(address, 4);
  writeBit(address, 3);
  writeBit(address, 2);
  writeBit(address, 1);
  writeBit(address, 0);
  // write segment
  writeBit(data, 3);
  writeBit(data, 2);
  writeBit(data, 1);
  writeBit(data, 0);

  digitalWrite(latchPin, HIGH);
}

void initializeModule() {
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clkPin, OUTPUT);
  
  delay(100);

  //100000000010
  //100000000110
  //100001010000

  initFrame(0b000000, 0b010);
  initFrame(0b000000, 0b110);
  initFrame(0b001010, 0b000);

  delay(3);

  initFrame(0b000000, 0b010);
  initFrame(0b000000, 0b110);
  initFrame(0b001010, 0b000);

  delay(200);
}

void initFrame(unsigned char address, unsigned char data) {
  digitalWrite(latchPin, LOW);

  // write header
  writeBitHigh();
  writeBitLow();
  writeBitLow();
  // write address
  writeBit(address, 5);
  writeBit(address, 4);
  writeBit(address, 3);
  writeBit(address, 2);
  writeBit(address, 1);
  writeBit(address, 0);
  // write segment
  writeBit(data, 2);
  writeBit(data, 1);
  writeBit(data, 0);

  digitalWrite(latchPin, HIGH);
}

void clearDisplay() {
  lcdShift(0b000100, 0);
  lcdShift(0b000101, 0);
  lcdShift(0b000110, 0);
  lcdShift(0b000111, 0);
  lcdShift(0b000000, 0);
  lcdShift(0b000001, 0);
  lcdShift(0b000010, 0);
  lcdShift(0b000011, 0);
}

/*
  half0       half1        +-----+
  0: 1248     0: 14        | 0:1 |
  1: 0        1: 14      +-+-----+-+
  2: 148      2: 12      |0|     |1|
  3: 18       3: 124     |:|     |:|
  4: 2        4: 124     |2|     |1|
  5: 128      5: 24      +-+-----+-+
  6: 1248     6: 24        | 1:2 |
  7: 1        7: 14      +-+-----+-+
  8: 1248     8: 124     |0|     |1|
  9: 128      9: 124     |:|     |:|
  A: 124      A: 124     |4|     |4|
  b: 248      b: 24      +-+-----+-+
  C: 1248     C: 0         | 0:8 |
  D: 48       D: 124       +-----+
  E: 1248     E: 2
  F: 124      F: 2
  L: 248      L: 0
  o: 48       o: 24
  n: 4        n: 24
  t: 248      t: 2
*/
const byte half0[] = {15, 0, 13, 9, 2, 11, 15, 1, 15, 11, 7, 14, 15, 12, 15, 7, 0};
const byte half1[] = {5, 5, 3, 7, 7, 6, 6, 5, 7, 7, 7, 6, 0, 7, 2, 2, 0};

const byte HOURS = 0;
const byte MINUTES = 4;

void display2digits(byte value, byte unit) {
  byte ones = value % 10;
  byte tens = value / 10;

  lcdShift(0 + unit, half0[tens]);
  lcdShift(1 + unit, half1[tens]);
  lcdShift(2 + unit, half0[ones]);
  lcdShift(3 + unit, half1[ones] + (unit == HOURS ? 8 : 0));
}

void showTime()
{
    display2digits(Prague.hour(), HOURS);
    display2digits(Prague.minute(), MINUTES);
}

//-------- Fuctions used for WiFi credentials saving and connecting to it which you do not need to change
bool testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
    delay(500);
    Serial.print("*");
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void launchWeb() {
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
  Serial.println("Server started");
}

void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }

  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i) {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);

    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }

  st += "</ol>";
  delay(100);
  WiFi.softAP("how2electronics", "");
  Serial.println("softap");
  launchWeb();
  Serial.println("over");
}

void createWebServer() {
  { server.on("/", []() {

      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
      content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
      content += ipStr;
      content += "<p>";
      content += st;
      content += "</p><form method='post' action='setting'><label>SSID: </label><input name='ssid' length=32><label>Password: </label><input name='pass' type='password' length=64><input type='submit'></form>";
      content += "</html>";
      server.send(200, "text/html", content);

    });

    server.on("/scan", []() {
      //setupAP();
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

      content = "<!DOCTYPE HTML>\r\n<html>go back";
      server.send(200, "text/html", content);

    });

    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");

        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i) {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i) {
          EEPROM.write(32 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        EEPROM.commit();

        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        ESP.reset();

      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");

      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);


    });
  }
}
