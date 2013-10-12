#coding:utf-8
import socket

def echo_server(port):
    if port <= 0 or port > 65535:
        print "port illegal"
        return -1
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(('0.0.0.0', port))
    sock.listen(5)
    while True:
        clisock, cliaddr = sock.accept()
        print "connection from", cliaddr
        data = clisock.recv(1024)
        clisock.send(data)
        clisock.close()
        

if __name__ == "__main__":
    try:
        echo_server(8888)
    except KeyboardInterrupt, e:
        print "\nserver exit"
