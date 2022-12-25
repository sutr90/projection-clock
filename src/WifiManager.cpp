#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "../include/WifiManager.hpp"

WifiManager::WifiManager(uint8_t resetPin, uint8_t webport) : _resetPin{resetPin}, server{webport}
{
}

void WifiManager::initialize()
{
    Serial.println();
    Serial.println("Disconnecting previously connected WiFi");
    WiFi.disconnect();

    pinMode(_resetPin, INPUT_PULLUP);
    int sensorVal = digitalRead(_resetPin);
    Serial.println();
    Serial.print("button value ");
    Serial.println(sensorVal);

    if (sensorVal == HIGH)
    {
        EEPROM.begin(512); // Initialasing EEPROM
        delay(10);
        Serial.println();
        Serial.println();
        Serial.println("Startup");

        //---------------------------------------- Read EEPROM for SSID and pass
        Serial.println("Reading EEPROM ssid");

        String esid;
        for (uint8_t i = 0; i < 32; ++i)
        {
            esid += char(EEPROM.read(i));
        }
        Serial.println();
        Serial.print("SSID: ");
        Serial.println(esid);
        Serial.println("Reading EEPROM pass");

        String epass = "";
        for (uint8_t i = 32; i < 96; ++i)
        {
            epass += char(EEPROM.read(i));
        }
        Serial.print("PASS: ");
        Serial.println(epass);

        WiFi.begin(esid.c_str(), epass.c_str());

        if (testWifi())
        {
            Serial.println("Succesfully Connected!!!");
            return;
        }
    }
    Serial.println("Turning the HotSpot On");
    launchWeb();
    setupAP(); // Setup HotSpot

    Serial.println();
    Serial.println("Waiting.");

    while ((WiFi.status() != WL_CONNECTED))
    {
        Serial.print(".");
        delay(100);
        server.handleClient();
    }
}

bool WifiManager::testWifi(void)
{
    int c = 0;
    Serial.println("Waiting for Wifi to connect");
    while (c < 20)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
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

void WifiManager::launchWeb()
{
    Serial.println("");
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("WiFi connected");
    }
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("SoftAP IP: ");
    Serial.println(WiFi.softAPIP());
    createWebServer();
    // Start the server
    server.begin();
    Serial.println("Server started");
}

void WifiManager::setupAP(void)
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0)
    {
        Serial.println("no networks found");
    }
    else
    {
        Serial.print(n);
        Serial.println(" networks found");
        for (uint8_t i = 0; i < n; ++i)
        {
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
    for (uint8_t i = 0; i < n; ++i)
    {
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
    WiFi.softAP("ProjectorClock", "");
    Serial.println("softap");
    launchWeb();
    Serial.println("over");
}

void WifiManager::createWebServer()
{
    {
        server.on("/", [&]()
                  {
                      IPAddress ip = WiFi.softAPIP();
                      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
                      content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
                      content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
                      content += ipStr;
                      content += "<p>";
                      content += st;
                      content += "</p><form method='post' action='setting'><label>SSID: </label><input name='ssid' length=32><label>Password: </label><input name='pass' type='password' length=64><input type='submit'></form>";
                      content += "</html>";
                      server.send(200, "text/html", content); });

        server.on("/scan", [&]()
                  {
                      IPAddress ip = WiFi.softAPIP();
                      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

                      content = "<!DOCTYPE HTML>\r\n<html>go back";
                      server.send(200, "text/html", content); });

        server.on("/setting", [&]()
                  {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      int statusCode;
      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (uint8_t i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");

        Serial.println("writing eeprom ssid:");
        for (uint8_t i = 0; i < qsid.length(); ++i) {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (uint8_t i = 0; i < qpass.length(); ++i) {
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
      server.send(statusCode, "application/json", content); });
    }
}
