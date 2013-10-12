#coding:utf-8
import socket
def echo_client(addr, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((addr, port))
    data = raw_input('> ')
    if not data:
        return
    sock.send(data)
    newdata = sock.recv(1024)
    print newdata
    sock.close()

if __name__ == "__main__":
    echo_client('127.0.0.1', 8888)
