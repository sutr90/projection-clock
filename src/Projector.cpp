#include "Arduino.h"
#include "../include/Projector.hpp"

Projector::Projector(uint8_t data_pin, uint8_t clock_pin, uint8_t latch_pin) : dataPin{data_pin}, clockPin{clock_pin}, latchPin{latch_pin}
{}

void Projector::writeBitLow() {
  digitalWrite(dataPin, LOW);
  digitalWrite(clockPin, HIGH);
  digitalWrite(clockPin, LOW);
}

void Projector::writeBitHigh() {
  digitalWrite(dataPin, HIGH);
  digitalWrite(clockPin, HIGH);
  digitalWrite(clockPin, LOW);
}

void Projector::writeBit(uint8_t bits, uint8_t index) {
  (bits & (1 << index)) == (1 << index) ? writeBitHigh() : writeBitLow();
}

void Projector::sendFrame(uint8_t address, uint8_t data) {
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

void Projector::initializeModule() {
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  
  delay(100);

  initFrame(0x0, 0x2);
  initFrame(0x0, 0x6);
  initFrame(0xa, 0x0);

  delay(3);

  initFrame(0x0, 0x2);
  initFrame(0x0, 0x6);
  initFrame(0xa, 0x0);

  delay(200);
}

void Projector::initFrame(uint8_t address, uint8_t data) {
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

void Projector::clearDisplay() {
  sendFrame(0, 0);
  sendFrame(1, 0);
  sendFrame(2, 0);
  sendFrame(3, 0);
  sendFrame(4, 0);
  sendFrame(5, 0);
  sendFrame(6, 0);
  sendFrame(7, 0);
}
