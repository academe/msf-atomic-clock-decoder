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
#include "MSFDecoderBitStream.h"

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

// Where to monitor the clock carrier.
unsigned int GPIO_MSF_in = 14;

short int lastSecondsNumber = 0;

// For keeping track of the carrier states.
msfDecoderCarrier *carrier;

// The bit streaming state machine
MSFDecoderBitStream *stateMachine;



// Constructing the date and time.
class MSFDecoderDateTime {
  private:
    /**
     * @brief Build up a BCD digit by one bit
     * 
     * @param digit the digit number to build on (0-10)
     * @param msb the most significant bit number (17-51)
     * @param lsb the least significant bit number (17-51)
     * @param bitValue the current bit value (on or off)
     * @param secondsNumber the seconds number (0-59)
     */
    void bcdBuild(short int digit, short int msb, short int lsb, short int bitValue, short int secondsNumber)
    {
      // Out of range; do nothing.

      if (secondsNumber < msb || secondsNumber > lsb) {
        return;
      }

      // First bit; reset the digit to zero.

      if (secondsNumber == msb) {
        bcdDigits[digit] = 0;
      }

      // Carrier bit not set; do nothing more.

      if (! bitValue) {
        return;
      }

      // Carrier bit is set; set the appropriate digit bit.

      bcdDigits[digit] |= 1 << (lsb - secondsNumber);
    }

  public:
    MSFDecoderBitStream *stateMachineObj;

    // We will build the date and time in one structure, then copy it 
    // to another structure once it is complete. This provides an
    // unchanging copy that can be read over the full minute it applies
    // to.
    // @todo use some kind of unsigned byte value - a char?

    short int year = 0;
    short int month = 0;
    short int day = 0;
    short int hour = 0;
    short int minute = 0;

    short int lastSecond = 0;

    // The decoded BCD digits to build up.

    short int bcdDigits[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    MSFDecoderDateTime(MSFDecoderBitStream *stateMachine) {
      stateMachineObj = stateMachine;
    }


    /**
     * @brief take the bits for the current second and incorporate them
     * into the date-time.
     *
     * @todo keep track of the last second processed, for cleaner re-entrant behaviour.
     * @todo include the check digits and checksum.
     * @todo strategies:
     * - Build up single-digit bit sequences then convert to a decimal once complete.
     *   This feels like a good abstract approach - converting bits to a stream
     *   of decimal digits, which can then be arranged and converted to decimal values
     *   (which itself can be left until that conversion is needed, if at all, to save
     *   CPU).
     * - Reset values at the start of a sequence and build up the values bit-by-bit.
     */
    void processBit() {
      // If we have already dealt with this second, then don't do it again.
      if (lastSecond == stateMachineObj->secondsNumber) {
        return;
      }

      lastSecond = stateMachineObj->secondsNumber;

      // Bit 17A is the start of the year sequence. Reset the year here.

      // Only bother if in the BCD digits range

      if (stateMachineObj->secondsNumber >= 17 || stateMachineObj->secondsNumber <= 51) {
        bcdBuild(0, 17, 20, stateMachineObj->bits.A, stateMachineObj->secondsNumber);
        bcdBuild(1, 21, 24, stateMachineObj->bits.A, stateMachineObj->secondsNumber);
        bcdBuild(2, 25, 25, stateMachineObj->bits.A, stateMachineObj->secondsNumber);
        bcdBuild(3, 26, 29, stateMachineObj->bits.A, stateMachineObj->secondsNumber);
        bcdBuild(4, 30, 31, stateMachineObj->bits.A, stateMachineObj->secondsNumber);
        bcdBuild(5, 32, 35, stateMachineObj->bits.A, stateMachineObj->secondsNumber);
        bcdBuild(6, 36, 38, stateMachineObj->bits.A, stateMachineObj->secondsNumber);
        bcdBuild(7, 39, 40, stateMachineObj->bits.A, stateMachineObj->secondsNumber);
        bcdBuild(8, 41, 44, stateMachineObj->bits.A, stateMachineObj->secondsNumber);
        bcdBuild(9, 45, 47, stateMachineObj->bits.A, stateMachineObj->secondsNumber);
        bcdBuild(10, 48, 51, stateMachineObj->bits.A, stateMachineObj->secondsNumber);
      }
    }
};


MSFDecoderDateTime *decoderDateTime;


/**
 * @brief catches a rising or falling carrier event and parses out data
 * 
 * Handles inversion if set.
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
  carrier->setCarrierState(digitalRead(GPIO_MSF_in));
  stateMachine->nextState();
  decoderDateTime->processBit();
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

  carrier = new msfDecoderCarrier();
  stateMachine = new MSFDecoderBitStream(carrier);
  decoderDateTime = new MSFDecoderDateTime(stateMachine);
}

int carrierOn = 0;

void loop()
{
  // Can this condition be built into the state machine class as a function.
  // Perhaps stateMachine.getSecond() which will return 0 if there is
  // no change, or the second number if a new second has been reached.
  // Seconds go from 1 to 59 (or 58 or 60) with no zero-second. The zero-
  // second is the end of the second of the last minute, so will be high.

  // @todo create some helper functions for reading these

  if (lastSecondsNumber != stateMachine->secondsNumber) {

    // Serial.printf("%d = %d%d \n", secondsNumber, bits.A, bits.B);
    if (stateMachine->secondsNumber != -1) {
      Serial.printf(
        "%d: '%d%d' %02d-%02d-%02d %02d:%02d:%02d %d%d-%d%d-%d%d (%d) %d%d:%d%d:%02d\n",
        stateMachine->lockedMinuteMarker,
        stateMachine->bits.A,
        stateMachine->bits.B,
        decoderDateTime->year,
        decoderDateTime->month,
        decoderDateTime->day,
        decoderDateTime->hour,
        decoderDateTime->minute,
        stateMachine->secondsNumber,
        decoderDateTime->bcdDigits[0],
        decoderDateTime->bcdDigits[1],
        decoderDateTime->bcdDigits[2],
        decoderDateTime->bcdDigits[3],
        decoderDateTime->bcdDigits[4],
        decoderDateTime->bcdDigits[5],
        decoderDateTime->bcdDigits[6],
        decoderDateTime->bcdDigits[7],
        decoderDateTime->bcdDigits[8],
        decoderDateTime->bcdDigits[9],
        decoderDateTime->bcdDigits[10],
        stateMachine->secondsNumber
      );
    }

    // Serial.printf("%d\n", States::wait_minute_marker_end);

    lastSecondsNumber = stateMachine->secondsNumber;

    // display.display(); // We want to do this only if a state has changed.
  }
}
