#!/bin/sh

if [ "$1" == '-i' ] ; then
  cp /tmp/_dev/out/pbkr /root/pbkr/
fi

gethw()
{
  DEV=$1
  NAME=$2
  #aplay -l|grep "device 0"|grep "$NAME"|sed "s/card \([0-9]*\).*$/hw:\1/"
  aplay -l|grep "^card .*$DEV.*device.*\[$NAME\]"|sed 's/^card *\([0-9]*\):.*device *\([0-9]\).*$/hw:\1,\2/'

}

I2S_DEV=$(gethw "sndrpihifiberry" "")
ALSA_DEV=$(gethw "ALSA")
HDMI_DEV=$(gethw "bcm2835 ALSA" "bcm2835 IEC958/HDMI")
HDPH_DEV=$(gethw "bcm2835 ALSA" "bcm2835 ALSA")
 
echo "I2S_DEV=$I2S_DEV"
echo "HDMI_DEV=$HDMI_DEV"
echo "HDPH_DEV=$HDPH_DEV"
for i in $( ps |grep pbkr |awk '{print $1;}'); do
kill $i 2>/dev/null
done

./pbkr -i $I2S_DEV $HDMI_DEV $HDPH_DEV 

