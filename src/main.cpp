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

// To monitor the clock input
unsigned int GPIO_MSF_in = 14;

// Second number, 0 to 59, and exceptionally 60.
// Only increment when locked onto the minute marker.
volatile unsigned int secondsNumber = 0;

// The state when counting bits.

volatile unsigned int state = 0;

// True when locked to the minute marker.
volatile bool lockedMinuteMarker = false;

long lastSecondsNumber = 0;

// For keeping track of the carrier states.

msfCarrier *carrier;

// The bit streaming state machine

MSFDecoderBitStream *stateMachine;

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

  carrier->setCarrierState(digitalRead(GPIO_MSF_in));

  // carrier.carrierOn(), carrier.divCount()
  stateMachine->msfNextState(carrier);
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

  carrier = new msfCarrier();
  stateMachine = new MSFDecoderBitStream(); // @todo pass carrier in
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
        "%d %02d %d%d\n",
        stateMachine->lockedMinuteMarker,
        stateMachine->secondsNumber,
        stateMachine->bits.A,
        stateMachine->bits.B
      );
    }

    // Serial.printf("%d\n", States::wait_minute_marker_end);

    lastSecondsNumber = stateMachine->secondsNumber;

    // display.display(); // We want to do this only if a state has changed.
  }
}
