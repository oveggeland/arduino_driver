#pragma once

#include "network.h"
#include "sync.h"

#define MAC_ADDRESS \
  { 0xA8, 0x61, 0x0A, 0xAF, 0x01, 0xC0 }
#define DEFAULT_IP IPAddress(192, 168, 1, 198)

#define PTP_IP IPAddress(224, 0, 1, 129)
#define PTP_GENERAL_PORT 320
#define PTP_EVENT_PORT 319
#define PTP_SYNC_INTERVAL 10000

void ptpSetup();
void ptpUpdate();


/* Using reversed endian, copy 'size' bytes from 'src' to 'dest'*/
void memcpy_reverse_endian(uint8_t* dest, uint8_t* src, size_t size);

/*Sets the memory pointed at by 'dest', to the value of 'value', which is of a given size 'size'. Reverse the default endian of the computer.*/
void memset_reverse_endian(uint8_t* dest, uint64_t value, size_t size);



#pragma pack(1)
struct ptp_header
{
  uint8_t message_type;
  uint8_t version_ptp;
  uint8_t msg_length[2];
  uint8_t domain_number; 
  uint8_t reserved1;
  uint8_t flags[2];
  uint8_t correction_field[8];
  uint8_t reserved2[4];
  uint8_t clock_id[8];
  uint8_t source_port_id[2];
  uint8_t seq_id[2];
  uint8_t control_field;
  uint8_t log_message_interval;
};

struct ptp_sync
{
    struct ptp_header header;
    uint8_t timestamp[10];
};

struct ptp_followup
{
    struct ptp_header header;
    uint8_t timestamp[10];
};

struct ptp_delayreq
{
    struct ptp_header header;
    uint8_t timestamp[10];
};

struct ptp_delayresp
{
    struct ptp_header header;
    uint8_t timestamp[10];
    uint8_t req_port_id[10];
};

struct ptp_announce
{
    struct ptp_header header;
    uint8_t timestamp[10];
    uint8_t utc_offset[2];
    uint8_t reserved;
    uint8_t gmPriority1;
    uint8_t gmClockClass;
    uint8_t gmClockAcc;
    uint8_t gmClockVar[2];
    uint8_t gmPriority2;
    uint8_t gmId[8];
    uint8_t stepsRemoved[2];
    uint8_t timeSource;
};
