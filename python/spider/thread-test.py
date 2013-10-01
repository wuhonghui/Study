#coding:utf-8
import threading
import time

num = 0
num_lock = threading.Lock()

class Snake(threading.Thread):
    def run(self):
        global num
        global num_lock
        while not self.stoped:
            time.sleep(1)
            num_lock.acquire()
            num = num + 1
            print "Snake", self.name, "run", "num: ", str(num)
            num_lock.release()
    def stop(self):
        print "Snake", self.name, "stop"
        self.stoped = True
    def __init__ (self, name="Snake", maxsize=None, color=None, speed=None):
        threading.Thread.__init__(self)
        self.name = name
        self.maxsize = maxsize
        self.color = color
        self.speed = speed
        self.stoped = False

if __name__ == "__main__":
    a = Snake(name="Snake a", maxsize=100, color=0xffffff, speed=3)
    b = Snake(name="Snake b", maxsize=100, color=0xffffff, speed=3)

    a.start()
    b.start()

    print a.getName(), a.isAlive()
    print 'total thread count:' threading.activeCount()
    print b.getName(), a.isAlive()

    time.sleep(10)

    a.stop()
    b.stop()
    
    a.join()
    b.join()
    
