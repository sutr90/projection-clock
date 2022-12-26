#include "Arduino.h"
#include "../include/ClockProjector.hpp"

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
ClockProjector::ClockProjector(uint8_t data_pin, uint8_t clock_pin, uint8_t latch_pin) : Projector::Projector(data_pin, clock_pin, latch_pin),
                                                                                         half0{15, 0, 13, 9, 2, 11, 15, 1, 15, 11, 7, 14, 15, 12, 15, 7, 0},
                                                                                         half1{5, 5, 3, 7, 7, 6, 6, 5, 7, 7, 7, 6, 0, 7, 2, 2, 0} {};

void ClockProjector::display2digits(uint8_t value, uint8_t unit, bool colon)
{
  uint8_t ones = value % 10;
  uint8_t tens = value / 10;

  sendFrame(0 + unit, half0[tens]);
  sendFrame(1 + unit, half1[tens]);
  sendFrame(2 + unit, half0[ones]);
  sendFrame(3 + unit, half1[ones] + (unit == HOURS && colon ? 8 : 0));
}

void ClockProjector::showTime(uint8_t hours, uint8_t minutes, bool colon)
{
  display2digits(hours, HOURS, colon);
  display2digits(minutes, MINUTES, false);
}
