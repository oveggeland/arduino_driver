#pragma once

// STD libs
#include <SPI.h>

// Imported libs
#include <ICM_20948.h>
#include <RingBuf.h>

// Code imports
#include "ADIS16480.h"
#include "network.h"
#include "sync.h"

#define IMU_CS_PIN 7
#define IMU_DR_PIN 2

#define IMU_SPI_SETTINGS SPISettings(14000000, MSBFIRST, SPI_MODE3)

typedef struct{
  uint32_t t_sec;
  uint32_t t_usec;
  int16_t rate[3];
  int16_t acc[3];
} imuData; // Struct to keep data from ADIS (used in interrupts)

#pragma pack(1)
typedef struct {
  char header[4] = {'$', 'I', 'M', 'U'};
  uint32_t t_sec;
  uint32_t t_usec;
  float acc[3];
  float rate[3];
} imuPackage;

void imuSetup();
void imuReset();
void imuUpdate();