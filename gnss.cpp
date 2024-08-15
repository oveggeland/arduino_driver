#include "gnss.h"

SFE_UBLOX_GNSS myGNSS;

uint32_t ts_pps_arduino; // Local microsecond clock
uint32_t ts_pps_gnss;    // GNSS timers (UNIX seconds, when PPS signal was sent)
bool pps_fix;

gnssPackage myGnssPkg; // Data struct for formating network messages


void gnssISR(void) {
  ts_pps_arduino = micros(); // Capture local time at pps event
  pps_fix = false;
}

void gnssSetup(){
  Serial.println("GNSS setup");

  WIRE_PORT.begin(); // Begin i2c

  while (myGNSS.begin(WIRE_PORT) == false) //Connect to the u-blox module
  {
    Serial.println(F("u-blox GNSS not detected. Retrying..."));
    delay (100);
  }

  myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  
  /* 
  Configure PPS for time synchronization.
  The idea is to stamp the PPS signal, such that this can be used as a reference for stamping subsequent measurements.
  We know that the PPS is sent at the top of a second on the internal U-blox reference clock.
  */
  myGNSS.newCfgValset(VAL_LAYER_RAM); // Create a new Configuration Interface VALSET message. Apply the changes in RAM only (not BBR).

  myGNSS.addCfgValset(UBLOX_CFG_TP_PERIOD_TP1, 0); // Set the period to zero during locking
  myGNSS.addCfgValset(UBLOX_CFG_TP_LEN_TP1, 0); // Set the pulse length to zero during locking

  myGNSS.addCfgValset(UBLOX_CFG_TP_PERIOD_LOCK_TP1, 1000000); // Set the period (us) after lock complete
  myGNSS.addCfgValset(UBLOX_CFG_TP_LEN_LOCK_TP1, 5000); // Set the pulse length (us) after lock complete

  myGNSS.addCfgValset(UBLOX_CFG_TP_TP1_ENA, 1); // Make sure the enable flag is set to enable the time pulse. (Set to 0 to disable.)
  myGNSS.addCfgValset(UBLOX_CFG_TP_USE_LOCKED_TP1, 1); // Tell the module to use PERIOD while locking and PERIOD_LOCK when locked to GNSS time
  myGNSS.addCfgValset(UBLOX_CFG_TP_PULSE_DEF, 0); // Tell the module that we want to set the period (not the frequency). PERIOD = 0. FREQ = 1.
  myGNSS.addCfgValset(UBLOX_CFG_TP_PULSE_LENGTH_DEF, 1); // Tell the module to set the pulse length (not the pulse ratio / duty). RATIO = 0. LENGTH = 1.
  myGNSS.addCfgValset(UBLOX_CFG_TP_POL_TP1, 0); // Tell the module that we want the falling edge at the top of second. Falling Edge = 0. Rising Edge = 1.
  myGNSS.addCfgValset(UBLOX_CFG_TP_ALIGN_TO_TOW_TP1, 1); // Align signal to top of second

  // Now set the time pulse parameters
  if (myGNSS.sendCfgValset() == false)
    Serial.println(F("VALSET failed!"));
  
  myGNSS.setNavigationFrequency(GNSS_FREQUENCY);

  // Setup a interrupt at for PPS monitoring
  pinMode(GNSS_PPS_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(GNSS_PPS_PIN), gnssISR, FALLING);  // Set up a falling interrupt
}

/*
This is assumed to be called at least once for each GNSS measurement. 
If this is not the case, measurements will be missed and time sync might be corrupted. 
*/
void gnssUpdate(){
  if (myGNSS.getPVT(1) == true){
    if (!pps_fix){ // We assume that the first GNSS message we receive is less than 0.5s away from the pps signal (very reasonable as the PPS interrupt is much faster than reading the data solution over i2c)
      ts_pps_gnss = myGNSS.getUnixEpoch(); // Get second when timestamp was registered
      pps_fix = true;
    }

    // Calculate arduino time for gnss acquisition
    uint32_t usecs;
    uint32_t secs = myGNSS.getUnixEpoch(usecs);
    myGnssPkg.ts = ts_pps_arduino + usecs + 1e6*(secs - ts_pps_gnss);

    // Retrieve position
    myGnssPkg.latitude = myGNSS.getLatitude();
    myGnssPkg.longitude = myGNSS.getLongitude();
    myGnssPkg.altitude = myGNSS.getAltitude();

    networkPushData((uint8_t*) &myGnssPkg, sizeof(myGnssPkg)); // Push print buffer (used for debugging)
  }
}