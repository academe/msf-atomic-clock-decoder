#ifndef MSF_DECODER_CARRIER_H
#define MSF_DECODER_CARRIER_H

#include <stdbool.h>
#include <Wire.h>

class msfCarrier {
    private:
        volatile bool carrierOn = true;
        volatile bool invertCarrier = false;
        volatile unsigned long carrierOnTime = 0;
        volatile unsigned long carrierOffTime = 0;
        volatile unsigned int divCount = 0;

        int round100(int number) const;

    public:
        void setCarrierState(int pinValue);
        bool carrierOnState() const;
        unsigned int lastDivCount() const;
};

#endif
