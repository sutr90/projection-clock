#ifndef Projector_hpp
#define Projector_hpp

#include <stdint.h>
/*
Class for controlling an LCD screen from the S282A projection clock. The display has 4 7-segment digits, colon separator, and 2 PM labels.
Each digit is controlled using two frames, where each address corresponds to half of the digit - see below. The display is latching, 
once you send it data it will keep them on, until new frame changing the state is sent.

The display has three control signals:

  latch
  ────────┐                                                              ┌────
          │                                                              │
          └──────────────────────────────────────────────────────────────┘
  clock
  ─────────┐   ┌───┐   ┌───┐   ┌───┐   ┌───┐   ┌───┐   ┌───┐   ┌───┐   ┌───────
           │   │   │   │   │   │   │   │   │   │   │   │   │   │   │   │
           └───┘   └───┘   └───┘   └───┘   └───┘   └───┘   └───┘   └───┘
  data
  ────────────┐      ┌───────────────┐           ┌──────────┐    ┌─────────────
              │      │               │           │          │    │
              └──────┘               └───────────┘          └────┘

When latch is low, the data is fed into the display on rising clock edge.
The display uses simple protocol. First is a 3-bit frame header, then 6-bit address and last 4-bit of data. 
The data are sent in MSB first order.

On boot the display has to be initilazed by sending two initiliazation busrst of three frames each:

  initFrame(0x0, 0x2);
  initFrame(0x0, 0x6);
  initFrame(0xa, 0x0);

When controlling the individual segments, following frames are sent:
0b101 | <6 bit address> | <4-bit data>
The address starts at 0 and counts upwards. The first digit is addressed using 0 and 1, the second 2 and 3, etc.

The segments are then addressed in following way:

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

The first number is the address offset, and the second is the bit value for the segment. 1 means turn on, 0 means turn off. 
The MSB data bit of the higher address (addresses 1,3,5,7) controlls the extra function of the display:
address | function
0x1     | PM symbol
0x3     | colon symbol
0x5     | unused 
0x7     | PM symbol upside down 
*/
class Projector {
    public:
        Projector(uint8_t data_pin, uint8_t clock_pin, uint8_t latch_pin);
        void initializeModule();
        void clearDisplay();
        void sendFrame(uint8_t address, uint8_t data);
    private:
        void initFrame(uint8_t address, uint8_t data);
        void writeBitLow();
        void writeBitHigh();
        void writeBit(uint8_t bits, uint8_t index);
        uint8_t dataPin;
        uint8_t clockPin;
        uint8_t latchPin;
};


#endif