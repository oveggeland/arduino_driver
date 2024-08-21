#include "sync.h" 

ntp_payload ntpReq;
EthernetUDP ntpUDP;

// Global references for estimating timestamps
uint32_t t_ref_local = 0;
timeval t_ref_ntp = {0};

// Sync flag
bool ntp_synced = false;
int32_t ntp_offset_ = 0;

// Arduino clock drift 
float drift_factor = DEFAULT_DRIFT_FACTOR;
bool drift_converge = false;


uint32_t byteArrayToUint32(uint8_t* arr){
  return (arr[0] << 24) | (arr[1] << 16) | (arr[2] << 8) | (arr[3] << 0);
}

uint32_t getSec(ntp_ts ts){
  return byteArrayToUint32(ts.sec);
}

uint32_t getUsec(ntp_ts ts){
  return (uint32_t)((double) byteArrayToUint32(ts.frac) * 1.0e6 / (double)0xFFFFFFFF);
}

timeval getTimeval(ntp_ts ts){
  return timeval{
    getSec(ts),
    getUsec(ts)
  };
}

// send an NTP request to the time server at the given address, return transmit timestamp
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
    timeval t1_ntp = getTimeval(ntpResp.ts_rec);
    timeval t2_ntp = getTimeval(ntpResp.ts_trans);

    uint32_t delay = ((t3_local - t0_local) - getTimeDiff(t1_ntp, t2_ntp)) / 2; // Microsecond delay (one-way)

    timeval t0_ntp;
    if (delay > t1_ntp.usec){
      t0_ntp.sec = t1_ntp.sec - 1;
      t0_ntp.usec = t1_ntp.usec - delay + 1e6;
    }
    else{
      t0_ntp.sec = t1_ntp.sec;
      t0_ntp.usec = t1_ntp.usec - delay;
    }

    // Adjust for UTC offset
    t0_ntp.sec -= NTP_OFFSET;

    // Adjust scale factor
    if (ntp_synced){
      uint32_t dt_ntp = getTimeDiff(t_ref_ntp, t0_ntp);
      uint32_t dt_local = (t0_local - t_ref_local);

      ntp_offset_ = dt_ntp - dt_local*drift_factor;

      float new_drift_factor = (float)dt_ntp / dt_local;

      if (abs(new_drift_factor - 1) < NTP_DRIFT_ATOL){
        drift_factor = new_drift_factor;
        drift_converge = true;
      }
    }
    
    // Update reference values used to get timestamps (Time critical that all are changed at the same time)
    noInterrupts();
    t_ref_ntp = t0_ntp;
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
      Serial.print("NTP sync complete with offset: ");
      Serial.println(ntp_offset_);
      ntp_synced = true;
    }
    else{
      Serial.println("NTP sync failed");
      ntp_synced = false;
    }
  }
}

// Microsecond difference (t1 - t0)
int32_t getTimeDiff(uint32_t t0_sec, uint32_t t0_usec, uint32_t t1_sec, uint32_t t1_usec){
  return 1e6*(int32_t)(t1_sec-t0_sec) + (int32_t)(t1_usec - t0_usec);
}

// Microsecond difference (t1 - t0)
int32_t getTimeDiff(timeval t0, timeval t1){
  return getTimeDiff(t0.sec, t0.usec, t1.sec, t1.usec);
}

// Get current time estimate from sync module
void getCurrentTime(uint32_t &sec, uint32_t &usec){
  usec = t_ref_ntp.usec + drift_factor*(micros() - t_ref_local);
  sec = t_ref_ntp.sec;

  uint32_t seconds_passed = usec / 1e6;

  usec -= seconds_passed*1e6;
  sec += seconds_passed;
}

// Get current time estimate from sync module
timeval getCurrentTime(){
  timeval t;
  getCurrentTime(t.sec, t.usec);
  return t;
}

void getCurrentTime(timeval& t){
  getCurrentTime(t.sec, t.usec);
}

// Pretty print helper for timestamps
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

// Pretty print helper for timestamps
void printTime(timeval t){
  printTime(t.sec, t.usec);
}
