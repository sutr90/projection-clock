#include <stdint.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ezTime.h>
#include "../include/ClockProjector.hpp"
#include "../include/WifiManager.hpp"

Timezone timezone;

bool tzSynced = false;

uint8_t dataPin = D2;  // DATA
uint8_t clkPin = D3;   // WRITE
uint8_t latchPin = D4; // CS

ClockProjector projector(dataPin, clkPin, latchPin);
WifiManager wifiManager(D1, 80);

void setup()
{
  projector.initializeModule();
  projector.clearDisplay();
  
  Serial.begin(115200);
  wifiManager.initialize();
}

void loop()
{
  if ((WiFi.status() == WL_CONNECTED))
  {
    while (!waitForSync(30) && !tzSynced)
    {
      Serial.println("Timezone Sync timeout!");
    }

    if (!tzSynced)
    {
      tzSynced = true;
      timezone.setLocation("Europe/Prague");
    }

    events();
    if (secondChanged())
    {
      Serial.println("Prague time: " + timezone.dateTime());
      projector.showTime(timezone.hour(), timezone.minute());
    }
  }
  else
  {
    tzSynced = false;
  }
}
