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

#include <ESP8266WiFi.h>

// To monitor the clock input
byte GPIO_MSF_in = 14;

volatile struct {
  bool A = 0;
  bool B = 0;
} bits;

// The structure for keeping track of the carrier states.

struct msfCarrierStateTime {
  // The time in mS that the carrier came on (rising edge).
  unsigned long carrierOnTime = 0;

  // The time in mS that the carrier dropped (falling edge).
  unsigned long carrierOffTime = 0;

  // The carrier state the last sequence of divisons.
  // Each division is 100mS, one tenth of a second.
  // The carrier off represents a logical 1.
  bool carrierOff = 0;
  bool carrierOn = 1;

  // The number of divisions that have completed in the above state.
  // A divCount of 7 will be 700mS.
  unsigned int divCount = 0;

  // Invert the carrier pin.
  volatile bool invertCarrier = false;
};

// For keeping track of the carrier states.

volatile msfCarrierStateTime carrier;

// Second number, 0 to 59, and exceptionally 60.
// Only increment when locked onto the minute marker.
volatile unsigned int secondsNumber = 0;

// The state when counting bits.

volatile unsigned int state = 0;

// True when locked to the minute marker.
volatile bool lockedMinuteMarker = false;

unsigned long lastSecondsNumber = 0;

const int CARRIER_STATE_START = 0;
const int CARRIER_STATE_MINUTE_MARKER = 1;
const int CARRIER_STATE_11_COMPLETE = 2;
const int CARRIER_STATE_10_COMPLETE = 3;
const int CARRIER_STATE_0X = 4;
const int CARRIER_STATE_01 = 5;
const int CARRIER_STATE_01_COMPLETE = 6;

// Details for the state machine

struct msfState {
  struct {
    bool A = 0;
    bool B = 0;
  } bits;

  // True when locked to the minute marker, meaning the date and
  // time is being parsed.
  bool lockedMinuteMarker = false;

  // Seconds number, 0 to 59, and exceptionally 60.
  // Only incremented when the minute marker has been locked in.
  unsigned int secondsNumber = 0;

  // The current state when counting bits.
  byte state = 0;
};

/**
 * @brief round a number to the nearest unit of 10^places
 * 
 * @param number the number to round
 * @param place example 2 = round to the nearest 100
 * @return int 
 */
int round100(int number)
{
    int r = number % 100;

    if (r < 50) {
        return number - r;
    } else {
        return number - r + 100;
    }
}

/**
 * @brief Read the carrier last state and length of that state
 * 
 * @param carrier 
 */
void IRAM_ATTR readCarrierStateTime(volatile msfCarrierStateTime *carrier, int pinValue)
{
  // Read the current state, and the last state will be the inverse.

  if (pinValue) {
    carrier->carrierOnTime = millis();
    carrier->divCount = round100(carrier->carrierOnTime - carrier->carrierOffTime) / 100;
    carrier->carrierOff = 0;
    carrier->carrierOn = 1;
  } else {
    carrier->carrierOffTime = millis();
    carrier->divCount = round100(carrier->carrierOffTime - carrier->carrierOnTime) / 100;
    carrier->carrierOff = 1;
    carrier->carrierOn = 0;
  }
}

/**
 * @brief run the state machine to parse the carrier to bits A and B
 * 
 * Some state transation will provide an A and B bit pair. Others will
 * not, and are just steps to build up to the bit parsing.
 * This function will return true if there are a new pair of bits that
 * can be read from the msfState structure.
 * 
 * @param msfState
 * @param msfCarrierStateTime
 * @return bool true if a pair of bits and a seconds number is ready
 */
