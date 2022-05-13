#!/bin/sh

# Usage : pbkr_cpy.sh <source> <dest> <name> <opt>
# copy project <name> from  <source> to  <dest>
# existing project with same name will be merged
#
# <source> and <dest> should be "/mnt/usb/PBKR" or "/mnt/mmcblk0p2/pbkr.projects"
#
# <opt> = -r to replace
# <opt> = -c to complete (and do not files with same name)
# otherwise : merge (and replace files with same name)


process () {

SRC="$1"
DEST="$2"
NAME="$3"
OPT="$4"
echo "SRC=<$SRC> DEST=<$DEST> NAME=<$NAME>"
[ "" != "${SRC}"  ] || return 1
[ "" != "${DEST}" ] || return 2
[ "" != "${NAME}" ] || return 3
[ "$SRC" != "${DEST}" ] || return 4
! [ -d  "${SRC}/${NAME}" ] &&  return 5
PATH_NOK=true
echo "${DEST}" | grep -q "^/mnt/usb" && PATH_NOK=false
echo "${DEST}" | grep -q "^/mnt/mmcblk0p2" && PATH_NOK=false
${PATH_NOK} && return 6

PATH_NOK=true
echo "${SRC}" | grep -q "^/mnt/usb" && PATH_NOK=false
echo "${SRC}" | grep -q "^/mnt/mmcblk0p2" && PATH_NOK=false
${PATH_NOK} && return 7

if ! [ -d "${DEST}" ] ; then
        mkdir "${DEST}"
fi

if [ "$OPT" = "-r" ] ; then
        # Remove existing directory
        if [ -d "${DEST}/${NAME}" ] ; then
                echo "Delete existing project ${NAME} on ${DEST} "
                rm -rf "${DEST}/${NAME}/*"
        fi
fi

# Does not work anymore in latest piCore..
# cp_p()
# {
        # strace -q -ewrite=1 cp -- "$1" "$2" 2>&1| \
        # awk '{if ($1 ~ /^sendfile64/ )
                        # {
                                # count += $NF;
                                # count_mb = (count/1024);
                                # printf  "File %d/%d (%3.0f%%)\n",FILE_IDX, NB_FILES, (100 * count_mb ) /total_size >> "/tmp/progress"
                                # system("");
                        # }
                # }
                # END { print "" }' total_size=$MBYTE_TOTAL count=$KBYTES_DONE NB_FILES=$NB_FILES FILE_IDX=$FILE_IDX
# }

#Seeach for wav files
cd "${SRC}/${NAME}"
ls -1 |grep -i -q "[.]wav$" || return 8

mkdir -p "${DEST}/${NAME}"

TO_PROCESS=$(ls -1 |grep -i 'wav$')
NB_FILES=0
BYTES_TOTAL=0
for f in $TO_PROCESS ;
do
        [ -f "$f" ] || continue
        if [ "$OPT" = "-c" ] && [ -f "${DEST}/${NAME}/$f" ] ; then
                echo "skip existing file $f" >> /tmp/pbkr_cpy.log
                continue
        fi
        NB_FILES=$((NB_FILES + 1))
        BYTES_TOTAL=$((BYTES_TOTAL + $(du -s $f | sed 's/\t.*//') ))
done
MBYTE_TOTAL=$((BYTES_TOTAL / 1024))

echo "$NB_FILES files to copy ($MBYTE_TOTAL MB)"
echo "NB_FILES=$NB_FILES : [$TO_PROCESS]" >> /tmp/pbkr_cpy.log
echo "BYTES_TOTAL=$BYTES_TOTAL kb" >> /tmp/pbkr_cpy.log

KBYTES_DONE=0
FILE_IDX=0
for f in $TO_PROCESS ;
do
        [ -f "$f" ] || continue
        if [ "$OPT" = "-c" ] && [ -f "${DEST}/${NAME}/$f" ] ; then
                continue
        fi
        FILE_IDX=$((FILE_IDX + 1))
		echo "Copying <$f> ..." >> /tmp/pbkr_cpy.log
    # rsync --info=progress2 "$f" "${DEST}/${NAME}" ||exit 9
		echo "Copy $f ..." >> /tmp/progress
		# remove possibly already existing extensions...
		rm -f -- "${DEST}/${NAME}/$f*" 2> /dev/null
        cp -f -- "$f" "${DEST}/${NAME}" || exit 9
		echo "Check $f ..." >> /tmp/progress
		sync
		echo "... OK" >> /tmp/pbkr_cpy.log
		for ext in $(ls -1 |grep -i "$f"'.*') ;
		do
			echo "Copy $ext ..." >> /tmp/pbkr_cpy.log
			cp -f -- "$ext" "${DEST}/${NAME}" || exit 10
		done
        KBYTES_DONE=$((KBYTES_DONE + $(du -s $f | sed 's/\t.*//') ))
        #break
done

return 0
}

echo $* > /tmp/pbkr_cpy.log

process "$1" "$2" "$3" "$4"
r=$?
echo "Result of $0 $*: $r" >> /tmp/pbkr_cpy.log
echo "Result of $0 $*:"
echo "$r"
exit $r