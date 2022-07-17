/*
 * A ESP32/OLED exercise to display simple text
 * Board:
 * esp32 by Espressif Systems version 1.0.4
 * OLED library:
 * ESP8266 and ESP32 Oled Driver for SSD1306 displays version 4.1.0
 * 
 * @todo auto-detect an inverted signal (carrier off for at least 700mS or 7 divs)
 */

#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include <SPI.h>
#include "MSFDecoderCarrier.h"

// #include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>
// #include "images.h"
// #include "MSFDecoder.h"
// #include "MsfTimeDecoder.h"

// Initialize the OLED display using Arduino Wire:
#define OLED_RESET -1
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// #include <ESP8266WiFi.h>

// To monitor the clock input
unsigned int GPIO_MSF_in = 14;

// For keeping track of the carrier states.

msfCarrier carrier;

// Second number, 0 to 59, and exceptionally 60.
// Only increment when locked onto the minute marker.
volatile unsigned int secondsNumber = 0;

// The state when counting bits.

volatile unsigned int state = 0;

// True when locked to the minute marker.
volatile bool lockedMinuteMarker = false;

long lastSecondsNumber = 0;

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






// Details for the state machine.
// Move to class MSFDecoderBitStream

struct msfState {

  struct {
    bool A = 0;
    bool B = 0;
  } bits;

  // True when locked to the minute marker, meaning the date and
  // time is being parsed.
  bool lockedMinuteMarker = 0;

  // Seconds number, 0 to 59, and exceptionally 60.
  // Only incremented when the minute marker has been locked in.
  // Will be -1 until the time is locked in.
  int secondsNumber = -1;

  // The current state when counting bits.
  States state = States::wait_minute_marker_start;
};

volatile msfState stateMachine;


/**
 * @brief run the state machine to parse the carrier to bits A and B
 * 
 * Some state transation will provide an A and B bit pair. Others will
 * not, and are just steps to build up to the bit parsing.
 * This function will return true if there are a new pair of bits that
 * can be read from the msfState structure.
 * 
 * @param msfState
 * @return bool true if a pair of bits and a seconds number is ready
 */
int msfNextState(volatile msfState *msfState, bool carrierOn, unsigned int divCount)
{
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


  switch (msfState->state) {
    // Waiting for 5 divs carrier OFF to start the first minute,
    // or the next minute after losing the sync.
    // @todo While in this state, check for an inverted input
    // and toggle the inversion flag if necessary. Passing in the
    // MSFDecoverCarrier class here would help to do that.
    case States::wait_minute_marker_start:
      msfState->lockedMinuteMarker = 0;
      msfState->secondsNumber = -1;
      if (divCount == 5 && ! carrierOn) {
        msfState->state = States::wait_minute_marker_end;
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
        msfState->lockedMinuteMarker = 1;
        msfState->secondsNumber = 0;
        msfState->bits.A = 1;
        msfState->bits.B = 1;
        msfState->state = States::wait_next_second;
        break;
      }
      msfState->state = States::wait_minute_marker_start;
      break;

    // Waiting for the carrier OFF at the start of the second to
    // capture the start bit and any initial "1" bits.
    case States::wait_next_second:
      if (carrierOn) {
        msfState->state = States::wait_minute_marker_start;
        break;
      }
      // msfState->secondsNumber++;
      // Will be 00 or 01. Don't know which yet.
      if (divCount == 1) {
        msfState->state = States::wait_0X_bit_1_end;
        break;
      }
      // One carrier off div after start bit, so must be a 10.
      // Still wait until the end of the second before calling it.
      if (divCount == 2) {
        msfState->state = States::wait_10_end;
        break;
      }
      // Two carrier off divs after the start bit, so is a 11.
      // Still wait until the end of the second before calling it.
      if (divCount == 3) {
        msfState->state = States::wait_11_end;
        break;
      }
      // The start of the next minute.
      if (divCount == 5) {
        msfState->state = States::wait_minute_marker_end;
        break;
      }
      msfState->state = States::wait_minute_marker_start;
      break;

    // Wait for the 7 divs to the end of the second for a 11.
    case States::wait_11_end:
      if (divCount == 7 && carrierOn) {
        msfState->bits.A = 1;
        msfState->bits.B = 1;
        msfState->secondsNumber++;
        msfState->state = States::wait_next_second;
        break;
      }
      msfState->state = States::wait_minute_marker_start;
      break;

    // Wait for the 8 divs to the end of the second for a 10.
    case States::wait_10_end:
      if (divCount == 8 && carrierOn) {
        msfState->bits.A = 1;
        msfState->bits.B = 0;
        msfState->secondsNumber++;
        msfState->state = States::wait_next_second;
        break;
      }
      msfState->state = States::wait_minute_marker_start;
      break;

    case States::wait_0X_bit_1_end:
      // After the starting bit, carrier was on right to the end
      // of the second, giving a 00.
      if (divCount == 9 && carrierOn) {
        msfState->bits.A = 0;
        msfState->bits.B = 0;
        msfState->secondsNumber++;
        msfState->state = States::wait_next_second;
        break;
      }
      // Carrier was on just for 1 div, so the second bit will be 1.
      // Wait until the end of the second before calling it (which is
      // two carrier transistions away).
      if (divCount == 1 && carrierOn) {
        msfState->state = States::wait_01_bit_2_end;
        break;
      }
      msfState->state = States::wait_minute_marker_start;
      break;

    // Got a 0 after the start bit. Wait for the end of the second bit,
    // which will be 1 div.
    case States::wait_01_bit_2_end:
      if (divCount == 1 && ! carrierOn) {
        msfState->state = States::wait_01_end;
        break;
      }
      msfState->state = States::wait_minute_marker_start;
      break;

    // Waiting for the 7 div carrier on after a 01 to take us to
    // the end of the second.
    case States::wait_01_end:
      if (divCount == 7 && carrierOn) {
        msfState->bits.A = 0;
        msfState->bits.B = 1;
        msfState->secondsNumber++;
        msfState->state = States::wait_next_second;
        break;
      }
      msfState->state = States::wait_minute_marker_start;
      break;

  };

  return msfState->secondsNumber;
}

