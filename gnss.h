#pragma once

#include <Wire.h>
#include <SparkFun_u-blox_GNSS_v3.h>

#include "network.h"
#include "sync.h"

#define WIRE_PORT Wire1
#define GNSS_PPS_PIN 3
#define GNSS_RESET_PIN 8

#define GNSS_TIMEOUT 10000
#define GNSS_RESET_TIME 60000
#define GNSS_DEFAULT_SAMPLE_RATE 1

#pragma pack(1)
typedef struct {
  char header[5] = {'$', 'G', 'N', 'S', 'S'};
  uint32_t t_sec;
  uint32_t t_usec;
  int32_t latitude;
  int32_t longitude;
  int32_t altitude;
} gnssPackage;

/**
* The three standard functionalities every module should have. 
* 1. Setup on power-up
* 2. Reset for error-handling (NOT MATURE)
* 3. Update is called in the main loop to perform relevant tasks
*/
void gnssSetup(); 
void gnssReset();
void gnssUpdate();
bool gnssGetResetFlag();
uint16_t gnssGetResetCount();

uint8_t gnssGetSampleRate();
void gnssSetSampleRate(uint8_t sr);
