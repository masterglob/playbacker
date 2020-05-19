import socket
import sys
from binascii import unhexlify
from threading import Thread 

#UDP_IP = "192.168.22.1"
UDP_IP = "192.168.7.80"
UDP_PORT_IN = 8000
UDP_PORT_OUT = 9000

def align_str(s):
    x = bytes(s)
    while 1:
        x += b'%c'%0
        if (len(x) %4) == 0:
             return x
    
def midiMsg(port, status, channel, data1,data2):
    return bytes (unhexlify ("%02X%01X%01X%02X%02X"%(port,status,channel,data1,data2)))
    
'''
    exemple pour multitoggle:
    Open Sound Control Encoding
    Message: /4/multitoggle1/1/2 ,i
        Header
            Path: /4/multitoggle1/1/2
            Format: ,i
        Int32: 1


PING:
    Open Sound Control Encoding
    Message: /ping ,
        Header
            Path: /ping
            Format: ,

Fader:
    Open Sound Control Encoding
    Message: /1/volume8 ,f
        Header
            Path: /1/volume8
            Format: ,f
        Float: 0.5 ( bytes= 3F 00 00 00)


'''

class Receiver (Thread):
    def __init__(self):
        Thread.__init__(self)
        self.done = False
        self.start()
    def run(self):
        sock = socket.socket(socket.AF_INET, # Internet
                             socket.SOCK_DGRAM) # UDP
        sock.bind(("0.0.0.0",UDP_PORT_OUT)) 
        sock.settimeout(0.02)
        while not self.done:
            try:
                data, addr = sock.recvfrom(1024)
                if data:
                    print ("%s: [%s]" %(addr," ".join(["%02x"%c for c in data])))
            except socket.timeout: pass
    
def main_send():
        
    sock = socket.socket(socket.AF_INET, # Internet
                         socket.SOCK_DGRAM) # UDP

    x = bytes()
    x +=  align_str (bytes(sys.argv[1],'latin-1'))
    if (len (sys.argv) > 3):
        val = sys.argv[2]
        
        if val[0] in "0123456789": #
            x +=  align_str (b",i")
            i = int(sys.argv[2])
            v= b"%02X%02X%02X%02X"%((i >>24) &0xFF,
                                   (i >>16) &0xFF,
                                   (i >>8) &0xFF,
                                   (i >>0) &0xFF)
            x +=  v
        else:
            x +=  align_str (b",s")
            x +=  align_str (bytes(sys.argv[2],'latin-1'))
    
    sock.sendto(x, (UDP_IP, UDP_PORT_IN))
if __name__ == "__main__":
    if len(sys.argv) < 2: sys.argv=[sys.argv[0], "/ping"]
    try:
        receiver =  Receiver()
        main_send()
    finally:
        pass
    print ("\n")
    input()
    receiver.done = True
    receiver.join()