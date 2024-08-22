#pragma once

#include "network.h"
#include "sync.h"
#include "ptp.h"
#include "imu.h"
#include "gnss.h"

#define STATUS_INTERVAL 500

void controlUpdate();

#pragma pack(1)
typedef struct {
  char header[3] = {'$', 'S', 'T'};
  
  // Network configuration
  // uint8_t local_ip[4];
  // uint8_t local_port;
  // uint8_t remote_ip[4];
  // uint8_t remote_port;

  // Timestamp
  uint32_t t_sec;
  uint32_t t_usec;
  
  // Time since last reset of arduino
  uint32_t age;
  
  // NTP stuff
  uint32_t ntp_interval;
  int32_t ntp_offset;

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