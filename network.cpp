#include "network.h"


EthernetUDP Udp;

uint8_t output_buffer[OUTPUT_BUFFER_SIZE];
volatile uint16_t output_buffer_cnt;

void maintain(){
  switch (Ethernet.maintain()) {
    case 1:
      Serial.println("Error: renewed fail");
      break;
    case 2:
      break;
    case 3:
      Serial.println("Error: rebind fail");
      break;
    case 4:
      break;
    default:
      break;
  }
}

void networkSetup(){
  Serial.println("Network setup");

  // Start ethernet and UDP
  byte mac[] = MAC_ADDRESS;
  if (Ethernet.begin(mac, 1000, 200) == 0){
    Serial.println("DHCP connection failed, initializing with static IP");
    Ethernet.begin(mac, DEFAULT_IP);
  }
  
  if (!Udp.begin(LOCAL_PORT)){
    Serial.print("Failed to start UDP socket on port: ");
    Serial.println(LOCAL_PORT);
  };
    
  // Disable SD card (to be safe)
  pinMode(SD_CARD_PIN, OUTPUT); 
  digitalWrite(SD_CARD_PIN, HIGH);
}

bool sendData(){
  uint32_t bytesToWrite = output_buffer_cnt;
  if (bytesToWrite > UDP_MIN_PAYLOAD_SIZE){
    if (!Udp.beginPacket(REMOTE_IP, REMOTE_PORT)){
      Serial.println("UDP begin packet failed");
      return false;
    };
    
    uint32_t bytesWritten = Udp.write(output_buffer, bytesToWrite);
    if (bytesWritten != bytesToWrite){
      Serial.println("UDP write failed");
      Serial.print("Bytes written is: ");
      Serial.println(bytesWritten);
      Serial.print("Bytes to write was: ");
      Serial.println(bytesToWrite);
      return false;
    };
    if (!Udp.endPacket()){
      Serial.println("UDP endpacket failed");
      return false;
    }

    output_buffer_cnt -= bytesWritten; // Buffer reset
  }
  return true;
}

void networkUpdate(){
  maintain();

  uint8_t tries = 0;
  while (tries++ < 5 && !sendData()){
    Serial.println("Trying again");
  }
}

bool networkPushData(uint8_t* src_buffer, uint16_t size){
  bool retval = false;
  
  noInterrupts();
  if (size <= OUTPUT_BUFFER_SIZE - output_buffer_cnt){
    // Copy data to output buffer, add to the buffer count
    memcpy(output_buffer + output_buffer_cnt, src_buffer, size);
    output_buffer_cnt += size;
    retval = true;
  }
  interrupts();

  return retval;
};