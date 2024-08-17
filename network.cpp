#include "network.h"

uint32_t ts_maintain = 0;

byte mac[] = {
  0xA8, 0x61, 0x0A, 0xAF, 0x01, 0xC0
};// A8610AAF01C0 (from board)

IPAddress staticIp(192, 168, 1, 177); // Default if DHCP not available
uint32_t localPort = 8888;      // local port to listen on

IPAddress remoteIP(192, 168, 1, 56);
uint32_t remotePort = 5005;      // local port to listen on

EthernetUDP Udp;

uint8_t output_buffer[OUTPUT_BUFFER_SIZE];
uint16_t output_buffer_cnt;

int maintain(){
  int ret_val = Ethernet.maintain(); // This takes about 15ms! 

  switch (ret_val) {
    case 1:
      //renewed fail
      Serial.println("Error: renewed fail");
      break;

    case 2:
      //renewed success
      Serial.println("Renewed success");
      //print your local IP address:
      Serial.print("My IP address: ");
      Serial.println(Ethernet.localIP());
      break;

    case 3:
      //rebind fail
      Serial.println("Error: rebind fail");
      break;

    case 4:
      //rebind success
      Serial.println("Rebind success");
      //print your local IP address:
      Serial.print("My IP address: ");
      Serial.println(Ethernet.localIP());
      break;

    default:
      break;
  }

  return ret_val;
}

void networkSetup(){
  Serial.println("Network setup");

  // Start ethernet and UDP
  if (Ethernet.begin(mac, 2000) == 0){
    if (maintain() & 0b1){
      Serial.println("DHCP connection failed, using static IP");
      Ethernet.begin(mac, staticIp);
    };
  };

  Udp.begin(localPort);
    
  // Disable SD card (to be safe)
  pinMode(SD_CARD_PIN, OUTPUT); 
  digitalWrite(SD_CARD_PIN, HIGH);
}


bool sendData(){
  uint32_t bytesToWrite = output_buffer_cnt;
  if (bytesToWrite > UDP_MIN_PAYLOAD_SIZE){
    if (!Udp.beginPacket(remoteIP, remotePort)){
      Serial.println("UDP begin packet failed");
      return false;
    };
    
    size_t bytesWritten = Udp.write(output_buffer, bytesToWrite);
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
}

void networkUpdate(){
  /*
  1. Maintain connections (DHCP, get destination IP, etc.)
  2. Send data on output buffer
  3. Parse incoming messages
  */ 
  if (millis() - ts_maintain > 1000){
    maintain();
    ts_maintain = millis();
  }

  sendData();
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