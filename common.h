#ifndef _COMMON_H

#define _COMMON_H

#include <inttypes.h>

// FIXME: turn debug off
// #define DEBUG 1

// FWS decls... mqtt_client
void initMyMqtt();
void myMqttLoop();
void mqtt1SecTick();
void mqtt10MinTick();
void sendOperState();

typedef struct {
    bool initIsDone;

    uint32_t mqtt_ticks;
    uint32_t mqtt_oper_mins;

    unsigned long next1000time;   // 1 second timer
    unsigned long next10mintime;  // 10 min timer
} State;

extern State state;

#endif // _COMMON_H
