
#include "MSFDecoderCarrier.h"

/**
 * @brief Class for managing the carrier monitoring.
 *
 * Given a carrier state change, provide the number of divs
 * (blocks of 100mS) the carrier stayed in the previous state.
 * A carrier that was OFF for 300mS, upon receiving an ON edge
 * will return a time of 3 divs.
 */

/**
 * @brief round a number to the nearest 100
 * 
 * @param number the number to round
 * @return int 123->100, 165->150
 */
int msfDecoderCarrier::round100(int number) const
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
 * Usually will be set in an interrupt.
 * 
 * The assumption is that the carrier ON will take the
 * GPIO digtal pin HIGH. This can be inverted if required.
 * 
 * @param pinValue 
 */
void msfDecoderCarrier::setCarrierState(int pinState)
{
    if (pinState) {
        pinHighTime = millis();
        divCountValue = round100(pinHighTime - pinLowTime) / 100;
        carrierOnFlag = true;
    } else {
        pinLowTime = millis();
        divCountValue = round100(pinLowTime - pinHighTime) / 100;
        carrierOnFlag = false;
    }
}

// @todo add helpers to get/set/toggle the invert flag

/**
 * @brief indicate whether the carrier is on, taking the invert flag
 * into account.
 * 
 * @return true if carrier is on
 * @return false if carrier is off
 */
bool msfDecoderCarrier::carrierOn() const
{
    return invertCarrier ? !carrierOnFlag : carrierOnFlag;
}

// @todo divCount
unsigned int msfDecoderCarrier::divCount() const
{
    return divCountValue;
}
