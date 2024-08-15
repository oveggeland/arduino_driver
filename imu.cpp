#include "imu.h"

imuData isr_data_obj;
RingBuf<imuData, 32> myDataBuffer; // Ringbuffer (FIFO) to store IMU measurements from ISR

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
SPI is not efficient with single read calls, this does not use the full duplex possibilities.
This function performs a burst read of all gyro + acc data in a much more efficient manner.
*/
void burstRead(volatile int16_t* rate, volatile int16_t* acc){
  SPI.beginTransaction(IMU_SPI_SETTINGS); 
  digitalWrite(IMU_CS_PIN, LOW);

  SPI.transfer16(0x8000); // Assert page 0

  SPI.transfer16(X_GYRO_OUT << 8); // Read command gyro_x
  rate[0] = SPI.transfer16(Y_GYRO_OUT << 8); // Retrieve gyro_x and read command gyro_y (Samne pattern underneath)
  rate[1] = SPI.transfer16(Z_GYRO_OUT << 8); 
  rate[2] = SPI.transfer16(X_ACCL_OUT << 8);
  acc[0] = SPI.transfer16(Y_ACCL_OUT << 8);
  acc[1] = SPI.transfer16(Z_ACCL_OUT << 8);
  acc[2] = SPI.transfer16(SPI_NOP); // No command, but we need to read out the last acc data

  // End transaction
  digitalWrite(IMU_CS_PIN, HIGH); // Chip select
  SPI.endTransaction();
}

void setSampleRate(float sr){
  uint16_t dec_rate_value = round((2460 / sr) - 1);
  writeReg(DEC_RATE, dec_rate_value);
}

void drdyISR(void) {
  isr_data_obj.ts = micros(); // Set timestamp immediately
  burstRead(isr_data_obj.rate, isr_data_obj.acc); // Instantly read out the data

  myDataBuffer.pushOverwrite((imuData) isr_data_obj);
}

void imuSetup(){
  Serial.println("IMU setup");
  SPI.begin(); // This is also called in Ethernet.h, but no worries about that

  // Setup pins
  pinMode(IMU_CS_PIN, OUTPUT); // Set CS pin to be an output
  digitalWrite(IMU_CS_PIN, HIGH); // Initialize CS pin to be high

  imuReset();

  // DRDY Interrupt setup
  pinMode(IMU_DR_PIN, INPUT); // Set RST pin to be an output
  pinMode(IMU_DR_PIN, INPUT_PULLDOWN);                                    // Using a pullup b/c ICM-20948 Breakout board has an onboard pullup as well and we don't want them to compete
  
  int val = digitalPinToInterrupt(IMU_DR_PIN);
  attachInterrupt(val, drdyISR, RISING);
  SPI.usingInterrupt(val);
}

void imuReset(){
  // Software resets
  writeReg(GLOB_CMD, 1 << 7); // This is the equivalent of triggering the RST pin (takes 1.8 seconds according to datasheet)
  delay(1800);

  // Self test
  writeReg(GLOB_CMD, 1 << 1); // Self test (Takes 12ms according to datasheet)
  delay(12);

  if (readReg(DIAG_STS))
    Serial.print("Self test failed...");
  else
    Serial.println("Self test passed");
  
  // Flash test
  writeReg(GLOB_CMD, 1 << 2); // Flash memory test (Takes 53ms according to datasheet)
  delay(53);
  uint16_t error_flag = readReg(SYS_E_FLAG);
  if (readReg(SYS_E_FLAG) & (1 << 6))
    Serial.println("flash test failed...");
  else
    Serial.println("flash test passed");

  // Set sample rate
  setSampleRate(200); 

  // Measurements should be in body frame
  uint16_t reg = readReg(EKF_CNFG);
  writeReg(EKF_CNFG, reg | (1 << 3));
}

/*
Iterate through ringbuffer of imu data (pushed by ISR). Construct IMU network packages and push to network buffer for later transmition. 
*/
void imuUpdate(){
  imuData data_obj;
  imuPackage data_pkg;
  while (myDataBuffer.lockedPop(data_obj)){
    data_pkg.ts = data_obj.ts;

    data_pkg.rate[0] = 0.02* data_obj.rate[0];
    data_pkg.rate[1] = 0.02* data_obj.rate[1];
    data_pkg.rate[2] = 0.02* data_obj.rate[2];

    data_pkg.acc[0] = 0.8* data_obj.acc[0];
    data_pkg.acc[1] = 0.8* data_obj.acc[1];
    data_pkg.acc[2] = 0.8* data_obj.acc[2];    

    networkPushData((uint8_t*) &data_pkg, sizeof(data_pkg)); // After we push to network buffer, the data is out of our hands. Note that this does not guarantee a delivery to host. 
  }
}