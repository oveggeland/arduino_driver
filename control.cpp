#include "control.h"

uint32_t ts_last_update_ = 0;
char cmd_header[4] = CMD_HEADER; // Used to validate incoming commands

void statusUpdate(){
  arduinoStatus st;

  st.ip = (uint32_t) Ethernet.localIP();

  // First update all status struct members
  timeval t = getCurrentTime();
  st.t_sec = t.sec;
  st.t_usec = t.usec;
  st.age = millis();
  st.sync_offset = getSyncOffset();

  st.imu_id = imuGetId();
  st.imu_rate = imuGetSampleRate();
  st.imu_temp = imuGetTemperature();

  st.ptp_interval = ptpGetInterval();

  networkPushData((uint8_t*) &st, sizeof(st));
  networkSendData(true);
}

void executeCommand(arduinoCommand cmd){
  if (cmd.reset){
    // NVIC_SystemReset(); // This is not working..
  }
}

void parseCommands(){
  // Check recv buffer for commands
  arduinoCommand cmd;
  uint16_t bytesRead = networkReadData((uint8_t*) &cmd, sizeof(cmd));
  if (bytesRead == 0)
    return;
  
  if (bytesRead == sizeof(cmd)){
    // Correct size, let's check header
    if (memcmp(cmd_header, cmd.header, sizeof(cmd.header)) == 0){
      // The command was correct
      executeCommand(cmd);
    }
  }
}

void controlUpdate(){
  if (millis() - ts_last_update_ > STATUS_INTERVAL){
    statusUpdate();
    ts_last_update_ = millis();
  }
  parseCommands();
}