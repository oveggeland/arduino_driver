import socket
import struct

# IP of ethernet connection and a valid port (must be same as in Network.h on Arduino side)
UDP_IP = "192.168.1.56"
UDP_PORT = 5005

sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
sock.bind((UDP_IP, UDP_PORT))

IMU_HEADER = "$IMU"
GNSS_HEADER = "$GNSS"
HB_HEADER = "$HB"

IMU_STRUCT_SIZE = 32 
GNSS_STRUCT_SIZE = 21


def parse_imu(data):
    ts, acc_x, acc_y, acc_z, rate_x, rate_y, rate_z = struct.unpack("@Iffffff", data[len(IMU_HEADER):])
    print(f"{ts} - Acc: [{acc_x:.2f}, {acc_y:.2f}, {acc_z:.2f}], Rate: [{rate_x:.2f}, {rate_y:.2f}, {rate_z:.2f}]")

def parse_gnss_data(data):
    ts, lat, lng, alt = struct.unpack("@Iiii", data[len(GNSS_HEADER):])
    print(f"{ts} - Lat: {lat}, Lng: {lng}, Alt: {alt}")

while True:
    data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes, should be more than enough

    idx = data.find(b'$') # '$' is the header sign, this is placed at the beginning of each package
    while idx != -1:
        data = data[idx:] # Remove random bytes before header start ('$')
        
        # Check for IMU Header
        if data[:len(IMU_HEADER)] == bytes(IMU_HEADER, 'utf-8'):
            #parse_imu(data[:IMU_STRUCT_SIZE])
            data = data[IMU_STRUCT_SIZE:]

        elif data[:len(GNSS_HEADER)] == bytes(GNSS_HEADER, 'utf-8'):
            parse_gnss_data(data[:GNSS_STRUCT_SIZE])
            data = data[GNSS_STRUCT_SIZE:]

        elif data[:len(HB_HEADER)] == bytes(HB_HEADER, 'utf-8'):
            print("HEARTBEAT!")
            data = data[len(HB_HEADER):]

        else: # Some unknown header, move on
            data = data[1:]

        idx = data.find(b'$')
