#ifndef _pbkr_config_h_
#define _pbkr_config_h_

#include "pbkr_version.h"
/*
 HW configuration:
      Raspberry 3 B

 J8 GPIOs : ([x] = GPIOx as numbered by WiringPi)
                    -------
             3.3V -| 1   2 |- 5V
     I2C1 SDA [8] -| 3   4 |- 5V
     I2C1 SCL [9] -| 5   6 |- GND
     GPCLK0   [7] -| 7   8 |- [15] (UART TX)
              GND -| 9  10 |- [16] (UART RX)
USER I2C POW [ 0] -| 11 12 |- [1] I2S BCK
USER BTN1    [ 2] -| 13 14 |- GND
USER LED1    [ 3] -| 15 16 |- [4]
             3.3V -| 17 18 |- [5]
    SPI MOSI [12] -| 19 20 |- GND
    SPI MISO [13] -| 21 22 |- [6]
    SPI SCLK [14] -| 23 24 |- [10] SPI_CE0
              GND -| 25 26 |- [11] SPI_CE1
             [30] -| 27 28 |- [31]
             [21] -| 29 30 |- GND
             [22] -| 31 32 |- [26]
             [23] -| 33 34 |- GND
    I2S LRCK [24] -| 35 36 |- [27]
             [25] -| 37 38 |- [28] (I2S IN)
              GND -| 39 40 |- [29] (I2S DOUT)
                    -------

- I2C:
   SDA = PIN3
   SCL = PIN5
   +5V = [17] PIN 11 => allow SW to control (start/stop) the I2C display.
         (Not working, maybe not enoough current. need a relay?


================
NETWORK
================
(installation hostapd)

eth0      Link encap:Ethernet  HWaddr B8:27:EB:88:E2:F1
          inet addr:192.168.7.80  Bcast:192.168.7.255  Mask:255.255.255.0

lo        Link encap:Local Loopback
          inet addr:127.0.0.1  Mask:255.0.0.0

wlan0     Link encap:Ethernet  HWaddr B8:27:EB:DD:B7:A4
          inet addr:192.168.22.1  Bcast:192.168.22.255  Mask:255.255.255.0



 */

#define USB_MOUNT_POINT "/mnt/usb/PBKR"
#define INTERNAL_MOUNT_POINT "/mnt/mmcblk0p2"
#define WEB_ROOT_PATH "/home/www"
#endif
