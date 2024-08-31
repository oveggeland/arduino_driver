#include "control.h"

uint32_t ts_last_update_ = 0;
char cmd_header[] = CMD_HEADER; // Used to validate incoming commands

void statusUpdate(){
  arduinoStatus st;

  // First update all status struct members
  timeval ts = getCurrentTime();
  st.t_sec = ts.sec;
  st.t_usec = ts.usec;
  st.age = millis();

  st.sync_offset = getSyncOffset();

  st.ptp_active = ptpIsActive();
  st.ptp_interval = ptpGetInterval();

  st.imu_active = imuIsActive();
  st.imu_sr = imuGetSampleRate();

  st.gnss_active = gnssIsActive();
  st.gnss_sr = GNSS_DEFAULT_SAMPLE_RATE; //gnssGetSampleRate(); // THIS SHOULD NOT BE USED (MESSES UP I2C bus somehow??)

  networkPushData((uint8_t*) &st, sizeof(st));
  networkSendData(true);
}

void resetArduino(){
  Serial.println("Arduino reset not implemented");
}

void executeCommand(arduinoCommand cmd){
  if (cmd.reset){
    resetArduino();
  }
  
  ptpActive(cmd.ptp_active);
  ptpSetInterval(cmd.ptp_interval);

  imuActive(cmd.imu_active);
  imuSetSampleRate(cmd.imu_sr);

  gnssActive(cmd.gnss_active);
  // gnssSetSampleRate(cmd.gnss_sr);
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
      Serial.println("Received a command");
      
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