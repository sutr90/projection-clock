#include <stdint.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ezTime.h>
#include "../include/ClockProjector.hpp"
#include "../include/WifiManager.hpp"
#include <TM1637Display.h>
#include <OneButton.h>

Timezone timezone;

bool tzSynced = false;
bool displayOn = true;
bool colonOn = true;

uint8_t dataPin = D2;  // DATA
uint8_t clkPin = D3;   // WRITE
uint8_t latchPin = D4; // CS

ClockProjector projector(dataPin, clkPin, latchPin);
WifiManager wifiManager(D1, 80);
TM1637Display display(D6, D5);
OneButton btn = OneButton(
    D7,    // Input pin for the button
    true, // Button is active LOW
    true  // Enable internal pull-up resistor
);

// Handler function for a single click:
static void handleClick()
{
  Serial.println("click");
  displayOn = !displayOn;
  digitalWrite(D0, displayOn ? LOW : HIGH);
  display.setBrightness(0x1, displayOn);
}

void updateDisplay(bool colon)
{
  Serial.println("Prague time: " + timezone.dateTime());

  uint8_t h = timezone.hour();
  uint8_t m = timezone.minute();
  projector.showTime(h, m, colon);

  uint8_t colonMask = colon ? 0b01000000 : 0;

  display.showNumberDecEx(h, colonMask, true, 2, 0);
  display.showNumberDecEx(m, colonMask, true, 2, 2);
}

void setup()
{
  projector.initializeModule();
  projector.clearDisplay();

  //Serial.begin(115200);
  wifiManager.initialize();

  display.setBrightness(0x0f);
  display.clear();

  pinMode(D0, OUTPUT);
  btn.attachClick(handleClick);
}

void loop()
{
  btn.tick();

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
      updateDisplay(false);
    }

    events();

    if (secondChanged())
    {
      colonOn = !colonOn;
      updateDisplay(colonOn);
    }
  }
  else
  {
    tzSynced = false;
  }
}
