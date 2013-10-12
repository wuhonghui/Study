#coding:utf-8
import socket

def udp_server(addr, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((addr, port))
    while True:
        data, addr = sock.recvfrom(1024)
        if not data:
            return
        print "receive from", addr
        sock.sendto(data, addr)

if __name__ == "__main__":
    try:
        udp_server("127.0.0.1", 8888)
    except KeyboardInterrupt, e:
        print "\nserver exit"
