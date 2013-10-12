#coding:utf-8
import os
import socket
import syslog

def handle(clisock):
    data = clisock.recv(1024)
    clisock.send(data)
    clisock.close()

def echo_process_server(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(('127.0.0.1', port))
    sock.listen(5)
    children_list = []
    while True:
        while children_list:
            pid, status = os.waitpid(0, os.WNOHANG)
            if not pid:
                break
            children_list.remove(pid)
        clisock, cliaddr = sock.accept()
        syslog.syslog("receive connection from"+ str(cliaddr))
        pid = os.fork()
        if pid:
            clisock.close()
            children_list.append(pid)
        else:
            sock.close()
            handle(clisock)
            os._exit(0)

if __name__ == "__main__":
    echo_process_server(8888)
