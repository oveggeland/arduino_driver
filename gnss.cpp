#include "gnss.h"

SFE_UBLOX_GNSS myGNSS;

// Used for GNSS synchronization of system
uint32_t t_local_ref = 0;
bool pps_fix = false;
bool pps_enable = false;

uint32_t t_last_connection = 0;
uint32_t t_reset = 0;
bool gnss_reset = false;
uint16_t gnss_reset_count = 0;

void gnssISR(void) {
  if (pps_enable){
    t_local_ref = micros();
    pps_fix = true;
  }
}


void gnssReset(){
  gnss_reset = true;
  digitalWrite(GNSS_RESET_PIN, LOW);
  delay(20);
  digitalWrite(GNSS_RESET_PIN, HIGH);
  t_reset = millis();
  gnss_reset_count++;
}


bool gnssConfig(){
  WIRE_PORT.begin(); // Begin i2c
  if (!myGNSS.begin(WIRE_PORT)) //Connect to the u-blox module
    return false;

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
  myGNSS.addCfgValset(UBLOX_CFG_TP_LEN_LOCK_TP1, 100000); // Set the pulse length (us) after lock complete

  myGNSS.addCfgValset(UBLOX_CFG_TP_TP1_ENA, 1); // Make sure the enable flag is set to enable the time pulse. (Set to 0 to disable.)
  myGNSS.addCfgValset(UBLOX_CFG_TP_USE_LOCKED_TP1, 1); // Tell the module to use PERIOD while locking and PERIOD_LOCK when locked to GNSS time
  myGNSS.addCfgValset(UBLOX_CFG_TP_PULSE_DEF, 0); // Tell the module that we want to set the period (not the frequency). PERIOD = 0. FREQ = 1.
  myGNSS.addCfgValset(UBLOX_CFG_TP_PULSE_LENGTH_DEF, 1); // Tell the module to set the pulse length (not the pulse ratio / duty). RATIO = 0. LENGTH = 1.
  myGNSS.addCfgValset(UBLOX_CFG_TP_POL_TP1, 0); // Tell the module that we want the falling edge at the top of second. Falling Edge = 0. Rising Edge = 1.

  if (!myGNSS.sendCfgValset())
    return false;
  
  if (!myGNSS.setNavigationFrequency(GNSS_DEFAULT_SAMPLE_RATE))
    return false;

  if (!myGNSS.setAutoPVT(true))
    return false;

  return true;
}


void gnssSetup(){
  // Set reset pin high (reset is active low)
  pinMode(GNSS_RESET_PIN, OUTPUT);
  digitalWrite(GNSS_RESET_PIN, HIGH);

  // Setup a interrupt at for PPS monitoring
  pinMode(GNSS_PPS_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(GNSS_PPS_PIN), gnssISR, FALLING);  // Set up a falling interrupt

  // This is also called after HW resets
  gnssConfig();
}

/*
This is assumed to be called at least once for each GNSS measurement. 
If this is not the case, measurements will be missed and time sync might be corrupted. 
*/
void gnssUpdate(){
  if (gnss_reset){
    if(gnssConfig()){
      gnss_reset = false;
      t_last_connection = millis();
    }
  }

  if ((!gnss_reset || millis() - t_reset > GNSS_RESET_TIME) && millis() - t_last_connection > GNSS_TIMEOUT){
    pps_enable = false;
    pps_fix = false;
    gnssReset();
    return;
  }

  if (myGNSS.getPVT() == true){
    t_last_connection = millis();

    if (myGNSS.getFixType() == 3){
      pps_enable = true;
      if (pps_fix){
        newGNSSReference(t_local_ref, timeval{myGNSS.getUnixEpoch(), 0});
        pps_fix = false;
      }

      gnssPackage myGnssPkg;
      myGnssPkg.t_sec = myGNSS.getUnixEpoch(myGnssPkg.t_usec);
    
      // Retrieve position
      myGnssPkg.latitude = myGNSS.getLatitude();
      myGnssPkg.longitude = myGNSS.getLongitude();
      myGnssPkg.altitude = myGNSS.getAltitude();

      networkPushData((uint8_t*) &myGnssPkg, sizeof(myGnssPkg)); // Push print buffer (used for debugging)
    }
  }
}

uint8_t gnssGetSampleRate(){
  myGNSS.getNavigationFrequency();
}

void gnssSetSampleRate(uint8_t sr){
  myGNSS.setNavigationFrequency(sr);
}

bool gnssGetResetFlag(){
  return gnss_reset;
}

uint16_t gnssGetResetCount(){
  return gnss_reset_count;
}