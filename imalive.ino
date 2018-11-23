#include "common.h"

// Board: ESP-12F IOT + (Generic ESP8266 Module)
// SKU: 425111
// serial speed: Serial.begin(115200);

// FWDs
void updateNextTime(unsigned long *nextTime, unsigned long increment);

State state;

// ----------------------------------------

void initGlobals() {
    memset(&state, 0, sizeof(state));

    state.initIsDone = false;

    updateNextTime(&state.next1000time, 1000);
    updateNextTime(&state.next10mintime, (60000 * 10));
}

void  initButtonPin() {
    pinMode(4, INPUT);
}

void setup() {
#ifdef DEBUG
    Serial.begin(115200); // (921600);
#endif

    // stage 1
    initGlobals();

    // stage 2
    initButtonPin();

    // stage 3
    initMyMqtt();

#ifdef DEBUG
    Serial.println("imalive init finished");
    Serial.print("sizeof(int) == "); Serial.println(sizeof(int), DEC);
    Serial.print("sizeof(unsigned long) == "); Serial.println(sizeof(unsigned long), DEC);
    Serial.print("sizeof(unsigned long long) == "); Serial.println(sizeof(unsigned long long), DEC);
#endif
    state.initIsDone = true;
}

void loop() {
    const unsigned long now = millis();

    while (true) {
        myMqttLoop();

        // check it it is time to do something interesting. Note that now will wrap and we will
        // get a 'blib' every 50 days. Who cares, right?
        //
        if (state.next1000time <= now) {   // 1 sec
            mqtt1SecTick();
            updateNextTime(&state.next1000time, 1000); continue;
        } else if (state.next10mintime <= now) {  // 10 mins
            mqtt10MinTick();
            updateNextTime(&state.next10mintime, (60000 * 10)); continue;
        }

        break;
    } // while
}

// -----

void updateNextTime(unsigned long *nextTimePtr, unsigned long increment) {
    unsigned long nextTime;

    while (true) {
        const unsigned long now = millis();
        nextTime = now + increment;

        // handle wrap by sleeping
        if (nextTime < now) {
#ifdef DEBUG
            Serial.print("updateNextTime hit wrap");
#endif
            delay(1000);
            continue;
        }

        break;
    }
    *nextTimePtr = nextTime;
}

