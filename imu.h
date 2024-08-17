#pragma once

// STD libs
#include <SPI.h>

// Imported libs
#include <ICM_20948.h>

// Code imports
#include "ADIS16480.h"
#include "network.h"
#include "sync.h"

#define IMU_CS_PIN 7
#define IMU_DR_PIN 2

#define IMU_SPI_SETTINGS SPISettings(14000000, MSBFIRST, SPI_MODE3)
#define IMU_SAMPLE_RATE 200

#pragma pack(1)
typedef struct {
  char header[4] = {'$', 'I', 'M', 'U'};
  uint32_t t_sec;
  uint32_t t_usec;
  int16_t acc[3];
  int16_t rate[3];
} imuPackage;

void imuSetup();
void imuReset();
void imuUpdate();