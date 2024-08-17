import socket
import struct

import rospy
from sensor_msgs.msg import Imu, NavSatFix

# IP of ethernet connection and a valid port (must be same as in Network.h on Arduino side)
UDP_IP = "192.168.1.56"
UDP_PORT = 5005

sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
sock.bind((UDP_IP, UDP_PORT))

IMU_HEADER = "$IMU"
GNSS_HEADER = "$GNSS"
HB_HEADER = "$HB"

IMU_STRUCT_SIZE = 24
GNSS_STRUCT_SIZE = 25



def parse_imu_data(data, verbose=False):
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
    t_sec, t_usec, lat, lng, alt = struct.unpack("@IIiii", data[len(GNSS_HEADER):])
    if verbose:
        print(f"{t_sec}.{t_usec:06d} - Lat: {lat}, Lng: {lng}, Alt: {alt}")

    msg = NavSatFix()
    msg.header.stamp = rospy.Time(t_sec, 1000*t_usec)

    msg.latitude = lat
    msg.longitude = lng
    msg.altitude = alt

    return msg


if __name__ == "__main__":
    rospy.init_node("test_node")

    imu_pub = rospy.Publisher('imu', Imu, queue_size=100)
    gnss_pub = rospy.Publisher('gnss', NavSatFix, queue_size=10)

    while not rospy.is_shutdown():
        data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes, should be more than enough

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

            elif data[:len(HB_HEADER)] == bytes(HB_HEADER, 'utf-8'):
                #print("HEARTBEAT!")
                data = data[len(HB_HEADER):]

            else: # Some unknown header, move on
                data = data[1:]

            idx = data.find(b'$')