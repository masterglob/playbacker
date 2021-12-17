cd /root/pbkr

gethw()
{
  DEV=$1
  NAME=$2
  aplay -l|grep "^card .*$DEV.*device.*\[$NAME\]"|sed 's/^card *\([0-9]*\):.*device *\([0-9]\).*$/hw:\1,\2/'
}

while true ; do
    LAN_OK=false
    ifconfig wlan0 2>/dev/null >/dev/null && LAN_OK=true
    if ! "$LAN_OK" ; then
       echo "No WIFI started on"
       read
    else
       echo "WIFI OK"
    fi

    PCM_OK=false
    lsmod|grep snd_soc_pcm5102a >/dev/null  && PCM_OK=true
    aplay -l|grep -q snd_rpi_hifiberry_dac || PCM_OK=false
    if ! "$PCM_OK" ; then
       echo "No I2S connected"
       read
    else
       echo "I2S card OK"
    fi

	I2S_DEV=$(gethw "sndrpihifiberry" "")
	HDMI_DEV=$(gethw "bcm2835 ALSA" "bcm2835 IEC958/HDMI")
	HDPH_DEV=$(gethw "bcm2835 ALSA" "bcm2835 ALSA")
    echo "Starting PBKR $I2S_DEV $HDMI_DEV $HDPH_DEV" |tee /dev/kmsg
    ./pbkr $I2S_DEV $HDMI_DEV $HDPH_DEV
    echo "PBKR stopped unexpectedly" |tee /dev/kmsg
    sleep 1
    echo "Press enter to restart"
    #read i
done




