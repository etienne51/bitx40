# BITX40

This is a modified Raduino sketch for my BITX40. It does not allow manual calibration and does not yet use the EEPROM to save current settings. Everything is hard coded and at each power up, default configuration will be used.

## Features

  - Rotary encoder to set the VFO frequency
  - Push button to switch between the VFO frequency adjustment steps *(1Hz, 10Hz, 100Hz, 1kHz)*
  - Push button to switch between LSB and USB

## Things to be added later

  - Make configuration more flexible
  - Input for PTT switch
  - Dual VFO and support for split operation
  - Use the EEPROM or another external memory to save current settings

## Code description

I noticed the *clicking* noise was a lot more pronounced when using more recent versions of the SI5351 library, so v2.0.1 is recommanded for now.
```C
#include <si5351.h>  // Use v2.0.1 to avoid clicking noise when tuning
```
Unlike the original Raduino firmware, I hard coded all the calibration values here, since this sketch I'm using was first meant for testing purposes. I will make things more flexible later on.
```C
#define CALIBRATION  -1116
```
I'm using the #CLK0 output of the SI5351 to generate the BFO frequency. Again, hardcoded values here.
I'm using a different SSB crystal filter that I removed from a Kenwood R-5000, that explains the 8.830MHz IF frequency instead of 12.000MHz with the original BITX40.
```C
#define BFO_CENTER   8828450  // Filter currently used comes from Kenwood R-5000, use 11996250 for internal filter
#define FILTER_WIDTH    2600  // Use 2500 for internal filter
#define BFO_SHIFT (FILTER_WIDTH / 2)
```
I'm using a 400 steps-per-turn optical encoder for the VFO knob. The ```ENCODER_SENSIVITY``` value is used to reduce the resolution of the encoder.
For example here, with a value set to ```10```, I get 1 step every 10 steps.
```C
#define ENCODER_SENSIVITY 10  // Higher value for lower sensivity
```
```C
long diff = encoderValue - lastEncoderValue;
if (abs(diff) >= ENCODER_SENSIVITY) { ... }
```