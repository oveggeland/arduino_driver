#include "network.h"

// Ip configurations
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 177);

uint32_t localPort = 8888;      // local port to listen on

IPAddress remoteIP(192, 168, 1, 56);
uint32_t remotePort = 5005;      // local port to listen on

EthernetUDP Udp;

uint8_t output_buffer[OUTPUT_BUFFER_SIZE];
uint16_t output_buffer_cnt;

void networkSetup(){
  Serial.println("Network setup");

  // Start ethernet and UDP
  Ethernet.begin(mac, ip);
  Udp.begin(localPort);
    
  // Disable SD card (to be safe)
  pinMode(SD_CARD_PIN, OUTPUT); 
  digitalWrite(SD_CARD_PIN, HIGH);
}

void networkUpdate(){
  if (output_buffer_cnt != 0){
    

    /*
    NB!!! The current SPI library for R4 wifi does not support usingInterrupt() for some reason? 
    Dirty solution is to disable the interrupts manually whenever SPI will be used... 
    Note that a lot of functionality is not supported while interrupts are disabled, e.g. Serial.print()
    */
    noInterrupts();

    Udp.beginPacket(remoteIP, remotePort);
    Udp.write(output_buffer, output_buffer_cnt);
    Udp.endPacket();

    interrupts();
    
    output_buffer_cnt = 0; // Buffer reset
  }
}


bool networkPushData(uint8_t* src_buffer, uint16_t size){
  if (size > OUTPUT_BUFFER_SIZE - output_buffer_cnt){
    Serial.println("Not enough space on output buffer");
    return false;
  }

  // Copy data to output buffer, add to the buffer count
  memcpy(output_buffer + output_buffer_cnt, src_buffer, size);
  output_buffer_cnt += size;

  return true;
};