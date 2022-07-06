// MSFDecoderExample
// Simple Arduino sketch that demonstrates use of MSFDecoder library.
// Kris Adcock 9th March 2013

#include <LiquidCrystal.h>
#include <MSFDecoder.h> 

MSFDecoder MSF; // our instance of the decoder library,
// Only one possible per project! (But then, why would you have more than one decoder connected?)

LiquidCrystal lcd(12 /*RS*/, 11 /*Enable*/, 7 /*D4*/, 8 /*D5*/, 9 /*D6*/, 10 /*D7*/); // Our lcd module

bool g_bPrevCarrierState;
byte g_iPrevBitCount;
// These globals are just to prevent needless updating of the screen. 
// Whether YOU need them or not depends on your project.

// Anything relating to serial debugging has been commented-out. Feel free to re-add it if you don't have an
// LCD module.

void setup()
{
   MSF.init();
   lcd.begin(16, 4);
   //Serial.begin(115200);
   g_bPrevCarrierState = false;
   g_iPrevBitCount = 255;
}

void loop()
{
  // print current progress of decoding.
  // (Only really done for reasons of feedback - you could happily ignore this if you don't need it.)
  bool bCarrierState = MSF.getHasCarrier();
  byte iBitCount = MSF.getBitCount();
  if ((bCarrierState != g_bPrevCarrierState) || (bCarrierState == true && iBitCount != g_iPrevBitCount))
  {
    lcd.setCursor(14, 0);

    if (bCarrierState)
    {    
      if (iBitCount < 10) lcd.print('0');
      lcd.print(iBitCount);
      
      //Serial.print(iBitCount);
    } else
    {
      lcd.print("--");
      //Serial.print("--");
    }
  }
  g_bPrevCarrierState = bCarrierState;
  g_iPrevBitCount = iBitCount;
  
  // check if there's a freshly-decoded time in the decoder ...
  if (MSF.m_bHasValidTime)
  {
    // yep!

    char buffer[20];
    
    sprintf(buffer, "%02d:%02d", MSF.m_iHour, MSF.m_iMinute);
    lcd.setCursor(0, 0);
    lcd.print(buffer);
    //Serial.println(buffer);
    
    sprintf(buffer, "%02d/%02d/20%02d", MSF.m_iDay, MSF.m_iMonth, MSF.m_iYear);
    lcd.setCursor(0, 1);
    lcd.print(buffer);
    //Serial.println(buffer);

    // lastly, clear the flag so we don't bother getting it on the next iteration ...
    MSF.m_bHasValidTime = false;
  }
}

