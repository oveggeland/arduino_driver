#pragma once 

#include <Ethernet.h>
#include <EthernetUdp.h>

#include <stdint.h>
#include <Arduino.h>

#define OUTPUT_BUFFER_SIZE 1472 // MTU for Ethernet shield is 1500, subtract (ipv4 and udp headers to get 1472 as maximum buffer size)
#define SD_CARD_PIN 4

#define UDP_MIN_PAYLOAD_SIZE 256

#define SYS1 // change this to use second system config
#ifdef SYS1
  #define MAC_ADDRESS {0xA8, 0x61, 0x0A, 0xAF, 0x01, 0xC0}
  #define LOCAL_IP IPAddress(192, 168, 1, 81)
  #define HOST_IP IPAddress(192, 168, 1, 51)
#else
  #define MAC_ADDRESS {0xA8, 0x61, 0x0A, 0xAF, 0x15, 0x59}
  #define LOCAL_IP IPAddress(192, 168, 1, 82)
  #define HOST_IP IPAddress(192, 168, 1, 52)
#endif

#define LOCAL_PORT 8888
#define HOST_PORT 5005

void networkSetup();
void networkUpdate();

bool sendUdpMsg(EthernetUDP *pcb, IPAddress dst_ip, int dst_port, uint8_t *buffer, uint16_t size); 

// Used to push data to the output buffer (E.g. gnss or imu using this to send data to host computer)
void networkPushData(uint8_t* src_buffer, uint16_t size);

// Allows other modules to force network buffer to be sent (If not, this also happens regularly in networkUpdate())
bool networkSendData(bool force_send=false);

// Read data from network socket onto buffer, returns number of bytes read
uint16_t networkReadData(uint8_t* buffer, uint16_t buffer_size);