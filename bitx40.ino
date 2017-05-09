/**
 * This source file is under General Public License version 3.
 * 
 * Most source code are meant to be understood by the compilers and the computers. 
 * Code that has to be hackable needs to be well understood and properly documented. 
 * Donald Knuth coined the term Literate Programming to indicate code that is written be 
 * easily read and understood.
 * 
 * The Raduino is a small board that includes the Arduin Nano, a 16x2 LCD display and
 * an Si5351a frequency synthesizer. This board is manufactured by Paradigm Ecomm Pvt Ltd
 * 
 * To learn more about Arduino you may visit www.arduino.cc. 
 * 
 * The Arduino works by firt executing the code in a function called setup() and then it 
 * repeatedly keeps calling loop() forever. All the initialization code is kept in setup()
 * and code to continuously sense the tuning knob, the function button, transmit/receive,
 * etc is all in the loop() function. If you wish to study the code top down, then scroll
 * to the bottom of this file and read your way up.
 * 
 * Below are the libraries to be included for building the Raduino
 */
 
#include <Wire.h>
#include <LiquidCrystal.h>

/** 
 *  The main chip which generates upto three oscillators of various frequencies in the
 *  Raduino is the Si5351a. To learn more about Si5351a you can download the datasheet 
 *  from www.silabs.com although, strictly speaking it is not a requirment to understand this code. 
 *  Instead, you can look up the Si5351 library written by Jason Mildrum, NT7S. You can download and 
 *  install it from https://github.com/etherkit/Si5351Arduino to complile this file.
 *  The Wire.h library is used to talk to the Si5351 and we also declare an instance of 
 *  Si5351 object to control the clocks.
 */
 
#include <si5351.h> // Use v2.0.1 to avoid clicking noise when tuning

#define BFO_CENTER   8828450 // Filter currently used comes from Kenwood R-5000, use 11996250 for internal filter
#define FILTER_WIDTH    2600 // Use 2500 for internal filter
#define BFO_SHIFT (FILTER_WIDTH / 2)

#define CALIBRATION    -1116

#define LOWEST_FREQ  7000000
#define HIGHEST_FREQ 7200000

#define START_FREQ   7060000

#define ENCODER_PIN1 2
#define ENCODER_PIN2 3

#define SWITCH4_PIN  5
#define SWITCH3_PIN  4
/* Switches 1 and 2 not yet wired up */

#define ENCODER_SENSIVITY 10 // Higher value for lower sensivity

LiquidCrystal lcd(8, 9, 10, 11, 12, 13);
Si5351 si5351;

volatile int lastEncoded = 0;
volatile long encoderValue = 0, lastEncoderValue = 0;
long lastencoderValue = 0;
int lastMSB = 0;
int lastLSB = 0;

volatile int lastPressed4 = HIGH, lastPressed3 = HIGH;

bool updateNeeded = true;

char c[30], b[30], printBuff[32];
int count = 0;

bool upperSideBand = false;

long frequency;
unsigned long bfoFreq = BFO_CENTER - BFO_SHIFT; // Lower Side Band
long tuneStep = 100;

void printLine1(char *c) {
  if (strcmp(c, printBuff)){
    lcd.setCursor(0, 0);
    lcd.print(c);
    strcpy(printBuff, c);
    count++;
  }
}

void printLine2(char *c) {
  lcd.setCursor(0, 1);
  lcd.print(c);
}

void updateDisplay() {
  int mhz = frequency / 1000000;
  int khz = (frequency / 1000) % 1000;
  int hz = frequency  % 1000;
  
  sprintf(c, "   %d.%03d.%03d Hz ", mhz, khz, hz);
        
  printLine1(c);

  mhz = tuneStep / 1000000;
  khz = (tuneStep / 1000) % 1000;
  hz = tuneStep  % 1000;

  if (hz)
    sprintf(c, "%cSB     [%3d Hz]", (upperSideBand ? 'U' : 'L'), hz);
  else if (khz)
    sprintf(c, "%cSB [%3d kHz]     ", (upperSideBand ? 'U' : 'L'), khz);

  printLine2(c);
}

