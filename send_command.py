import socket
import struct
import time

#UDP_IP = "192.168.1.255"
UDP_IP = "192.168.1.255"
UDP_PORT = 8888
MESSAGE = b"Hello, World!"

# Define command
HEADER = "$CMD"
RESET = True
NTP_INTERVAL = int(10*1e6) # Microseconds
PTP_ACTIVE = True
PTP_INTERVAL = int(12*1e3) # Milliseconds
IMU_ACTIVE = True
IMU_SR = 170
GNSS_ACTIVE = True
GNSS_SR = 1

print("UDP target IP: %s" % UDP_IP)
print("UDP target port: %s" % UDP_PORT)
print("message: %s" % MESSAGE)

if __name__ == "__main__":

    sock = socket.socket(socket.AF_INET, # Internet
                        socket.SOCK_DGRAM) # UDP
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    bytes = struct.pack("=4s?I?I?B?B",
        bytes(HEADER, 'utf-8'),
        RESET,
        NTP_INTERVAL,
        PTP_ACTIVE,
        PTP_INTERVAL,
        IMU_ACTIVE,
        IMU_SR,
        GNSS_ACTIVE,
        GNSS_SR
    )

    sock.sendto(bytes, (UDP_IP, UDP_PORT))