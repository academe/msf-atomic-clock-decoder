#ifndef MSF_DECODER_CARRIER_H
#define MSF_DECODER_CARRIER_H

#include <stdbool.h>
#include <Wire.h>

class msfDecoderCarrier {
    private:
        // The carrier state the last sequence of divisons.
        // Each division is 100mS, one tenth of a second.
        // The carrier off represents a logical 1.
        volatile bool carrierOnFlag = true;

        // Invert the carrier pin.
        volatile bool invertCarrier = false;

        // The time in mS that the carrier came on (rising edge).
        volatile unsigned long pinHighTime = 0;

        // The time in mS that the carrier dropped (falling edge).
        volatile unsigned long pinLowTime = 0;

        // The number of divisions that have completed in the above state.
        // A divCount of 7 will be 700mS.
        volatile unsigned int divCountValue = 0;

        int round100(int number) const;

    public:
        void setCarrierState(int pinValue);

        bool carrierOn() const;
        unsigned int divCount() const;
};

#endif
