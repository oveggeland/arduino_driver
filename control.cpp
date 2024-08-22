#include "control.h"

uint32_t ts_last_update_ = 0;

void statusUpdate(){
  arduinoStatus st;

  // First update all status struct members
  timeval ts = getCurrentTime();
  st.t_sec = ts.sec;
  st.t_usec = ts.usec;
  st.age = millis();

  st.ntp_interval = ntpGetInterval();
  st.ntp_offset = ntpGetOffset();

  st.ptp_active = ptpIsActive();
  st.ptp_interval = ptpGetInterval();

  networkPushData((uint8_t*) &st, sizeof(st));
  Serial.print("Sending status struct of size: ");
  Serial.println(sizeof(st));
  networkSendData();
}

void parseCommands(){
  // Check recv buffer for commands
}

void controlUpdate(){
  if (millis() - ts_last_update_ > STATUS_INTERVAL){
    statusUpdate();
    ts_last_update_ = millis();
  }
  parseCommands();
}