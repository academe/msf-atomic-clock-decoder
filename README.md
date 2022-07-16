# msf-atomic-clock-decoder

Decode MSF atomic clock transmissions using ESP8266 and interrupts.

This is a WIP propject, with lots of refactoring, exploration and trying things out.

The end aim is for a library to support decoding of MSF-60 (UK) atomic clock transmissions on an ESP8266, and ESP32.

The library will use interrupts, so the ESP/Arduino can carry on doing other things, such as handling the clock display.

The main components are:

    carrier on/off timer -> state machine bit extractor -> date and time constructor -> clock display

## State Diagrams

### Success Path Transistions

Each transition is marked with the carrier state (`ON` or `OFF`) and the
number of divs is has stayed in that state (each div is 100mS).

The success path from `wait_next_second` back to itself will contain
exactly ten divs, addin up to one second. Each transition will be a
state change of the carrier from `ON` to `OFF`, or from `OFF` to `ON`.

```mermaid
stateDiagram-v2
    [*] --> wait_minute_marker_start
    wait_minute_marker_start --> wait_minute_marker_end : OFF 5
    wait_minute_marker_end --> wait_next_second : ON 5
    wait_next_second --> wait_0X_bit_1_end : OFF 1
    wait_next_second --> wait_10_end : OFF 2
    wait_next_second --> wait_11_end : OFF 3
    wait_next_second --> wait_minute_marker_end : OFF 5
    wait_11_end --> wait_next_second : ON 7
    wait_10_end --> wait_next_second : ON 8
    wait_01_bit_2_end --> wait_01_end : OFF 1
    wait_0X_bit_1_end --> wait_next_second : ON 9
    wait_0X_bit_1_end --> wait_01_bit_2_end : ON 1
    wait_01_end --> wait_next_second : ON 7
```

### Error Transistion Recovery

If a carrier state for an unexpected number of divs occurs, then the
minute lock is considered lost.
The state returns to waiting for the miniute marker start (5 divs of
carrier `OFF`) to get a lock on a new minute.
Later there may be patterns that can be used to lock back on to the
miniute earlier, or places where a small timing error can be ignored
because they don't affect the bit stream being read.

```mermaid
stateDiagram-v2
    wait_minute_marker_start --> wait_minute_marker_end
    wait_11_end --> wait_minute_marker_start
    wait_10_end --> wait_minute_marker_start
    wait_01_bit_2_end --> wait_minute_marker_start
    wait_0X_bit_1_end --> wait_minute_marker_start
    wait_01_end --> wait_minute_marker_start
    wait_minute_marker_end --> wait_next_second
```
