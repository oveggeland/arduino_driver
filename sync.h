/*
NTP and PTP configurations
*/

#pragma once

#include <stdint.h>

#include "network.h"

#define NTP_SERVER_PORT 123
#define NTP_SERVER_IP IPAddress(10,1,30,250)
#define NTP_CLIENT_PORT 1023

#define DEFAULT_NTP_INTERVAL 10e6 // Microseconds
#define NTP_TIMEOUT 10e3 // Microseconds
#define NTP_DRIFT_ATOL 0.1 // Tolerance for drift deviation (away from 1.0 corresponding to no drift)

#define NTP_OFFSET 2208988800

#define DEFAULT_DRIFT_FACTOR 1.003

// This struct defines the system wide time
typedef struct{
  uint32_t sec;
  uint32_t usec;
} timeval;

#pragma pack(1) // So struct can be send straight to UDP (no alignment bytes)
typedef struct{
  uint8_t sec[4];
  uint8_t frac[4];
} ntp_ts;

typedef struct{
  uint8_t flags;
  uint8_t stratum;
  uint8_t interval;
  uint8_t precision;
  uint32_t root_delay;
  uint32_t root_disp;
  uint32_t ref_id;
  ntp_ts ts_ref;
  ntp_ts ts_orig;
  ntp_ts ts_rec;
  ntp_ts ts_trans;
} ntp_payload;

void ntpSetup();
void ntpReset();
void ntpUpdate();

uint32_t ntpGetInterval();
void ntpSetInterval(uint32_t interval_ms);
int32_t ntpGetOffset();

void getCurrentTime(uint32_t &sec, uint32_t &usec);
void getCurrentTime(timeval &t);
timeval getCurrentTime();

void printTime(uint32_t sec, uint32_t usec);
void printTime(timeval t);

int32_t getTimeDiff(uint32_t t0_sec, uint32_t t0_usec, uint32_t t1_sec, uint32_t t1_usec);
int32_t getTimeDiff(timeval t0, timeval t1);