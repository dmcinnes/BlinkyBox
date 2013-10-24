/*

   Blinky Box Programmable Toy
   by Miria Grunick

   This is the Teensy code for my Blinky Box project here: http://blog.grunick.com/blinky-box/

   This code was written for the Teensy 3. The button pins will need to change if you
   want to use an earlier version of Teensy.

   This is free to use and modify!
 */


#include "LPD8806.h"
#include <avr/sleep.h>

#define BODS 7                   //BOD Sleep bit in MCUCR
#define BODSE 2                  //BOD Sleep enable bit in MCUCR

// LED constants
const unsigned int nLEDs    = 16;
const unsigned int dataPin  = 6;
const unsigned int clockPin = 7;
const unsigned int maxPower = 10;  // maximum brightness of LEDs

// Constants used for rainbows
const int NUM_COLORS = 16;
const int rainbow_r[] = {13, 13, 13, 13, 13,  6,   0,   0,   0,   0,   0,  2,  4,  8, 13, 13};
const int rainbow_g[] = {  0,  2,  4,  8, 13, 13, 13, 13, 13,  6,   0,   0,   0,   0,   0,   0};
const int rainbow_b[] = {  0,   0,   0,   0,   0,   0,   0,  3, 13, 13, 13, 13, 13,  8,  4,  2};


// Button constants
const unsigned int whitePin  = 0;
const unsigned int redPin    = 1;
const unsigned int yellowPin = 2;
const unsigned int greenPin  = 3;
const unsigned int bluePin   = 4;

const unsigned int knobButtonPin = 5;

const unsigned int colorMap[] = {whitePin, redPin, yellowPin, greenPin, bluePin};

// Knob constants
const unsigned int encoderPinOne = 9;
const unsigned int encoderPinTwo = 10;
const unsigned int numPatterns   = 6;

LPD8806 strip = LPD8806(nLEDs, dataPin, clockPin);

volatile int color = 0;
volatile int disco = 0;

volatile int pattern = 1;

volatile int alternateState = 0;
volatile int rainbowState = 0;
volatile int chaseState = 0;
volatile int fadeState = 0;

unsigned long currentMillis = 0;
unsigned long lastMillis    = 0;
unsigned long lastActivity  = 0;

void setup() {

  // arcade buttons
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);

  // knob button
  pinMode(5, INPUT_PULLUP);

  // rotary knob
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);

  // set pins 0-5 to fire interrupts for buttons
  PCMSK0 = (1<<PCINT0) | (1<<PCINT1) | (1<<PCINT2) | (1<<PCINT3) | (1<<PCINT4) | (1<<PCINT5);

  // set pins 9 & 10 to fire interrupts for the rotary knob
  PCMSK1 = (1<<PCINT8) | (1<<PCINT9); // PCINT8 is pin 10, PCINT9 is pin 9

  // Enable PCINT interrupts 0-7 and 8-11
  GIMSK = (1<<PCIE0) | (1<<PCIE1);

  // Global interrupt enable
  sei();

  strip.begin();
  strip.show();
  solidLights(127, 127, 127);
  /*Serial.println("End setup!");*/

  lastActivity = millis();
}

void goToSleep(void) {
  // turn off the lights
  solidLights(0, 0, 0);

  ADCSRA &= ~_BV(ADEN); // disable ADC
  ACSR   |= _BV(ACD);   // disable the analog comparator

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  // turn off the brown-out detector.
  // must have an ATtiny45 or ATtiny85 rev C or later for software to be able to disable the BOD.
  // current while sleeping will be <0.5uA if BOD is disabled, <25uA if not.
  cli();
  uint8_t mcucr1 = MCUCR | _BV(BODS) | _BV(BODSE);  // turn off the brown-out detector
  uint8_t mcucr2 = mcucr1 & ~_BV(BODSE);
  MCUCR = mcucr1;
  MCUCR = mcucr2;
  sei();                         // ensure interrupts enabled so we can wake up again
  sleep_cpu();                   // go to sleep
  cli();                         // wake up here, disable interrupts
  sleep_disable();

  sei();                         // enable interrupts again

  ADCSRA |= _BV(ADEN);   // enable ADC
  ACSR   &= ~_BV(ACD);   // enable the analog comparator
}


void setIfPresent(int idx, int r, int g, int b) {
  // Don't write to non-existent pixels
  if (idx >= 0 && idx < strip.numPixels()) {
    strip.setPixelColor(idx, r, g, b);
  }
}

void raindropLights(int wait, int r, int g, int b) {
  clearStrip();
  int i = chaseState;

  setIfPresent(i-6, 0, 0, 0);
  setIfPresent(i-5, r/32, g/32, b/32);
  setIfPresent(i-4, r/16, g/16, b/16);
  setIfPresent(i-3, r/8, g/8, b/8);
  setIfPresent(i-2, r/4, g/4, b/4);
  setIfPresent(i-1, r/2, g/2, b/2);
  setIfPresent(i, r, g, b );
  strip.show();
  delay(wait/nLEDs);

  chaseState = chaseState + 1;
  if (chaseState == nLEDs) {
    chaseState = 0;
  }
}

void solidLights(int r, int g, int b) {
  int i;
  for (i=strip.numPixels()-1; i>=0; i--) {
    strip.setPixelColor(i, r, g, b);
  }
  strip.show();
}

