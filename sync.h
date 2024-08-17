/*
NTP and PTP configurations
*/

#pragma once

#include <stdint.h>

#include <Ethernet.h>
#include <EthernetUdp.h>

#define NTP_SERVER_PORT 123
#define NTP_SERVER_IP IPAddress(192,168,1,56)
#define NTP_CLIENT_PORT 1023

#define NTP_INTERVAL 60 

typedef struct{
  uint8_t sec[4];
  uint8_t frac[4];
} ntp_ts;

#pragma pack(1) // So struct can be send straight to UDP (no alignment bytes)
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

void getCurrentTime(uint32_t &sec, uint32_t &usec);
void printTime(uint32_t sec, uint32_t usec);
int32_t getTimeDiff(uint32_t t0_sec, uint32_t t0_usec, uint32_t t1_sec, uint32_t t1_usec);