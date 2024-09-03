#include "imu.h"
#include "gnss.h"
#include "network.h"
#include "sync.h"
#include "ptp.h"
#include "control.h"

void setup() {
  // Setup serial comms
  // Serial.begin(115200);
  // while (!Serial);

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
}
