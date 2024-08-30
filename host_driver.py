import socket
import struct

import rospy
from sensor_msgs.msg import Imu, NavSatFix

import netifaces as ni
UDP_IP = ni.ifaddresses('enp87s0')[ni.AF_INET][0]['addr']
UDP_PORT = 5005

sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
sock.bind((UDP_IP, UDP_PORT))

IMU_HEADER = "$IMU"
GNSS_HEADER = "$GNSS"
STATUS_HEADER = "$ST"

IMU_STRUCT_SIZE = 24
GNSS_STRUCT_SIZE = 25
STATUS_STRUCT_SIZE = 37


def parse_imu_data(data, verbose=False):
    if len(data) != IMU_STRUCT_SIZE:
        print("IMU struct length error...")
    
    t_sec, t_usec, acc_x, acc_y, acc_z, rate_x, rate_y, rate_z = struct.unpack("@IIhhhhhh", data[len(IMU_HEADER):])

    acc_x *= 0.8
    acc_y *= 0.8
    acc_z *= 0.8

    rate_x *= 0.02
    rate_y *= 0.02
    rate_z *= 0.02

    if verbose:
        print(f"{t_sec}.{t_usec:06d} - Acc: [{acc_x:+09.2f}, {acc_y:+09.2f}, {acc_z:+09.2f}], Rate: [{rate_x:+07.2f}, {rate_y:+07.2f}, {rate_z:+07.2f}]")

    msg = Imu()
    msg.header.stamp = rospy.Time(t_sec, 1000*t_usec)

    msg.linear_acceleration.x = acc_x
    msg.linear_acceleration.y = acc_y
    msg.linear_acceleration.z = acc_z

    msg.angular_velocity.x = rate_x
    msg.angular_velocity.y = rate_y
    msg.angular_velocity.z = rate_z

    return msg


def parse_gnss_data(data, verbose=False):
    if len(data) != GNSS_STRUCT_SIZE:
        print("GNSS struct length error...")

    t_sec, t_usec, lat, lng, alt = struct.unpack("@IIiii", data[len(GNSS_HEADER):])
    if verbose:
        print(f"{t_sec}.{t_usec:06d} - Lat: {lat}, Lng: {lng}, Alt: {alt}")

    msg = NavSatFix()
    msg.header.stamp = rospy.Time(t_sec, 1000*t_usec)

    msg.latitude = lat
    msg.longitude = lng
    msg.altitude = alt

    return msg


def parse_status_data(data, verbose=False):
    if len(data) != STATUS_STRUCT_SIZE:
        print("Status struct length error...")

    t_sec, t_usec, age, ip, dhcp_st, ntp_interval, ntp_offset, ptp_active, ptp_interval, imu_active, imu_sr, gnss_active, gnss_sr = struct.unpack("=IIII?Ii?I?B?B", data[len(STATUS_HEADER):])
    
    ip = struct.unpack("BBBB", struct.pack("I", ip))
    if verbose:
        print(f"{t_sec}.{t_usec:06d} - Age: {age}, IP: {ip}, DHCP_STATUS: {dhcp_st}, T_NTP: {ntp_interval}, NTP_OFFSET: {ntp_offset:+6d}, T_PTP: {ptp_interval}")
        print(f"{t_sec}.{t_usec:06d} - Imu active: {imu_active}, Imu sample rate: {imu_sr}, GNSS active: {gnss_active}, GNSS sample rate: {gnss_sr}")
    
    return True


if __name__ == "__main__":
    rospy.init_node("test_node")
    print("Using ip:", UDP_IP)

    imu_pub = rospy.Publisher('imu', Imu, queue_size=1000)
    gnss_pub = rospy.Publisher('gnss', NavSatFix, queue_size=10)

    while not rospy.is_shutdown():
        data, addr = sock.recvfrom(2048) 

        idx = data.find(b'$') # '$' is the header sign, this is placed at the beginning of each package
        while idx != -1:
            data = data[idx:] # Remove random bytes before header start ('$')
            
            # Check for IMU Header
            if data[:len(IMU_HEADER)] == bytes(IMU_HEADER, 'utf-8'):
                imu_msg = parse_imu_data(data[:IMU_STRUCT_SIZE], verbose=False)
                imu_pub.publish(imu_msg)

                data = data[IMU_STRUCT_SIZE:]

            elif data[:len(GNSS_HEADER)] == bytes(GNSS_HEADER, 'utf-8'):
                gnss_msg = parse_gnss_data(data[:GNSS_STRUCT_SIZE], verbose=False)
                gnss_pub.publish(gnss_msg)

                data = data[GNSS_STRUCT_SIZE:]


            elif data[:len(STATUS_HEADER)] == bytes(STATUS_HEADER, 'utf-8'):
                status_msg = parse_status_data(data[:STATUS_STRUCT_SIZE], verbose=True)
                #status_pub.publish(status_msg)

                data = data[STATUS_STRUCT_SIZE:]

            else: # Some unknown header, move on
                data = data[1:]

            idx = data.find(b'$')