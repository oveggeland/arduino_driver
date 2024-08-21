#include "sync.h" 

ntp_payload ntpReq;
EthernetUDP ntpUDP;

// Global references for estimating timestamps
uint32_t t_ref_local, t_ref_sec, t_ref_usec;

// Sync flag
bool ntp_synced = false;

// Arduino clock drift factor
float drift_factor = DEFAULT_DRIFT_FACTOR;
bool drift_converge = false;


uint32_t getMicros(){
  return drift_factor*micros();
}

uint32_t getMillis(){
  return drift_factor*millis();
}

uint32_t byteArrayToUint32(uint8_t* arr){
  return (arr[0] << 24) | (arr[1] << 16) | (arr[2] << 8) | (arr[3] << 0);
}

uint32_t getSec(ntp_ts ts){
  return byteArrayToUint32(ts.sec);
}

uint32_t getUsec(ntp_ts ts){
  return (uint32_t)((double) byteArrayToUint32(ts.frac) * 1.0e6 / (double)0xFFFFFFFF);
}

int32_t getTimeDiff(uint32_t t0_sec, uint32_t t0_usec, uint32_t t1_sec, uint32_t t1_usec){
  return 1e6*(int32_t)(t1_sec-t0_sec) + (int32_t)(t1_usec - t0_usec);
}


// send an NTP request to the time server at the given address
bool sendNTPRequest(uint32_t &ts) {
  if (!ntpUDP.beginPacket(NTP_SERVER_IP, NTP_SERVER_PORT))
    return false;

  size_t bytesWritten = ntpUDP.write((uint8_t*) &ntpReq, sizeof(ntpReq));
  if (bytesWritten != sizeof(ntpReq))
    return false;

  if (!ntpUDP.endPacket())
    return false;

  ts = micros(); // Save timestamp of transmittion

  return true;
}


bool doSync(){
  // Keep track of transmittion time (in local arduino counter time)
  uint32_t t0_local, t3_local;
  if (!sendNTPRequest(t0_local))
    return false;

  // Wait for socket to receive response
  int bytesAvailable;
  do{
    bytesAvailable = ntpUDP.parsePacket();
    t3_local = micros(); // Time of reception
  } while ((bytesAvailable == 0) && (micros() - t0_local < NTP_TIMEOUT));

  // Read package
  if (bytesAvailable == sizeof(ntpReq)){
    // We've received a packet, read the data from it
    ntp_payload ntpResp;
    ntpUDP.read((uint8_t*) &ntpResp, sizeof(ntpReq)); // read the packet into the buffer

    // TODO: Add assertions to check NTP package quality (ref_id, and other validity tests)
    // TODO: Make some nice assertions for micros() overflow counting (this happens every 70 minutes)

    // 1. Calculate one-way delay (in microseconds)    
    uint32_t t1_ntp_sec = getSec(ntpResp.ts_rec);
    uint32_t t1_ntp_usec = getUsec(ntpResp.ts_rec);

    uint32_t t2_ntp_sec = getSec(ntpResp.ts_trans);
    uint32_t t2_ntp_usec = getUsec(ntpResp.ts_trans);

    uint32_t delay = ((t3_local - t0_local) - getTimeDiff(t1_ntp_sec, t1_ntp_usec, t2_ntp_sec, t2_ntp_usec)) / 2; // Microsecond delay (one-way)

    uint32_t t0_ntp_sec, t0_ntp_usec;
    if (delay > t1_ntp_usec){
      t0_ntp_sec = t1_ntp_sec - 1;
      t0_ntp_usec = t1_ntp_usec - delay + 1e6;
    }
    else{
      t0_ntp_sec = t1_ntp_sec;
      t0_ntp_usec = t1_ntp_usec - delay;
    }

    // Adjust scale factor
    if (ntp_synced){
      uint32_t dt_ntp = getTimeDiff(t_ref_sec, t_ref_usec, t0_ntp_sec, t0_ntp_usec);

      float new_drift_factor = (float)dt_ntp / (t0_local - t_ref_local);

      if (abs(drift_factor - new_drift_factor) < NTP_DRIFT_CONVERGENCE_ATOL)
        drift_converge = true;
      else
        drift_converge = false; // Diverging drift? Weird

      drift_factor = new_drift_factor;
    }
    
    // Update reference values used to get timestamps (Time critical that all are changed at the same time)
    noInterrupts();
    t_ref_sec = t0_ntp_sec;
    t_ref_usec = t0_ntp_usec;
    t_ref_local = t0_local;
    interrupts();

    return true;
  }

  return false;
}


void ntpSetup() {
  ntpUDP.begin(NTP_CLIENT_PORT);

  // Setup global ntpReq
  ntpReq.flags = 0b11100011;
  ntpReq.interval = 6; // Not sure if needed?
  byte myMac[6];
  Ethernet.MACAddress(myMac);
  ntpReq.ref_id = (word(myMac[0], myMac[1]) << 16) | word(myMac[4], myMac[5]); // ID to recognize correct response from server (Construct this from unique MAC)

  // Primary sync
  ntp_synced = doSync();
}


void ntpUpdate() {
  uint32_t interval = NTP_INTERVAL;
  if (!drift_converge)
    interval = 1e6;

  if (micros() - t_ref_local > interval || !ntp_synced){
    if(doSync()){
      Serial.println("NTP sync complete");
      ntp_synced = true;
    }
    else{
      Serial.println("NTP sync failed");
      ntp_synced = false;
    }
  }
}


/*
Fills sec and usec with synchronized values from NTP. Returns false if system clock is not synchronized
*/
bool getCurrentTime(uint32_t &sec, uint32_t &usec){
  noInterrupts();

  usec = t_ref_usec + drift_factor*(micros() - t_ref_local);
  sec = t_ref_sec;

  uint32_t seconds_passed = usec / 1e6;

  usec -= seconds_passed*1e6;
  sec += seconds_passed;
  interrupts();

  // Return status of NTP sync
  if (!ntp_synced){ 
    return false;
  }
  return true;
}

void printTime(uint32_t sec, uint32_t usec){
  Serial.print(sec);
  Serial.print(".");
  if (usec < 10){ Serial.print("00000"); }
  else if (usec < 100){ Serial.print("0000"); }
  else if (usec < 1000){ Serial.print("000"); }
  else if (usec < 10000) { Serial.print("00"); }
  else if (usec < 100000){ Serial.print("0"); }
  Serial.print(usec);
}
