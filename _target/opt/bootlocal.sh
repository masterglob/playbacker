#!/bin/sh

# To start serial terminal /w console
# Add the following to the cmdline.txt console=serial0,115200
# Reference https://www.raspberrypi.org/documentation/configuration/uart.md for UART configuration
# Uncomment the next line
# /usr/sbin/startserialtty &

# Set CPU frequency governor to ondemand (default is performance)
echo ondemand > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

# Load modules - i2c-dev needs manually loaded even if enabled in config.txt
/sbin/modprobe i2c-dev
/sbin/modprobe snd_soc_pcm5102a

loadkmap < /usr/share/kmap/azerty/fr-latin1.kmap

if [ -f /var/run/udhcpc.eth0.pid ]; then
kill `cat /var/run/udhcpc.eth0.pid`
sleep 0.1
fi

# Start openssh daemon
/usr/local/etc/init.d/openssh start &

route add default gw 192.168.1.1 eth0
ifconfig wlan0 up
ifconfig wlan0 192.168.22.1 netmask 255.255.255.0
#/usr/local/bin/hostapd -B /usr/local/etc/hostapd.conf
#udhcpd /etc/udhcpd.conf

ifconfig eth0 192.168.7.80 netmask 255.255.255.0

# ------ Put other system startup commands below this line
ls environment
mkdir -p /tmp/_dev
chmod 777 /tmp/_dev

# mkfifo /tmp/virt_kbd

mkdir -p /home/www/res
rm -f /home/www/cmd 2>/dev/null
mkfifo /home/www/cmd
chmod -R 777 /home/www

# configure serial link
stty -F /dev/ttyAMA0  115200 -evenp

#httpd
#echo "" > /usr/local/apache2/htdocs/index.html

ifconfig

# start application
cd /root/pbkr && ./pbkr.sh

