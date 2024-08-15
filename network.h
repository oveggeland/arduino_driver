#pragma once 

#include <Ethernet.h>
#include <EthernetUdp.h>

#include <stdint.h>
#include <Arduino.h>

#define OUTPUT_BUFFER_SIZE 1024
#define SD_CARD_PIN 4

/**
* The three standard functionalities every module should have. 
* 1. Setup on power-up
* 2. Reset for error-handling
* 3. Update is called in the main loop to perform relevant tasks
*/
void networkSetup();
void networkReset();
void networkUpdate();

// Used to push data to the output buffer (E.g. gnss or imu using this to send data to host computer)
bool networkPushData(uint8_t* src_buffer, uint16_t size);