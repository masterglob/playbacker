
# Usage : pbkr_cpy.sh <source> <dest> <name> <opt>
# copy project <name> from  <source> to  <dest>
# existing project with same name will be merged
#
# <source> and <dest> should be "/mnt/usb/PBKR" or "/mnt/mmcblk0p2/pbkr.projects"
#
# <opt> = -r to replace
# <opt> = -c to complete (and do not files with same name)
# otherwise : merge (and replace files with same name)

SRC="$1"
DEST="$2"
NAME="$3"
OPT="$4"

[ "" != "${SRC}"  ] || exit 1
[ "" != "${DEST}" ] || exit 2
[ "" != "${NAME}" ] || exit 3
[ "$SRC" != "${DEST}" ] || exit 4
! [ -d  "${SRC}/${NAME}" ] &&  exit 5
PATH_NOK=true
echo "${DEST}" | grep -q "^/mnt/usb" && PATH_NOK=false
echo "${DEST}" | grep -q "^/mnt/mmcblk0p2" && PATH_NOK=false
${PATH_NOK} && exit 6

PATH_NOK=true
echo "${SRC}" | grep -q "^/mnt/usb" && PATH_NOK=false
echo "${SRC}" | grep -q "^/mnt/mmcblk0p2" && PATH_NOK=false
${PATH_NOK} && exit 7

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

cp_p()
{
        strace -q -ewrite=1 cp -- "$1" "$2" 2>&1| \
        awk '{if ($1 ~ /^sendfile64/ )
                        {
                                count += $NF;
                                printf  "%3.0f %3.0f\n",(count/1024)/1024, total_size  >> "/tmp/progress"
                                system("");
                        }
                }
                END { print "" }' total_size=$MBYTE_TOTAL count=$BYTES_DONE
}

#Seeach for wav files
cd "${SRC}/${NAME}"
ls -1 |grep -i -q "[.]wav$" ||exit 8

mkdir -p "${DEST}/${NAME}"

NB_FILES=0
BYTES_TOTAL=0
for f in *.wav *.WAV *.Wav *.WAv *.wAV *.waV ;
do
        [ -f "$f" ] || continue
        NB_FILES=$((NB_FILES + 1))
        BYTES_TOTAL=$((BYTES_TOTAL + $(stat -c '%s' "${f}") ))
done
MBYTE_TOTAL=$((BYTES_TOTAL / 1024 / 1024))

echo "$NB_FILES files to copy ($MBYTE_TOTAL MB)"

BYTES_DONE=0
for f in *.wav *.WAV *.Wav *.WAv *.wAV *.waV ;
do
        [ -f "$f" ] || continue
        if [ "$OPT" = "-c" ] && [ -f "${DEST}/${NAME}/$f" ] ; then
                echo "skip existing file $f"
                continue
        fi
        echo "Copying <$f>"
    # rsync --info=progress2 "$f" "${DEST}/${NAME}" ||exit 9
        cp_p "$f" "${DEST}/${NAME}" ||exit 9
        cp "$f.title" "$f.track" "${DEST}/${NAME}"||exit 10
        BYTES_DONE=$((BYTES_DONE + $(stat -c '%s' "${f}") ))
        #break
done

exit 0
