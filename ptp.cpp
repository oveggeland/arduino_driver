#include "ptp.h"

EthernetUDP ptp_pcb_general;
EthernetUDP ptp_pcb_event;

struct ptp_announce announce_msg = { 0 };
struct ptp_sync sync_msg = { 0 };
struct ptp_followup followup_msg = { 0 };
struct ptp_delayresp delayresp_msg = { 0 };

bool ptp_active_ = true;
uint32_t ptp_interval_ = PTP_DEFAULT_INTERVAL; 

void initHeaders() {
  // Common header
  struct ptp_header common_header = { 0 };
  common_header.version_ptp = 0x02;

  memset_reverse_endian((uint8_t *)&common_header.clock_id, 0xA8610AEEEFAF01C0, 8);
  memset_reverse_endian((uint8_t *)&common_header.source_port_id, 1, 2);
  memset_reverse_endian((uint8_t *)&common_header.msg_length, 44, 2);  // For sync and followup (change for delayresp and announce)

  // Announce specific adjustments
  announce_msg.header = common_header;
  announce_msg.header.message_type = 0x0b;
  memset_reverse_endian((uint8_t *)&announce_msg.header.msg_length, 64, 2);
  announce_msg.header.control_field = 0x05;
  announce_msg.header.log_message_interval = 4;


  // Sync specific adjustments
  sync_msg.header = common_header;
  memset_reverse_endian((uint8_t *)&sync_msg.header.msg_length, 44, 2);
  memset_reverse_endian((uint8_t *)&sync_msg.header.flags, 0x0200, 2);

  // Followup specific adjustments
  followup_msg.header = common_header;
  followup_msg.header.message_type = 0x08;
  followup_msg.header.control_field = 0x02;

  // Delayresponse specific adjustments
  delayresp_msg.header = common_header;
  delayresp_msg.header.message_type = 0x09;
  delayresp_msg.header.control_field = 0x03;
  memset_reverse_endian((uint8_t *)&delayresp_msg.header.msg_length, 54, 2);
}


/* Using reversed endian, copy 'size' bytes from 'src' to 'dest'*/
void memcpy_reverse_endian(uint8_t* dest, uint8_t* src, size_t size){
    for (int i = 0; i < size; i++){
        dest[i] = src[size - 1 - i];
    }
}

/*Sets the memory pointed at by 'dest', to the value of 'value', which is of a given size 'size'. Reverse the default endian of the computer.*/
void memset_reverse_endian(uint8_t* dest, uint64_t value, size_t size){
    uint8_t* value_pointer = (uint8_t*) &value;
    for (int i = 0; i < size; i++){
        dest[i] = value_pointer[size - 1 - i];
    }
}

void ptpSetup() {
  // put your setup code here, to run once:
  ptp_pcb_general.beginMulticast(PTP_IP, PTP_GENERAL_PORT);
  ptp_pcb_event.beginMulticast(PTP_IP, PTP_EVENT_PORT);

  initHeaders();
}

void setTimestampBytes(uint8_t* p_ts_dst, timeval ts){
  uint32_t nsec = 1e3*ts.usec;
  memcpy_reverse_endian(&p_ts_dst[2], (uint8_t *)&ts.sec, 4);
  memcpy_reverse_endian(&p_ts_dst[6], (uint8_t *)&nsec, 4);
}

bool announce(uint16_t seq) {
  // Set seq
  memset_reverse_endian(announce_msg.header.seq_id, seq, 2);

  // Set time stamp
  setTimestampBytes(announce_msg.timestamp, getCurrentTime());

  // Set random stuff based on what I see on ptpd library packages
  announce_msg.gmPriority1 = 128;
  announce_msg.gmPriority2 = 128;
  announce_msg.gmClockClass = 13;
  announce_msg.gmClockAcc = 0xfe;
  memset_reverse_endian(announce_msg.gmClockVar, 0xffff, 2);
  announce_msg.gmPriority2 = 128;
  announce_msg.timeSource = 0xa0;

  memset_reverse_endian(announce_msg.gmId, 0xA8610AEEEFAF01C0, 8);

  return sendUdpMsg(&ptp_pcb_general, PTP_IP, PTP_GENERAL_PORT, (uint8_t *)&announce_msg, sizeof(announce_msg));
}

bool sendSyncMsg(uint16_t seq, timeval &t1) {
  memset_reverse_endian((uint8_t *)&sync_msg.header.seq_id, seq, 2);

  setTimestampBytes(sync_msg.timestamp, getCurrentTime());

  if (sendUdpMsg(&ptp_pcb_event, PTP_IP, PTP_EVENT_PORT, (uint8_t *)&sync_msg, sizeof(sync_msg))){
    getCurrentTime(t1);
    return true;
  }
  return false;
}

bool sendFollowupMsg(uint16_t seq, timeval t1) {
  memset_reverse_endian((uint8_t *)&followup_msg.header.seq_id, seq, 2);

  setTimestampBytes(followup_msg.timestamp, t1);

  return sendUdpMsg(&ptp_pcb_general, PTP_IP, PTP_GENERAL_PORT, (uint8_t *)&followup_msg, sizeof(followup_msg));
}

bool sendDelayRespMsg(timeval t3_ptp, struct ptp_delayreq ptpReq) {
  memcpy(delayresp_msg.header.seq_id, ptpReq.header.seq_id, 2);
  memcpy(delayresp_msg.req_port_id, ptpReq.header.clock_id, 10);

  setTimestampBytes(delayresp_msg.timestamp, t3_ptp);

  return sendUdpMsg(&ptp_pcb_general, PTP_IP, PTP_GENERAL_PORT, (uint8_t *)&delayresp_msg, sizeof(delayresp_msg));
}

bool delayRespond() {
  int bytesAvailable = ptp_pcb_event.parsePacket();
  timeval t3_ptp = getCurrentTime();

  if (bytesAvailable == sizeof(struct ptp_delayreq)) {
    struct ptp_delayreq ptpReq;
    ptp_pcb_event.read((uint8_t *)&ptpReq, sizeof(ptpReq));  // read the packet into the buffer

    sendDelayRespMsg(t3_ptp, ptpReq);
  }

  return true;
}

uint16_t seq = 0;
uint32_t t_sync = 0;
void ptpUpdate() {
  if (!ptp_active_)
    return;

  delayRespond();

  if (millis() - t_sync > ptp_interval_){
    announce(seq);

    timeval t1_ptp;
    sendSyncMsg(seq, t1_ptp);
    sendFollowupMsg(seq, t1_ptp);

    seq++;
    
    t_sync = millis();
  }
}

void ptpSetInterval(uint32_t interval_ms){
  ptp_interval_ = interval_ms;
}

uint32_t ptpGetInterval(){
  return ptp_interval_;
}

void ptpActive(bool set){
  ptp_active_ = set;
}

bool ptpIsActive(){
  return ptp_active_;
};
