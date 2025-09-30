import network
import espnow
import time
import ubinascii
from machine import Pin

def macToString(mac):
    return ubinascii.hexlify(mac, ':').decode().upper()

receiver_mac = b'\x8C\xAA\xB5\x84\xDD\x74'
sender_mac = b'\xA8\x42\xe3\xc9\xe5\x4C'  # Sender MAC

    
try:
    #receiver_mac = b'\xff\xff\xff\xff\xff\xff' #broadcast

    # Initialize Wi-Fi in station mode
    sta = network.WLAN(network.STA_IF)
    sta.active(True)
    # Adresse MAC brute (type bytes)
    mac = sta.config('mac')

    print("Adresse MAC :", macToString(mac))

    #sta.config(channel=1)  # Set channel explicitly if packets are not delivered
    sta.disconnect()
    
    # Initialize ESP-NOW
    e = espnow.ESPNow()
    try:
        e.active(True)
    except OSError as err:
        print("Failed to initialize ESP-NOW:", err)
        raise
    
    if mac == receiver_mac:
        print("Device is Receiver")
        leds = [Pin(2, Pin.OUT), Pin(5, Pin.OUT)]
        def proc_rcv(msg, leds=leds):
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
                print("Erreur dans proc_rcv:", e)
                
        while True:
            try:
                # Receive message (host MAC, message, timeout of 100 ms)
                host, msg = e.recv(100)
                if msg:
                    proc_rcv(msg)
                
            except OSError as err:
                print("Error:", err)
                time.sleep(5)
                
            except KeyboardInterrupt:
                print("Stopping receiver...")
                e.active(False)
                sta.active(False)
                break

        
        
    elif mac == sender_mac:
        print("Device is Sender")
        # Add peer
        try:
            e.add_peer(receiver_mac)
        except OSError as err:
            print("Failed to add peer:", err)
            raise

        pin2 = Pin(2, Pin.OUT)
        def gpio16_irq(pin, e=e, pin2=pin2):
            p = pin.value()
            
            print("[{} ms] GPIO5 : {}".format(time.ticks_ms(), p))
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
    
        
    while True:
        time.sleep(1)
    
except Exception as e:
    print("Error", e)


