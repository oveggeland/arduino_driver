#include "sync.h" 

ntp_payload ntpReq;
EthernetUDP ntpUDP;

// Local time for send and receive (Stamped by local clock on transmit and receive of NTP messages)
uint32_t t0_local;
uint32_t t3_local;

// NTP time for sending (to be calculated based on NTP response)
uint32_t t0_ntp_sec;
uint32_t t0_ntp_usec;

// Last sync time
uint32_t t_sync = 0;
bool ntp_synced = false;


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
void sendNTPpacket() {
  ntpUDP.beginPacket(NTP_SERVER_IP, NTP_SERVER_PORT); // NTP requests are to port 123
  ntpUDP.write((uint8_t*) &ntpReq, sizeof(ntpReq));

  ntpUDP.endPacket();
  t0_local = micros();
}


bool doSync(){
// Example version
  sendNTPpacket(); // send an NTP packet to a time server

  // Wait for response (timeout at 10 milliseconds)
  uint32_t t0 = millis();
  int bytesAvailable;
  do{
    t3_local = micros();
    bytesAvailable = ntpUDP.parsePacket();
  } while ((bytesAvailable == 0) && (millis() - t0 < 10)); // Keep trying as long as we have not bytes or timeout is not raised

  // Read package
  if (bytesAvailable == sizeof(ntpReq)){
    // We've received a packet, read the data from it
    ntp_payload ntpResp;
    ntpUDP.read((uint8_t*) &ntpResp, sizeof(ntpReq)); // read the packet into the buffer

    // TODO: Add assertions to check NTP package quality (ref_id, and other validity tests)
    // TODO: Make some nice assertions for micros() overflow counting (this happens every 70 minutes)

    // 1. Calculate one-way delay (in microseconds)    
    uint32_t t1_sec = getSec(ntpResp.ts_rec);
    uint32_t t1_usec = getUsec(ntpResp.ts_rec);

    uint32_t t2_sec = getSec(ntpResp.ts_trans);
    uint32_t t2_usec = getUsec(ntpResp.ts_trans);

    uint32_t delay = ((t3_local - t0_local) - getTimeDiff(t1_sec, t1_usec, t2_sec, t2_usec)) / 2; // Microsecond delay

    
    if (delay > t1_usec){
      t0_ntp_sec = t1_sec - 1;
      t0_ntp_usec = t1_usec - delay + 1e6;
    }
    else{
      t0_ntp_sec = t1_sec;
      t0_ntp_usec = t1_usec - delay;
    }

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
  if (doSync())
    ntp_synced = true;
}


void ntpUpdate() {
  if (millis() - t_sync > NTP_INTERVAL || !ntp_synced){
    if(doSync()){
      Serial.println("NTP sync complete");
      t_sync = millis();
      ntp_synced = true;
    }
    else{
      Serial.println("NTP sync failed");
      ntp_synced = false;
    }
  }
}

void getCurrentTime(uint32_t &sec, uint32_t &usec){
  noInterrupts(); // No interrupts should be allowed while setting timestamps
  uint32_t t_local = micros();

  sec = t0_ntp_sec;
  usec = t0_ntp_usec + (t_local - t0_local);

  uint32_t seconds_passed = usec / 1e6;

  usec -= seconds_passed*1e6;
  sec += seconds_passed;
  interrupts();
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