void updateBFO() {
  bfoFreq = upperSideBand
    ? BFO_CENTER + BFO_SHIFT
    : BFO_CENTER - BFO_SHIFT;
  
  si5351.set_freq(bfoFreq * 100ULL, SI5351_CLK0);
}

void setFrequency(unsigned long f) {
  si5351.set_freq((bfoFreq + f + CALIBRATION) * 100ULL, SI5351_CLK2);

  frequency = f;
}

void updateEncoder(){
  long old_freq = frequency;
  
  int MSB = digitalRead(ENCODER_PIN1); // MSB = most significant bit
  int LSB = digitalRead(ENCODER_PIN2); // LSB = least significant bit

  int encoded = (MSB << 1) |LSB; // Converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; // Adding it to the previous encoded value

  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    encoderValue--;
    
  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    encoderValue++;

  long diff = encoderValue - lastEncoderValue;
  if (abs(diff) >= ENCODER_SENSIVITY) {
    if (diff > 0) {
      if (frequency + tuneStep < HIGHEST_FREQ) {
        frequency -= frequency % tuneStep;
        frequency += tuneStep;
      }
      else
        frequency = HIGHEST_FREQ;
    }
    else {
      if (frequency - tuneStep > LOWEST_FREQ) {
        frequency -= frequency % tuneStep;
        frequency -= tuneStep;
      }
      else
        frequency = LOWEST_FREQ;
    }
    lastEncoderValue = encoderValue;
  }

  if (frequency != old_freq) {
    updateNeeded = true;
  }

  lastEncoded = encoded; // Store this value for next time
}

void updateFrequency() {
  updateNeeded = false;
  setFrequency(frequency);
  updateDisplay();
}

void switchSideBand() {
  upperSideBand = !upperSideBand;
  updateBFO();
  updateFrequency();
  updateDisplay();
}

void checkPress() {
  int pressed4 = digitalRead(SWITCH4_PIN);
  
  if (pressed4 != lastPressed4) {
    if (pressed4 == HIGH) {
      if (tuneStep < 1000)
        tuneStep *= 10;
      else
        tuneStep = 1;
        
      updateDisplay();
    }
    
    lastPressed4 = pressed4;
  }

  int pressed3 = digitalRead(SWITCH3_PIN);
  
  if (pressed3 != lastPressed3) {
    if (pressed3 == HIGH)
      switchSideBand();
    
    lastPressed3 = pressed3;
  }
}

void setup() {
  pinMode(ENCODER_PIN1, INPUT_PULLUP); 
  pinMode(ENCODER_PIN2, INPUT_PULLUP);
  pinMode(SWITCH4_PIN, INPUT_PULLUP);
  pinMode(SWITCH3_PIN, INPUT_PULLUP);
  
  lcd.begin(16, 2);
  printBuff[0] = 0;

  Serial.begin(9600);
  analogReference(DEFAULT);
  
  Serial.println("*Raduino booting up\n(v0.01)\n");
  delay(100);
  
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 25000000l, 0);
  Serial.println("*Initiliazed Si5351");
  
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLB);
  Serial.println("*Fixed PLL");
  
  si5351.output_enable(SI5351_CLK0, 1);
  si5351.output_enable(SI5351_CLK1, 0);
  si5351.output_enable(SI5351_CLK2, 1);
  Serial.println("*Output enabled PLL");

  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_2MA);
  si5351.set_freq(500000000l,  SI5351_CLK2); // CLK2 used for VFO frequency
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);
  si5351.set_freq(bfoFreq * 100ULL, SI5351_CLK0); // CLK0 used for BFO frequency
  Serial.println("*Si5354 ON\n");
  delay(10);

  frequency = START_FREQ;
  attachInterrupt(0, updateEncoder, CHANGE);
  attachInterrupt(1, updateEncoder, CHANGE);
  Serial.println("*Initialized rotary encoder\n");
}

void loop() {
  if (updateNeeded)
    updateFrequency();
    
  checkPress();
}

