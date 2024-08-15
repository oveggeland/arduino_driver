#include "imu.h"
#include "gnss.h"
#include "network.h"

#include <WDT.h>

#define WATCHDOG_INTERVAL 3000 // Milliseconds (This should be bigger than the amount of time it takes to initialize all modules)

#define HEARTBEAT_INTERVAL 1000
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

  if(!WDT.begin(WATCHDOG_INTERVAL)) {
    Serial.println("Error initializing watchdog");
    NVIC_SystemReset(); 
  }

  /*
  NETWORK MUST BE SETUP BEFORE IMU. THIS IS BECAUSE IMU INTERRUPTS MIGHT AFFECT THE SETUP ROUTINE
  */
  networkSetup();

  // GNSS setup
  gnssSetup();

  // Imu stuff
  imuSetup();
}


void loop() {
  // put your main code here, to run repeatedly:
  imuUpdate();
  gnssUpdate();

  networkUpdate();

  heartbeat();
  WDT.refresh();
}
