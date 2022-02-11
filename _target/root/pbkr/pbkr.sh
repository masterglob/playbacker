cd /root/pbkr

gethw()
{
  DEV=$1
  #aplay -l|grep "device 0"|grep "$NAME"|sed "s/card \([0-9]*\).*$/hw:\1/"
  aplay -l|grep "^card .*$DEV.*device"|sed 's/^card *\([0-9]*\):.*device *\([0-9]\).*$/hw:\1,\2/'
}

while true ; do
    LAN_OK=false
    ifconfig wlan0 2>/dev/null >/dev/null && LAN_OK=true
    if ! "$LAN_OK" ; then
       echo "No WIFI started on"
       ./lcd_display i +b -c -B e t "!! No WIFI !!"
       sleep 5
    else
       echo "WIFI OK"
    fi

    PCM_OK=false
    lsmod|grep snd_soc_pcm5102a >/dev/null  && PCM_OK=true
    aplay -l|grep -q snd_rpi_hifiberry_dac || PCM_OK=false
    if ! "$PCM_OK" ; then
       echo "I2S driver missing"
       ./lcd_display i +b -c -B t "I2S driver missing"
       read
    else
       echo "I2S card OK"
    fi

	I2S_DEV=$(gethw "sndrpihifiberry" "")
	HDMI_DEV=$(gethw "bcm2835 HDMI 1")
	HDPH_DEV=$(gethw "Headphones")
    echo "Starting PBKR '$I2S_DEV' '$HDMI_DEV' '$HDPH_DEV'" |tee /dev/kmsg
    ./pbkr "$I2S_DEV" "$HDMI_DEV" "$HDPH_DEV"
    echo "PBKR stopped unexpectedly" |tee /dev/kmsg
   ./lcd_display i +b -c -B e t "PBKR restarting..."
	sleep 1
    echo "Press enter to restart"
    #read i
done




