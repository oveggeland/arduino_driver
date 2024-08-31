#include "imu.h"
#include "gnss.h"
#include "network.h"
#include "sync.h"
#include "ptp.h"
#include "control.h"

#define HEARTBEAT_INTERVAL 2000
uint32_t ts_heartbeat = 0;

void heartbeat(){
  if (millis() - ts_heartbeat > HEARTBEAT_INTERVAL){
    ts_heartbeat = millis();
    Serial.println("Heartbeat");
  }
}

void setup() {
  // Setup serial comms
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Starting Arduino");

  networkSetup();

  ptpSetup();

  gnssSetup();

  imuSetup();
}


void loop() {
  networkUpdate();

  ptpUpdate();

  gnssUpdate();

  imuUpdate();

  controlUpdate();

  heartbeat();
}
