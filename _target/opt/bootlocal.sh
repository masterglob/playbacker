#!/bin/sh

# Start serial terminal
# /usr/sbin/startserialtty &

# Set CPU frequency governor to ondemand (default is performance)
echo ondemand > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

# Load modules
/sbin/modprobe i2c-dev
/sbin/modprobe snd_soc_pcm5102a

# ------ Put other system startup commands below this line
loadkmap < /usr/share/kmap/azerty/fr-latin1.kmap
/usr/local/etc/init.d/openssh start &

route add default gw 192.168.1.1 eth0
ifconfig eth0 192.168.7.80 netmask 255.255.255.0
ifconfig wlan0 up
ifconfig wlan0 192.168.22.1 netmask 255.255.255.0
/usr/local/bin/hostapd -B /usr/local/etc/hostapd.conf
udhcpd /etc/udhcpd.conf

ld environment
mkdir -p /tmp/_dev
chmod 777 /tmp/_dev

# mkfifo /tmp/virt_kbd

# configure serial link
stty -F /dev/ttyAMA0  115200 -evenp

# start application
(sleep 5 && cd /root/pbkr && ./pbkr.sh)
