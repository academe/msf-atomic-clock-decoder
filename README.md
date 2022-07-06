# msf-atomic-clock-decoder

Decode MSF atomic clock transmissions using ESP8266 and interrupts.

This is a WIP propject, with lots of refactoring, exploration and trying things out.

The end aim is for a library to support decoding of MSF-60 (UK) atomic clock transmissions on an ESP8266, and ESP32.

The library will use interrupts, so the ESP/Arduino can carry on doing other things, such as handling the clock display.

The main components are:

    carrier on/off timer -> state machine bit extractor -> date and time constructor -> clock display
