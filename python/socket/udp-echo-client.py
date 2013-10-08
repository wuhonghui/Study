#coding:utf-8
import socket

def udp_echo_client(addr, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    data = raw_input('> ')
    sock.sendto(data, (addr, port))
    data, addr = sock.recvfrom(1024)
    if not data:
        return 
    print data
    sock.close()

if __name__ == "__main__":
    udp_echo_client("127.0.0.1", 8888)
