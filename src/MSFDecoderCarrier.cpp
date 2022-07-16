
#include "MSFDecoderCarrier.h"

// Class for managing the carrier monitoring.

/**
 * @brief round a number to the nearest unit of 10^places
 * 
 * @param number the number to round
 * @param place example 2 = round to the nearest 100
 * @return int 
 */
int msfCarrier::round100(int number) const
{
    int r = number % 100;

    if (r < 50) {
        return number - r;
    } else {
        return number - r + 100;
    }
}

/**
 * @brief Set the Carrier State on a rising or fallign edge.
 * Usually will be set here in an interrupt.
 * 
 * @param pinValue 
 */
void msfCarrier::setCarrierState(int pinValue)
{
    if (pinValue) {
        carrierOnTime = millis();
        divCount = round100(carrierOnTime - carrierOffTime) / 100;
        carrierOn = true;
    } else {
        carrierOffTime = millis();
        divCount = round100(carrierOffTime - carrierOnTime) / 100;
        carrierOn = false;
    }
}

// @todo add invert helpers

/**
 * @brief indicate whether the carrier is on, taking the invert flag
 * into account.
 * 
 * @return true 
 * @return false 
 */
bool msfCarrier::carrierOnState() const
{
    return invertCarrier ? !carrierOn : carrierOn;
}

unsigned int msfCarrier::lastDivCount() const
{
    return divCount;
}
