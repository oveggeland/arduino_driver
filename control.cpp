#include "control.h"

uint32_t ts_last_update_ = 0;

uint8_t recv_buffer[RECV_BUFFER_SIZE];

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

  st.imu_active = imuIsActive();
  st.imu_sr = imuGetSampleRate();

  st.gnss_active = gnssIsActive();
  st.gnss_sr = gnssGetSampleRate();

  networkPushData((uint8_t*) &st, sizeof(st));
  networkSendData();
}

void parseCommands(){
  // Check recv buffer for commands
  uint16_t size = networkReadData(recv_buffer, RECV_BUFFER_SIZE);

  if (size > 0){
    Serial.print("Data received of size: ");
    Serial.println(size);
  }
}

void controlUpdate(){
  if (millis() - ts_last_update_ > STATUS_INTERVAL){
    statusUpdate();
    ts_last_update_ = millis();
  }
  parseCommands();
}