/**
 * @brief catches a rising or falling carrier event and parses out data
 * 
 * @todo Handles inversion if set.
 * @todo Handles automatic detection of inversion if required.
 * 
 * The ISR has three main steps:
 * 
 * - Calculate the time and state of the last signal period.
 * - Run a state machine to extract the seconds number and
 *   the A/B bits. Also monitors for errors.
 * - Mapper to construct the A/B bits into the final data form
 *   as they arrive.
 * 
 * On moving through a complete minute, the date and time can
 * be constructed. At the start of the next miniute, the constructed
 * date and time are marked as valid and current. A seconds counter
 * is also available from the moment first locking onto the minute
 * marker.
 */
void IRAM_ATTR Ext_INT1_MSF()
{
  // @todo raise a software interrupt to enable or disable
  // the signal indicator. The indicator could be an LED on
  // a GPIO pin or an icon on an LED display.

  // @todo automatic invert if the input looks inverted.

  carrier.setCarrierState(digitalRead(GPIO_MSF_in));

  msfNextState(&stateMachine, carrier.carrierOn(), carrier.divCount());
}

void setup()
{
  Serial.begin(9600);
  Serial.println();
  Serial.println();

  // display.begin(SSD1306_SWITCHCAPVCC, 0x3C, true);
  // display.clearDisplay();

  // display.setTextSize(1);
  // display.setTextColor(WHITE);
  // display.setCursor(0,0);
  // display.println("Hello, world!");

  // display.display();

  // MSF.init();

  // g_bPrevCarrierState = false;
  // g_iPrevBitCount = 255;

  // Initialising the display.
  // display.init();

  // display.flipScreenVertically();
  // display.setTextAlignment(TEXT_ALIGN_LEFT);
  // display.setFont(ArialMT_Plain_16);
  // display.clear();

  pinMode(GPIO_MSF_in, INPUT);

  // Do this in MSF library init.
  // Only one interrupt can be added to a pin, so we must
  // use CHANGE to catch both up and down edges.

  // RISING FALLING HIGH LOW CHANGE
  attachInterrupt(digitalPinToInterrupt(GPIO_MSF_in), Ext_INT1_MSF, CHANGE);

  // ESP.deepSleep(1000, WAKE_RF_DISABLED); // Not working

  // WiFi.disconnect();
  // WiFi.mode(WIFI_OFF);
  // WiFi.forceSleepBegin();
}

int carrierOn = 0;

void loop()
{
  // Can this condition be built into the state machine class as a function.
  // Perhaps stateMachine.getSecond() which will return 0 if there is
  // no change, or the second number if a new second has been reached.
  // Seconds go from 1 to 59 (or 58 or 60) with no zero-second. The zero-
  // second is the end of the second of the last minute, so will be high.

  if (lastSecondsNumber != stateMachine.secondsNumber) {

    // Serial.printf("%d = %d%d \n", secondsNumber, bits.A, bits.B);
    if (stateMachine.secondsNumber != -1) {
      Serial.printf(
        "%d %02d %d%d\n",
        stateMachine.lockedMinuteMarker,
        stateMachine.secondsNumber,
        stateMachine.bits.A,
        stateMachine.bits.B
      );
    }

    // Serial.printf("%d\n", States::wait_minute_marker_end);

    lastSecondsNumber = stateMachine.secondsNumber;

    // display.display(); // We want to do this only if a state has changed.
  }
}
