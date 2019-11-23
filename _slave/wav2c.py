
import sys, os

for f in sys.argv[1:]:
    
    f_next = os.path.splitext(f)[0]
    
    print ("const int16_t SAMPLES_%s[] = {"%f_next)
    idx=0;
    s = "    "
    with open(f, "rb") as f:
        f.seek(0x28)
        byte = f.read(4)
        size = int.from_bytes(byte, byteorder='little', signed=False)
        byte = f.read(2)
        size -= 2
        while byte != b"" and size > 0:
            # Do stuff with byte.
            s = s + ("0x%04X, "%int.from_bytes(byte, byteorder='little', signed=False))
            
            idx += 1
            if (idx > 15):
                idx = 0
                print (s)
                s = "    "
            
            # input()
            
            byte = f.read(2)
            size -= 2

    print ("%s\n};"%s)
    s = ""