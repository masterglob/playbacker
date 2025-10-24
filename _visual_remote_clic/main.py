import network
import time
import ubinascii
import machine
from machine import Pin

RCV_TIMEOUT = 120
INTERNAL_LED=2
FLASH_LED=4

# Error codes:
EC_MAC_FAILED = 3
EC_UNKNOWN_ROLE = 4
EC_RCV_EXCPT = 5

#receiver_mac = b'\x8C\xAA\xB5\x84\xDD\x74'
# sender_mac = b'\xA8\x42\xe3\xc9\xe5\x4C'  # Sender MAC

# receiver_mac = '8CAAB584DAA8'
receiver_mac = '8CAAB584DD74'
sender_mac = 'A842E3C9E54C'

def nbLedFlash(n, leds):
    for _ in range(n):
        for led in leds:led.value(1)
        time.sleep(1)
        for led in leds:led.value(0)
        time.sleep(0.1)
    
led = Pin(INTERNAL_LED, Pin.OUT)
try:
    print("Starting SW")

    mac = ubinascii.hexlify(network.WLAN().config('mac')).decode().upper() 
    print("Adresse MAC :", (mac))

except Exception as E:
    print(E)
    nbLedFlash(EC_MAC_FAILED, [led])
    time.sleep(5)
    machine.reset()
    
led.value(1)

try:
    import espnow
    # Initialize ESP-NOW
    e = espnow.ESPNow()
    try:
        e.active(True)
    except OSError as err:
        ledOUT = Pin(INTERNAL_LED, Pin.OUT)
        ledOUT.value(1)
        raise Exception(f"Failed to initialize ESP-NOW:{err}")
    
    if mac == receiver_mac:
        print("Init Seq")
        leds = [Pin(INTERNAL_LED, Pin.OUT), Pin(FLASH_LED, Pin.OUT)]
        
        for i in range(10):
            for led in leds: led.value(1)
            time.sleep(0.1)
            for led in leds: led.value(0)
            time.sleep(0.05)
            
        for led in leds: led.value(1)
    
        print("Device is Receiver")
        lastRcv=time.time()
        def proc_rcv(msg, leds=leds):
            global lastRcv
            lastRcv=time.time()
            try:
                # Vérifie que le message est bien au format attendu
                if len(msg) == 3 and msg[0] == 0xEF and msg[1] == 0xDC:
                    etat = msg[2]   # Troisième octet = 0 ou 1
                    if etat:
                        print(">>> Signal = HIGH")
                        for led in leds:led.value(1)
                    else:
                        print(">>> Signal = LOW")
                        for led in leds:led.value(0)
                else:
                    print("Message inattendu :", msg)
            except Exception as e:
                nbLedFlash(EC_RCV_EXCPT, leds)
                time.sleep(5)
                machine.reset()
                
        while True:
            try:
                # Receive message (host MAC, message, timeout of 100 ms)
                host, msg = e.recv(100)
                if msg:
                    proc_rcv(msg)
                if time.time() - lastRcv > RCV_TIMEOUT:
                    print("RcvTimeout")
                    lastRcv=time.time()
                    for led in leds: led.value(1)
                
            except OSError as err:
                print("Error:", err)
                time.sleep(5)
                
            except KeyboardInterrupt:
                print("Stopping receiver...")
                e.active(False)
                break

        
        
    elif mac == sender_mac:
        print("Device is Sender")
        # Add peer
        try:
            e.add_peer(receiver_mac)
        except OSError as err:
            print("Failed to add peer:", err)
            raise

        pin2 = Pin(INTERNAL_LED, Pin.OUT)
        def gpio16_irq(pin, e=e, pin2=pin2):
            p = pin.value()
            print("[{} ms] GPIO16 : {}".format(time.ticks_ms(), p))
            if p:
                msg = b"\xEF\xDC\x01"
                pin2.value(1)
            else:
                msg = b"\xEF\xDC\x00"
                pin2.value(0)
            # Send the message without acknowledgment
            try:
                if not e.send(receiver_mac, msg, False):
                    print("Failed to send message (send returned False)")
            except OSError as err:
                print(f"Failed to send message (OSError: {err})")
            
        pin23 = Pin(23, Pin.OUT)
        pin23.value(1)
        pin16 = Pin(16, Pin.IN, Pin.PULL_DOWN)
        pin16.irq(trigger=Pin.IRQ_RISING|Pin.IRQ_FALLING, handler=gpio16_irq)

    else:
        print("Device is not recognized as Sender nor Receiver")
        led = Pin(INTERNAL_LED, Pin.OUT)
        nbLedFlash(EC_UNKNOWN_ROLE, [led])
        time.sleep(5)
        machine.reset()
        
        
    while True:
        time.sleep(1)

except KeyboardInterrupt:
    # Permet à Thonny de stopper le script avec CTRL+C
    print("CTRL+C")
    raise

except Exception as e:
    print (f"Exception:{e}")
    time.sleep(5)





