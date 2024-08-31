#include "network.h"


EthernetUDP network_pcb;

uint8_t output_buffer[OUTPUT_BUFFER_SIZE];
volatile uint16_t output_buffer_cnt;

void networkSetup(){
  Serial.println("Network setup");

  // Disable SD card (to be safe)
  pinMode(SD_CARD_PIN, OUTPUT); 
  digitalWrite(SD_CARD_PIN, HIGH);

  // Start ethernet and UDP
  byte mac[] = MAC_ADDRESS;
  Ethernet.begin(mac, LOCAL_IP);
  
  if (!network_pcb.begin(LOCAL_PORT)){
    Serial.print("Failed to start UDP socket on port: ");
    Serial.println(LOCAL_PORT);
  };
}

bool networkSendData(bool force_send){
  uint32_t bytesToWrite = output_buffer_cnt;
  if (force_send || bytesToWrite > UDP_MIN_PAYLOAD_SIZE){
    if (!sendUdpMsg(&network_pcb, HOST_IP, HOST_PORT, output_buffer, output_buffer_cnt))
      return false;
    
    output_buffer_cnt -= bytesToWrite; // Buffer reset
  }
  return true;
}

void networkUpdate(){
  if (!networkSendData()){
    Serial.println("Network update: Failed to send data...");
  }
}

void networkPushData(uint8_t* src_buffer, uint16_t size){
  noInterrupts();
  if (size <= OUTPUT_BUFFER_SIZE - output_buffer_cnt){
    memcpy(output_buffer + output_buffer_cnt, src_buffer, size);
    output_buffer_cnt += size;
  }
  interrupts();
};

uint16_t networkReadData(uint8_t* buffer, uint16_t buffer_size){
  int bytesAvailable = network_pcb.parsePacket();
  if (bytesAvailable <= 0){
    return 0;
  }
  else if (bytesAvailable > buffer_size){
    Serial.println("Can not read data from UDP, buffer size is too small");
    return 0;
  }

  // Suitable buffer size and bytes available
  return network_pcb.read(buffer, bytesAvailable);
}


bool sendUdpMsg(EthernetUDP *pcb, IPAddress dst_ip, int dst_port, uint8_t *buffer, uint16_t size) {
  // Begin packet
  if (!pcb->beginPacket(dst_ip, dst_port)) {
    return false;
  };

  // Write bytes
  uint32_t bytesWritten = pcb->write(buffer, size);
  if (bytesWritten != size) {
    return false;
  };

  // Send packet
  if (!pcb->endPacket()) {
    return false;
  }

  return true;
}