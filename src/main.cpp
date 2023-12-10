// See this page for more details:
// http://alfadex.com/2014/02/dimming-230v-ac-with-arduino-2/

#include "Arduino.h"
#include "pio-attiny84.h"
#include "linear-ramp.h"

// When enabled, unstable power will be simulated by randomly
// turning the TRIAC off for some AC cycles, which adds flickering.
#define hasUnstablePower 0

//#define TEST_MODE

struct dimmer_pins {

    // Input pin which goes HIGH on a zero crossing from the AC line
    short zeroCross;

    // Input pin. When HIGH, the output power will be reduced.
    short dimDarker;

    // Output pin
    short status;

} pin;

// Total number of zero crosses.
// Since millis() does not work due to long ISRs, this code relies on AC cycles
// as measurement of time.
volatile unsigned long acTime = 0;

// After a zero crossing, wait for this amount of µs until firing the TRIAC.
// The higher the value, the darker the light.
volatile int microDelay = 0;

// Count the number of zero crosses since the last loop() call
volatile uint8_t newZeroCrosses = 0;

struct triac_channel {

    // Dim level to use for this TRIAC. 0 = no dimming (on), 1 = off
    float dimLevel;

    // Calculated delay to fire this TRIAC
    int microDelay;

    // Input pin. When HIGH, the TRIAC is enabled.
    short pinEnable;

    // Output pin for firing the TRIAC
    short pinTrigger;

} triac;

struct unstable_light {

    // Last time the light was unstable
    unsigned long tLast;

    // Time to wait until it is unstable again
    unsigned long dtNext;

} instabilities;

#define RAMP_MAX 400
#define RAMP_DIMMED 80
LinearRamp ramp(RAMP_MAX);

const int tOnBT136 = 5;               // BT136 has 2 µs turn-on-time, no need to keep it on for much longer
const float maxBrightness = 1.f;      // Maximum lamp brightness can be reduced with this variable. 0 = off, 1 = full brightness.
const unsigned long tRampUp = 200;    // Ramp-up time in AC cycles aka /100 seconds (fade-in time)



/**
   Measurements:
   Zero crossing impulse is 1.66 ms wide,
   real zero crossing comes about 850 µs after the raising edge
   dt between raising edges is 10 ms
   @param dimLevel Dim levels: 1 = off, 0 = on
*/
const int tZeroPulseWidth = 1700;   // Pulse width of the zero-crossing pulse. Starts a bit before 0 V and ends a bit after 0 V too.
const int tWave = 10000 - tZeroPulseWidth; // What is left between the zero-crossing pulses
const int tMaxDelay = tWave - 200; // Do not fire the triac for less than 500 µs

int calculateMicroDelay(float dimLevel) {
    const int tDelay = int(tWave * dimLevel) + tZeroPulseWidth / 2;
    return tDelay;
}

void isrZeroCross() {

#ifdef TEST_MODE
    digitalWrite(pin.status, HIGH);
    delay(5);
    digitalWrite(pin.status, LOW);
    return;
#endif

    // Safety check: If microDelay is set too high, do not fire at all
    if (microDelay < tMaxDelay) {

        // Wait before firing the TRIAC
        digitalWrite(pin.status, HIGH);
        delayMicroseconds(microDelay);
        digitalWrite(pin.status, LOW);

        // Fire
        digitalWrite(triac.pinTrigger, HIGH);
        delayMicroseconds(tOnBT136);
        digitalWrite(triac.pinTrigger, LOW);

    }

    newZeroCrosses++;
}

void calculateDimLevel(triac_channel *a, float brightness, bool powerIsUnstable) {

    if (powerIsUnstable) {
        a->dimLevel = 1; // Currently, we are just simulating a short power outage
    } else {
        a->dimLevel = 1 - brightness;
    }

    a->microDelay = calculateMicroDelay(a->dimLevel);
}

void setup() {
    pin.zeroCross = PB2;
    pin.dimDarker = PA7;
    pin.status = PB0;

    triac.pinEnable = PA3;
    triac.pinTrigger = PA1;

    pinMode(pin.zeroCross, INPUT);
    pinMode(pin.dimDarker, INPUT);
    pinMode(pin.status, OUTPUT);

    pinMode(triac.pinEnable, INPUT);
    pinMode(triac.pinTrigger, OUTPUT);
    digitalWrite(triac.pinTrigger, LOW);

    attachInterrupt(0, isrZeroCross, RISING);

    // Initialise instabilities with some more or less random values
    instabilities.tLast = 0;
    instabilities.dtNext = 100; // Once a second

    for (int i = 0; i < 5; i++){
        digitalWrite(pin.status, HIGH);
        delay(100);
        digitalWrite(pin.status, LOW);
        delay(100);
    }
}


void loop() {

    bool unstablePower = false;
    byte addedZeroCrosses;

    // Variable is written in interrupt, disable interrupt so
    // the read is atomic
    noInterrupts();
    addedZeroCrosses = newZeroCrosses;
    newZeroCrosses = 0;
    interrupts();

    acTime += addedZeroCrosses;


#if hasUnstablePower
    if ( (acTime - instabilities.tLast) > instabilities.dtNext ) {
      unstablePower = true;
      instabilities.tLast = acTime;
      instabilities.dtNext = random(200, 2000);
    }
#endif


    bool doEnable = digitalRead(triac.pinEnable) == HIGH;
    bool doDimDown = digitalRead(pin.dimDarker) == HIGH;

    ramp.setTarget(doEnable ? (doDimDown ? RAMP_DIMMED : RAMP_MAX) : 0);
    ramp.addTicks(addedZeroCrosses);

    calculateDimLevel(&triac, ramp.relativeValue() * maxBrightness, unstablePower);

    // Writing variables with more than 8 bit is not atomic,
    // so disable interrupts because variable is used in interrupt
    noInterrupts();
    microDelay = triac.microDelay;
    interrupts();

#ifndef TEST_MODE
    if (triac.dimLevel == 1) {
        digitalWrite(pin.status, LOW);
    }
#endif

    delay(2);
}


