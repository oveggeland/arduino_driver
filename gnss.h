#pragma once

#include <Wire.h>
#include <SparkFun_u-blox_GNSS_v3.h>

#include "network.h"
#include "sync.h"

#define WIRE_PORT Wire1
#define GNSS_PPS_PIN 3

#define GNSS_FREQUENCY 5

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