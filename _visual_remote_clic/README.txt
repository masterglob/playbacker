How to install & update remote clic:

Sender: 
- ESP32 D1 Mini
Receiver:
- Any ESP32-wroom based (D1 Mini will work)

Install (both cards have the same procedure, the MAC address will trigger expected function)
- Install micropython for ESP32 WROOM Generic (flash 'ESP32_GENERIC-20250911-v1.26.1.bin' using ESPTOOL with python)
- Use Thonny (for example):
  * Connect to COM device.
  * Create file "main.py" (on remote) and paste the content of main.py in this folder. This file will be automatically executed on power UP.


To replace the ESP devices (If any is changed, both must be updated)
- Load main.py on new board
- Using Thonny and the command line retreive the MAC address (displayed at startup)
- Update main.py (variable 'receiver_mac' and.or 'sender_mac') according to new value
- Upload again main.py on both boards.

How to use:
- Sender listens to GPIO5 and sends the GPIO status to Receiver using ESPNow
- Receiver listens on ESPNow and updates its own GPIO5 as output accordingly
- Both Sender and receiver also update LED2 (internal built-in led) accordingly input status.
- Note: Sender GPIO5 is connected (+R 10kOhm) on RPI 'GPIO_13_PIN33' (Visual Red Led for CLic feedback)
