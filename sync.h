/*
Used to synchronize the system to a gnss pps reference
*/

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <Arduino.h>

#define NTP_DRIFT_ATOL 0.1 // Tolerance for drift deviation (away from 1.0 corresponding to no drift)
#define DEFAULT_DRIFT_FACTOR 1.003

// This struct defines the system wide time
typedef struct{
  uint32_t sec;
  uint32_t usec;
} timeval;

void getCurrentTime(uint32_t &sec, uint32_t &usec);
void getCurrentTime(timeval &t);
timeval getCurrentTime();

void printTime(uint32_t sec, uint32_t usec);
void printTime(timeval t);

int32_t getTimeDiff(uint32_t t0_sec, uint32_t t0_usec, uint32_t t1_sec, uint32_t t1_usec);
int32_t getTimeDiff(timeval t0, timeval t1);

void newGNSSReference(uint32_t local_ref, timeval global_ref);
int32_t getSyncOffset();