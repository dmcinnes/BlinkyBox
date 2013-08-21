/*

   Blinky Box Programmable Toy
   by Miria Grunick

   This is the Teensy code for my Blinky Box project here: http://blog.grunick.com/blinky-box/

   This code was written for the Teensy 3. The button pins will need to change if you
   want to use an earlier version of Teensy.

   This is free to use and modify!
 */

// Encoder doesn't know about attiny84
// #define CORE_NUM_INTERRUPT 1
// #define CORE_INT0_PIN      2

#include "LPD8806.h"
#include "Encoder.h"

// LED constants
const unsigned int nLEDs = 16;
const unsigned int dataPin = 2;
const unsigned int clockPin = 3;
const unsigned int maxPower = 10;  // maximum brightness of LEDs

// Constants used for rainbows
const int NUM_COLORS = 16;
const int rainbow_r[] = {127, 127, 127, 127, 127,  64,   0,   0,   0,   0,   0,  20,  40,  83, 127, 127};
const int rainbow_g[] = {  0,  20,  40,  83, 127, 127, 127, 127, 127,  64,   0,   0,   0,   0,   0,   0};
const int rainbow_b[] = {  0,   0,   0,   0,   0,   0,   0,  32, 127, 127, 127, 127, 127,  83,  40,  20};

// Button constants
const unsigned int whitePin  = 6;
const unsigned int redPin    = 7;
const unsigned int yellowPin = 8;
const unsigned int greenPin  = 9;
const unsigned int bluePin   = 10;

const unsigned int knobPin   = 5;

// Knob constants
const unsigned int encoderPinOne = 0;
const unsigned int encoderPinTwo = 1;
const unsigned int numPatterns   = 6;

LPD8806 strip = LPD8806(nLEDs, dataPin, clockPin);

volatile int color = 0;
volatile int disco = 0;

volatile int pattern = 1;
volatile int prevKnobState = 0;

Encoder knob(encoderPinOne, encoderPinTwo);

volatile int alternateState = 0;
volatile int rainbowState = 0;
volatile int chaseState = 0;
volatile int fadeState = 0;

void setup() {

  // arcade buttons
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);

  // set pins 6 & 7 to fire interrupts
  PCMSK0 |= (1<<PCINT7) | (1<<PCINT6);

  // set pins 8, 9 & 10 to fire interrupts
  PCMSK1 |= (1<<PCINT8) | (1<<PCINT9) | (1<<PCINT10);

  // Enable PCINT interrupts 0-7 and 8-11
  GIMSK |= (1<<PCIE0) | (1<<PCIE1);

  // Global interrupt enable
  sei();

  strip.begin();
  strip.show();
  solidLights(127, 127, 127);
  /*Serial.println("End setup!");*/
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
  }
  delay(wait);
  for (i=strip.numPixels(); i >= 0; i--) {
    strip.setPixelColor(i, 0, 0, 0);
    strip.show();
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
  for (j=0; j<10; j++) {
    for (i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, rainbow_r[state], rainbow_g[state], rainbow_b[state]);
      state = state + 1;
      if (state == NUM_COLORS)
        state = 0;
    }
    strip.show();
    delay(wait/NUM_COLORS);
    rainbowState = rainbowState + 1;
    if (rainbowState == NUM_COLORS)
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
  // Using intervals of 4 because the knob I have is really sensitive.
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
  // Read knob
  long knobState = knob.read();
  if (knobState != prevKnobState ) {
    prevKnobState = knobState;
    pattern = knobState % numPatterns;
  }

  // Change LEDs based on state
  if (disco == 1) {
    discoRainbowLights(1000);
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

// Pins 0-7
ISR(PCINT0_vect) {
  if (digitalRead(6) == LOW) {
    color = 0;
  } else if (digitalRead(7) == LOW) {
    color = 1;
  }
}

// Pins 8-11
ISR(PCINT1_vect) {
  if (digitalRead(8) == LOW) {
    color = 2;
  } else if (digitalRead(9) == LOW) {
    color = 3;
  } else if (digitalRead(10) == LOW) {
    color = 4;
  }
}