bool msfNextState(volatile msfState *msfState, volatile msfCarrierStateTime *carrier)
{
  return 0;
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

  readCarrierStateTime(&carrier, digitalRead(GPIO_MSF_in));

  // In the state machine, we will go through all the states of each
  // second, just to make sure there are no spurious errors. In theory,
  // we only need the first 300mS to determine what the result is.
  // However, we trigger only on rising and falling edges, so for a
  // "00" pair we won't know the result until the start of the next
  // second. Following through all the states keeps that timing
  // consistent for all bit pairs.

  // The main waiting state.
  // Key:
  // S = start div, 100mS of carrier off.
  // M = minute marker precurser, 500mS carrier off.
  // 1 = decoded data bit, carrier off.
  // 0 = decoded data bit, carrier on.
  // P = padding to end of second, carrier on.

  // Waiting for the start of the next second (a carrier off for
  // at least 100mS).

  if (state == CARRIER_STATE_START) {

    if (carrier.carrierOn) {
      return;
    }

    switch (carrier.divCount) {
      // The first half of a minute marker.
      // \⎽⎽⎽⎽⎽/
      //  MMMMM
      case 5: state = CARRIER_STATE_MINUTE_MARKER;
      break;

      // \⎽⎽⎽/
      //  S11
      case 3: state = CARRIER_STATE_11_COMPLETE;
      break;

      // \⎽⎽⎽/
      //  S10
      case 2: state = CARRIER_STATE_10_COMPLETE;
      break;

      // Either 00 or 01; we need to see what happens next.
      // \⎽/
      //  S
      case 1: state = CARRIER_STATE_0X;
      break;
    }

    return;
  }

  // Waiting for the minute marker second to finish.

  if (state == CARRIER_STATE_MINUTE_MARKER) {
    if (carrier.carrierOn && carrier.divCount == 5) {
      // \⎽⎽⎽⎽⎽/⎺⎺⎺⎺⎺\
      //  SSSSS PPPPP
      // Reset the seconds counter.
      secondsNumber = 0;
      lockedMinuteMarker = true;
    }

    state = CARRIER_STATE_START;

    return;
  }

  // Waiting for the 11 second to finish.

  if (state == CARRIER_STATE_11_COMPLETE) {
    if (carrier.carrierOn && carrier.divCount == 7) {
      // \⎽⎽⎽/⎺⎺⎺⎺⎺⎺⎺\
      //  S11 0000000
      if (lockedMinuteMarker) secondsNumber++;

      bits.A = 1;
      bits.B = 1;
    }

    state = CARRIER_STATE_START;

    return;
  }

  // Waiting for the 10 second to finish.

  if (state == CARRIER_STATE_10_COMPLETE) {
    if (carrier.carrierOn && carrier.divCount == 8) {
      // \⎽⎽/⎺⎺⎺⎺⎺⎺⎺⎺\
      //  S1 0PPPPPPP
      if (lockedMinuteMarker) secondsNumber++;

      bits.A = 1;
      bits.B = 0;
    }

    state = CARRIER_STATE_START;

    return;
  }

  // Waiting for the 0X first bit to finish.
  // For 00 the first bit, second bit and the
  // padding will run into one.

  if (state == CARRIER_STATE_0X) {
    if (carrier.carrierOn && carrier.divCount == 9) {
      // \⎽/⎺⎺⎺⎺⎺⎺⎺⎺⎺\
      //  S 00PPPPPPP

      if (lockedMinuteMarker) secondsNumber++;

      bits.A = 0;
      bits.B = 0;

      state = CARRIER_STATE_START;

      return;
    }

    if (carrier.carrierOn && carrier.divCount == 1) {
      // In theory this is enough to determie 01, since the next
      // div can only be a single carrier off (logic 1). But we
      // carry on for error checking.

      // \⎽/⎺\
      //  S 0

      state = CARRIER_STATE_01;

      return;
    }
  }

  // Waiting for the 01 second bit to finish.

  if (state == CARRIER_STATE_01) {
    if (carrier.carrierOff && carrier.divCount == 1) {
      // \⎽/⎺\⎽/
      //  S 0 1

      state = CARRIER_STATE_01_COMPLETE;

      return;
    }
  }

  // Waiting for the 01 second to finish.

  if (state == CARRIER_STATE_01_COMPLETE) {
    if (carrier.carrierOn && carrier.divCount == 7) {
      // ⎺\⎽/⎺\⎽/⎺⎺⎺⎺⎺⎺⎺\
      // P S 0 1 PPPPPPP
      if (lockedMinuteMarker) secondsNumber++;

      bits.A = 0;
      bits.B = 1;

      state = CARRIER_STATE_START;

      return;
    }
  }

  state = CARRIER_STATE_START;
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

  digitalPinToInterrupt(GPIO_MSF_in);
  // RISING FALLING HIGH LOW CHANGE
  attachInterrupt(GPIO_MSF_in, Ext_INT1_MSF, CHANGE);

  // ESP.deepSleep(1000, WAKE_RF_DISABLED); // Not working

  // WiFi.disconnect();
  // WiFi.mode(WIFI_OFF);
  // WiFi.forceSleepBegin();
}

void loop()
{
  if (lastSecondsNumber != secondsNumber) {
    // Serial.printf("%d bits in state %s\n", divCount, carrierOff ? "1" : "0");

    Serial.printf("%d = %d%d\n", secondsNumber, bits.A, bits.B);

    lastSecondsNumber = secondsNumber;

    // display.display(); // We want to do this only if a state has changed.
  }

  // delay(1);
}
