#include "imu.h"

// Status/Diagnostics variables
uint32_t imu_t_diag = 0;

bool imu_active_ = true;
bool imu_error_flag_ = false;

uint16_t readReg(uint16_t addr){
  SPI.beginTransaction(IMU_SPI_SETTINGS);  
  digitalWrite(IMU_CS_PIN, LOW); // Chip select
  
  // SPI COMMANDS
  SPI.transfer16(0x8000 | (addr >> 8)); // Change to correct page
  SPI.transfer16(addr << 8); // Read request from regAddr
  uint16_t output = SPI.transfer16(SPI_NOP); // Send dummy bytes to read the output

  digitalWrite(IMU_CS_PIN, HIGH); // Chip select
  SPI.endTransaction();

  return output;
}

/*
Input uint16_t addr contains page number (first byte), and register addr (second byte)
Input uint16_t value contains the value to write to selected register
*/
void writeReg(uint16_t addr, uint16_t data){
  SPI.beginTransaction(IMU_SPI_SETTINGS);  
  digitalWrite(IMU_CS_PIN, LOW); // Chip select
  
  // SPI COMMANDS
  SPI.transfer16(0x8000 | (addr >> 8)); // Change to correct page
  SPI.transfer16(0x8000 | (addr << 8) | (data & 0x00FF)); // Write to lower byte first
  SPI.transfer16(0x8000 | (addr+1 << 8) | (data >> 8)); // Write to upper byte

  digitalWrite(IMU_CS_PIN, HIGH); // Chip select
  SPI.endTransaction();
}

/*
Read all accleration and gyroscope measurements
*/
void burstRead(uint16_t* rate, uint16_t* acc){
  SPI.beginTransaction(IMU_SPI_SETTINGS); 
  digitalWrite(IMU_CS_PIN, LOW);

  SPI.transfer16(0x8000); // Assert page 0

  // Send read command, followed by dummy bytes (SPI_NOP) to retrieve data from IMU data registers
  SPI.transfer16(X_GYRO_OUT << 8);
  rate[0] = SPI.transfer16(SPI_NOP);

  SPI.transfer16(Y_GYRO_OUT << 8);
  rate[1] = SPI.transfer16(SPI_NOP);

  SPI.transfer16(Z_GYRO_OUT << 8);
  rate[2] = SPI.transfer16(SPI_NOP);

  SPI.transfer16(X_ACCL_OUT << 8);
  acc[0] = SPI.transfer16(SPI_NOP);

  SPI.transfer16(Y_ACCL_OUT << 8);
  acc[1] = SPI.transfer16(SPI_NOP);

  SPI.transfer16(Z_ACCL_OUT << 8);
  acc[2] = SPI.transfer16(SPI_NOP);

  // End transaction
  digitalWrite(IMU_CS_PIN, HIGH); // Chip select
  SPI.endTransaction();
}

void imuSetSampleRate(uint8_t sr){
  writeReg(DEC_RATE, round(2460 / sr) - 1);
}

uint8_t imuGetSampleRate(){
  return 2460.0 / (readReg(DEC_RATE) + 1);
}

void drdyISR(void) {
  imuPackage pkg;
  getCurrentTime(pkg.t_sec, pkg.t_usec);
  burstRead((uint16_t*) pkg.rate, (uint16_t*) pkg.acc);
  networkPushData((uint8_t*) &pkg, sizeof(pkg));
}

void imuSetup(){
  Serial.println("IMU setup");

  SPI.begin(); // This is also called by Ethernet.h, but no worries about that. Safer to do both

  // Setup pins
  pinMode(IMU_CS_PIN, OUTPUT); // Set CS pin to be an output
  digitalWrite(IMU_CS_PIN, HIGH); // Initialize CS pin to be high

  imuReset();

  // DRDY Interrupt setup
  pinMode(IMU_DR_PIN, INPUT); // Set RST pin to be an output
  pinMode(IMU_DR_PIN, INPUT_PULLDOWN);                                    // Using a pullup b/c ICM-20948 Breakout board has an onboard pullup as well and we don't want them to compete
  
  attachInterrupt(digitalPinToInterrupt(IMU_DR_PIN), drdyISR, RISING);
}

void imuReset(){
  // Software resets
  writeReg(GLOB_CMD, 1 << 7); // This is the equivalent of triggering the RST pin (takes 1.8 seconds according to datasheet)
  delay(1800);

  // Self test
  writeReg(GLOB_CMD, 1 << 1); // Self test (Takes 12ms according to datasheet)
  delay(12);

  // Flash test
  writeReg(GLOB_CMD, 1 << 2); // Flash memory test (Takes 53ms according to datasheet)
  delay(53);

  // Set sample rate
  imuSetSampleRate(IMU_DEFAULT_SAMPLE_RATE); 

  // Measurements should be in body frame
  uint16_t reg = readReg(EKF_CNFG);
  writeReg(EKF_CNFG, reg | (1 << 3));

  imuDiag();
}

void imuDiag(){
  if (readReg(DIAG_STS)){
    Serial.print("IMU: Diag error");
    imu_error_flag_ = true;
  }
  else if (readReg(SYS_E_FLAG) & ~(0b11 << 8)){
    Serial.println("IMU: SYS ERROR");
    imu_error_flag_ = true;
  }
  else{
    imu_error_flag_ = false;
  }

  imu_t_diag = millis();
}

// Called frequently from the main program
void imuUpdate(){
  if (!imu_active_)
    return; 
  // Run occasional diagnostics
  if (millis() - imu_t_diag > IMU_DIAG_INTERVAL)
    imuDiag();
}


bool imuIsActive(){
  return imu_active_;
}

void imuActive(bool set){
  imu_active_ = set;
}