#!/bin/sh

if [ "$1" == '-i' ] ; then
  cp /tmp/_dev/out/pbkr /root/pbkr/
fi

gethw()
{
  DEV=$1
  #aplay -l|grep "device 0"|grep "$NAME"|sed "s/card \([0-9]*\).*$/hw:\1/"
  aplay -l|grep "^card .*$DEV.*device"|sed 's/^card *\([0-9]*\):.*device *\([0-9]\).*$/hw:\1,\2/'
}


I2S_DEV="hw:CARD=sndrpihifiberry"
HDPH_DEV="hw:CARD=Headphones"
HDMI_DEV="hw:CARD=vc4hdmi"
for i in $( ps |grep pbkr |awk '{print $1;}'); do
kill $i 2>/dev/null
done

echo ./pbkr -i "'$I2S_DEV'" "'$HDMI_DEV'" "'$HDPH_DEV'"
./pbkr -i "$I2S_DEV" "$HDMI_DEV" "$HDPH_DEV" 
./lcd_display i +b -c -B e t "PBKR stopped"

