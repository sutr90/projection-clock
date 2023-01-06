#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "../include/WifiManager.hpp"
#include "../include/DebugSerial.hpp"

WifiManager::WifiManager(uint8_t resetPin, uint8_t webport) : _resetPin{resetPin}, server{webport}
{
}

void WifiManager::initialize()
{
    DEBUG_SERIAL.println();
    DEBUG_SERIAL.println("Disconnecting previously connected WiFi");
    WiFi.disconnect();

    pinMode(_resetPin, INPUT_PULLUP);
    int sensorVal = digitalRead(_resetPin);
    DEBUG_SERIAL.println();
    DEBUG_SERIAL.print("button value ");
    DEBUG_SERIAL.println(sensorVal);

    if (sensorVal == HIGH)
    {
        EEPROM.begin(512); // Initialasing EEPROM
        delay(10);
        DEBUG_SERIAL.println();
        DEBUG_SERIAL.println();
        DEBUG_SERIAL.println("Startup");

        //---------------------------------------- Read EEPROM for SSID and pass
        DEBUG_SERIAL.println("Reading EEPROM ssid");

        String esid;
        for (uint8_t i = 0; i < 32; ++i)
        {
            esid += char(EEPROM.read(i));
        }
        DEBUG_SERIAL.println();
        DEBUG_SERIAL.print("SSID: ");
        DEBUG_SERIAL.println(esid);
        DEBUG_SERIAL.println("Reading EEPROM pass");

        String epass = "";
        for (uint8_t i = 32; i < 96; ++i)
        {
            epass += char(EEPROM.read(i));
        }
        DEBUG_SERIAL.print("PASS: ");
        DEBUG_SERIAL.println(epass);

        WiFi.begin(esid.c_str(), epass.c_str());

        if (testWifi())
        {
            DEBUG_SERIAL.println("Succesfully Connected!!!");
            return;
        }
    }
    DEBUG_SERIAL.println("Turning the HotSpot On");
    launchWeb();
    setupAP(); // Setup HotSpot

    DEBUG_SERIAL.println();
    DEBUG_SERIAL.println("Waiting.");

    while ((WiFi.status() != WL_CONNECTED))
    {
        DEBUG_SERIAL.print(".");
        delay(100);
        server.handleClient();
    }
}

bool WifiManager::testWifi(void)
{
    int c = 0;
    DEBUG_SERIAL.println("Waiting for Wifi to connect");
    while (c < 20)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            return true;
        }
        delay(500);
        DEBUG_SERIAL.print("*");
        c++;
    }
    DEBUG_SERIAL.println("");
    DEBUG_SERIAL.println("Connect timed out, opening AP");
    return false;
}

void WifiManager::launchWeb()
{
    DEBUG_SERIAL.println("");
    if (WiFi.status() == WL_CONNECTED)
    {
        DEBUG_SERIAL.println("WiFi connected");
    }
    DEBUG_SERIAL.print("Local IP: ");
    DEBUG_SERIAL.println(WiFi.localIP());
    DEBUG_SERIAL.print("SoftAP IP: ");
    DEBUG_SERIAL.println(WiFi.softAPIP());
    createWebServer();
    // Start the server
    server.begin();
    DEBUG_SERIAL.println("Server started");
}

void WifiManager::setupAP(void)
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    int n = WiFi.scanNetworks();
    DEBUG_SERIAL.println("scan done");
    if (n == 0)
    {
        DEBUG_SERIAL.println("no networks found");
    }
    else
    {
        DEBUG_SERIAL.print(n);
        DEBUG_SERIAL.println(" networks found");
        for (uint8_t i = 0; i < n; ++i)
        {
            // Print SSID and RSSI for each network found
            DEBUG_SERIAL.print(i + 1);
            DEBUG_SERIAL.print(": ");
            DEBUG_SERIAL.print(WiFi.SSID(i));
            DEBUG_SERIAL.print(" (");
            DEBUG_SERIAL.print(WiFi.RSSI(i));
            DEBUG_SERIAL.print(")");
            DEBUG_SERIAL.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
            delay(10);
        }
    }

    DEBUG_SERIAL.println("");
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
    DEBUG_SERIAL.println("softap");
    launchWeb();
    DEBUG_SERIAL.println("over");
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
        DEBUG_SERIAL.println("clearing eeprom");
        for (uint8_t i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
        DEBUG_SERIAL.println(qsid);
        DEBUG_SERIAL.println("");
        DEBUG_SERIAL.println(qpass);
        DEBUG_SERIAL.println("");

        DEBUG_SERIAL.println("writing eeprom ssid:");
        for (uint8_t i = 0; i < qsid.length(); ++i) {
          EEPROM.write(i, qsid[i]);
          DEBUG_SERIAL.print("Wrote: ");
          DEBUG_SERIAL.println(qsid[i]);
        }
        DEBUG_SERIAL.println("writing eeprom pass:");
        for (uint8_t i = 0; i < qpass.length(); ++i) {
          EEPROM.write(32 + i, qpass[i]);
          DEBUG_SERIAL.print("Wrote: ");
          DEBUG_SERIAL.println(qpass[i]);
        }
        EEPROM.commit();

        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        ESP.reset();

      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        DEBUG_SERIAL.println("Sending 404");
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content); });
    }
}
