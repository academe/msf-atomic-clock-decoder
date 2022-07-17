
#include "MSFDecoderBitStream.h"

// Details for the state machine.
// Move to class MSFDecoderBitStream

/**
 * @brief run the state machine to parse the carrier to bits A and B
 * 
 * Some state transation will provide an A and B bit pair. Others will
 * not, and are just steps to build up to the bit parsing.
 * This function will return true if there are a new pair of bits that
 * can be read from the msfState structure.
 * 
 * @todo move into MSFDecoderBitStream class
 * 
 * @param msfCarrier *carrier this could probably be injected at initialisation of the class
 * @return bool true if a pair of bits and a seconds number is ready
 */
int MSFDecoderBitStream::msfNextState(msfCarrier *carrier)
{
    bool carrierOn = carrier->carrierOn();
    unsigned int divCount = carrier->divCount();

    // Replace state numbers with a short code or letter to be easier to read.
    // swith only handles integer expressions - doh
    // switch (sprintf("%d:%s:%d", msfState->state, carrierOn ? "H" : "L", divCount)) {
    //   // The first half of a minute marker.
    //   // \⎽⎽⎽⎽⎽/
    //   //  MMMMM

    //   case "MM_SEARCH:L:5":
    //     msfState->state = 999; // The new state.
    //     break;

    //   default:
    //     // Unexpected state; go back to minute marker searching.
    //     break;
    // };


    switch (state) {
    // Waiting for 5 divs carrier OFF to start the first minute,
    // or the next minute after losing the sync.
    // @todo While in this state, check for an inverted input
    // and toggle the inversion flag if necessary. Passing in the
    // MSFDecoverCarrier class here would help to do that.
    case States::wait_minute_marker_start:
        lockedMinuteMarker = 0;
        secondsNumber = -1;
        if (divCount == 5 && ! carrierOn) {
        state = States::wait_minute_marker_end;
        }
        if (divCount > 6 && ! carrierOn) {
        // Likely inverted. Carrier should never normally remain
        // off for more than 5 divs.
        }
        break;

    // Waiting for 5 divs carrier ON
    case States::wait_minute_marker_end:
        if (divCount == 5 && carrierOn) {
        // Got it. That's the start of a minute.
        lockedMinuteMarker = 1;
        secondsNumber = 0;
        bits.A = 1;
        bits.B = 1;
        state = States::wait_next_second;
        break;
        }
        state = States::wait_minute_marker_start;
        break;

    // Waiting for the carrier OFF at the start of the second to
    // capture the start bit and any initial "1" bits.
    case States::wait_next_second:
        if (carrierOn) {
        state = States::wait_minute_marker_start;
        break;
        }
        // msfState->secondsNumber++;
        // Will be 00 or 01. Don't know which yet.
        if (divCount == 1) {
        state = States::wait_0X_bit_1_end;
        break;
        }
        // One carrier off div after start bit, so must be a 10.
        // Still wait until the end of the second before calling it.
        if (divCount == 2) {
        state = States::wait_10_end;
        break;
        }
        // Two carrier off divs after the start bit, so is a 11.
        // Still wait until the end of the second before calling it.
        if (divCount == 3) {
        state = States::wait_11_end;
        break;
        }
        // The start of the next minute.
        if (divCount == 5) {
        state = States::wait_minute_marker_end;
        break;
        }
        state = States::wait_minute_marker_start;
        break;

    // Wait for the 7 divs to the end of the second for a 11.
    case States::wait_11_end:
        if (divCount == 7 && carrierOn) {
        bits.A = 1;
        bits.B = 1;
        secondsNumber++;
        state = States::wait_next_second;
        break;
        }
        state = States::wait_minute_marker_start;
        break;

    // Wait for the 8 divs to the end of the second for a 10.
    case States::wait_10_end:
        if (divCount == 8 && carrierOn) {
        bits.A = 1;
        bits.B = 0;
        secondsNumber++;
        state = States::wait_next_second;
        break;
        }
        state = States::wait_minute_marker_start;
        break;

    case States::wait_0X_bit_1_end:
        // After the starting bit, carrier was on right to the end
        // of the second, giving a 00.
        if (divCount == 9 && carrierOn) {
        bits.A = 0;
        bits.B = 0;
        secondsNumber++;
        state = States::wait_next_second;
        break;
        }
        // Carrier was on just for 1 div, so the second bit will be 1.
        // Wait until the end of the second before calling it (which is
        // two carrier transistions away).
        if (divCount == 1 && carrierOn) {
        state = States::wait_01_bit_2_end;
        break;
        }
        state = States::wait_minute_marker_start;
        break;

    // Got a 0 after the start bit. Wait for the end of the second bit,
    // which will be 1 div.
    case States::wait_01_bit_2_end:
        if (divCount == 1 && ! carrierOn) {
        state = States::wait_01_end;
        break;
        }
        state = States::wait_minute_marker_start;
        break;

    // Waiting for the 7 div carrier on after a 01 to take us to
    // the end of the second.
    case States::wait_01_end:
        if (divCount == 7 && carrierOn) {
        bits.A = 0;
        bits.B = 1;
        secondsNumber++;
        state = States::wait_next_second;
        break;
        }
        state = States::wait_minute_marker_start;
        break;

    };

    return secondsNumber;
}
