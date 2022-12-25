#ifndef Clock_Projector_hpp
#define Clock_Projector_hpp

#include <stdint.h>
#include "Projector.hpp"

class ClockProjector : public Projector
{
private:
    const uint8_t half0[17];
    const uint8_t half1[17];

    const uint8_t HOURS = 0;
    const uint8_t MINUTES = 4;

    void display2digits(uint8_t value, uint8_t unit);

public:
    ClockProjector(uint8_t data_pin, uint8_t clock_pin, uint8_t latch_pin);
    void showTime(uint8_t hours, uint8_t minutes);
};

#endif