void onOffLights(int wait, int r, int g, int b) {
  clearStrip();
  int i;
  for (i=0; i< strip.numPixels() ; i++) {
    strip.setPixelColor(i, r, g, b);
    strip.show();
    delay(20);
  }
  delay(wait);
  for (i=strip.numPixels(); i >= 0; i--) {
    strip.setPixelColor(i, 0, 0, 0);
    strip.show();
    delay(20);
  }
  delay(wait);
}

void fadeLights(int wait, int r, int g, int b) {
  const int transitions = 15;
  int j;

  int brightness = 0;
  if (fadeState < transitions) {
    brightness = fadeState;
  } else {
    brightness = transitions - (fadeState-transitions);
  }
  float percentage = float(brightness)/float(transitions);
  for (j=0; j<strip.numPixels(); j++) {
    strip.setPixelColor(j, r * percentage, g * percentage, b *percentage);
  }
  strip.show();
  delay(wait/transitions);
  fadeState = fadeState + 1;
  if (fadeState == 30)
    fadeState = 0;
}

void twinkleLights(int wait, int r, int g, int b) {
  clearStrip();
  int i, j, pixel;
  const int transitions = 15;
  const int numTwinkles = 1;
  for (j = 0; j < numTwinkles; j++) {
    pixel = random(0, strip.numPixels());

    // Fade in
    for (i=0; i<transitions; i++) {
      float percentage = float(i)/float(transitions);
      strip.setPixelColor(pixel, r * percentage, g * percentage, b*percentage);
      strip.show();
      delay(wait/numTwinkles/transitions);
    }
    // Fade out
    for (i=transitions; i>=0; i--) {
      float percentage = float(i)/float(transitions);
      strip.setPixelColor(pixel, r *percentage, g *percentage, b*percentage);
      strip.show();
      delay(wait/numTwinkles/transitions);
    }
  }
}

void discoRainbowLights(int wait) {
  int i;
  int j;
  int state = rainbowState;

  for (j=0; j<NUM_COLORS; j++) {
    state = j;
    for (i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, rainbow_r[state], rainbow_g[state], rainbow_b[state]);
      state = state + 1;
      if (state == NUM_COLORS)
        state = 0;
    }
    strip.show();
    delay(wait/NUM_COLORS);
  }

  rainbowState = rainbowState + 1;
  if (rainbowState == NUM_COLORS) {
    rainbowState = 0;
  }
}

void alternateLights(int wait, int r, int g, int b) {
  int i;
  int state = alternateState;
  for (i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, r*state, g*state, b*state);
    if (state == 1) {
      state = 0;
    } else {
      state = 1;
    }
  }
  strip.show();
  delay(wait);

  if (alternateState == 1) {
    alternateState = 0;
  } else {
    alternateState = 1;
  }
}

void clearStrip() {
  int j;
  for (j=0; j<strip.numPixels(); j++) {
    strip.setPixelColor(j, 0, 0, 0);
  }
  strip.show();
}


void changeLights(int pattern, int r, int g, int b) {
  switch(pattern) {
    case 0:
      solidLights(r, g, b);
      break;
    case 1:
      fadeLights(1000, r, g, b);
      break;
    case 2:
      twinkleLights(30, r, g, b);
      break;
    case 3:
      raindropLights(1000, r, g, b);
      break;
    case 4:
      alternateLights(1000, r, g, b);
      break;
    case 5:
      onOffLights(1000, r, g, b);
      break;
  }
}

void loop() {
  currentMillis = millis();
  if (currentMillis - lastActivity > 300000) { // 5 minutes
    goToSleep();
  }

  // Change LEDs based on state
  if (disco == 1) {
    discoRainbowLights(5000);
    disco = 0;
  } else {
    if (color == 0) {
      changeLights(pattern, maxPower, maxPower, maxPower);
    } else if (color == 1) {
      changeLights(pattern, maxPower, 0, 0);
    } else if (color == 2) {
      changeLights(pattern, maxPower, maxPower, 0);
    } else if (color == 3) {
      changeLights(pattern, 0, maxPower, 0);
    } else if (color == 4) {
      changeLights(pattern, 0, 0, maxPower);
    }
  }
}

// Buttons
// Pins 0-7
ISR(PCINT0_vect) {
  lastActivity = millis();

  if (digitalRead(knobButtonPin) == LOW) {
    disco = 1;
  }

  // arcade buttons
  for (unsigned int i = 0; i < 5; i++) {
    if (digitalRead(colorMap[i]) == LOW) {
      color = i;
      break;
    }
  }
}

// from here
// http://www.circuitsathome.com/mcu/rotary-encoder-interrupt-service-routine-for-avr-micros
static uint8_t old_AB = 3;  //lookup table index
static int8_t encval = 0;   //encoder value
static const int8_t enc_states [] PROGMEM = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};  //encoder lookup table

// Rotary Encoder
// Pins 8-11
ISR(PCINT1_vect) {
  lastActivity = millis();

  /**/
  old_AB <<=2;  //remember previous state
  old_AB |= ( PINB & 0x03 ); // reading PB0 and PB1, pins 10 and 9
  encval += pgm_read_byte(&(enc_states[( old_AB & 0x0f )]));

  if ( encval > 3 ) {  //four steps forward
    pattern = (pattern + 1) % numPatterns;
    encval = 0;
  } else if ( encval < -3 ) {  //four steps backwards
    pattern = (pattern - 1) % numPatterns;
    encval = 0;
  }
}
