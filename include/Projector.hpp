#ifndef Projector_hpp
#define Projector_hpp

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
}


#endif