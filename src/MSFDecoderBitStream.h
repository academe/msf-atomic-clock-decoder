#ifndef MSF_DECODER_BIT_STREAM_H
#define MSF_DECODER_BIT_STREAM_H

#include <stdbool.h>
#include <Wire.h>
#include <MSFDecoderCarrier.h>

class MSFDecoderBitStream {

  public:
    enum class States {
      wait_minute_marker_start,
      wait_minute_marker_end,
      wait_next_second,
      wait_11_end,
      wait_10_end,
      wait_0X_bit_1_end,
      wait_01_bit_2_end,
      wait_01_end
    };

    volatile struct {
      bool A = 0;
      bool B = 0;
    } bits;

    // True when locked to the minute marker, meaning the date and
    // time is being parsed.
    volatile bool lockedMinuteMarker = 0;

    // Seconds number, 0 to 59, and exceptionally 60.
    // Only incremented when the minute marker has been locked in.
    // Will be -1 until the time is locked in.
    volatile short int secondsNumber = -1;

    // The current state when counting bits.
    volatile States state = States::wait_minute_marker_start;

    // The carrier monitoring object.
    msfDecoderCarrier *carrierObj;

    short int nextState();

    // Constructor.
    MSFDecoderBitStream(msfDecoderCarrier *carrier);
};

#endif
