#pragma once

#include "network.h"
#include "sync.h"
#include "ptp.h"
#include "imu.h"
#include "gnss.h"

#define STATUS_INTERVAL 500
#define CMD_HEADER  {'$', 'C', 'M', 'D'}

void controlUpdate();

#pragma pack(1)
typedef struct {
  char header[3] = {'$', 'S', 'T'};

  // Timing
  uint32_t t_sec;
  uint32_t t_usec;
  uint32_t age; // Local time (arduino clock)
  int32_t sync_offset;

  // PTP
  bool ptp_active;
  uint32_t ptp_interval;

  // IMU
  bool imu_active;
  uint8_t imu_sr; 

  // GNSS
  bool gnss_active;
  uint8_t gnss_sr;
} arduinoStatus;

typedef struct {
  char header[4];
  
  bool reset;

  // NTP stuff
  uint32_t ntp_interval;

  // PTP
  bool ptp_active;
  uint32_t ptp_interval;

  // IMU
  bool imu_active;
  uint8_t imu_sr; 

  // GNSS
  bool gnss_active;
  uint8_t gnss_sr;
} arduinoCommand;