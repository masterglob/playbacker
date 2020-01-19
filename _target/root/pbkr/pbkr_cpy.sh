
# Usage : pbkr_cpy.sh <source> <dest> <name>
# copy project <name> from  <source> to  <dest>
# existing project with same name is deleted
#
# <source> and <dest> should be "/mnt/usb/PBKR" or "/mnt/mmcblk0p2/pbkr.projects"

SRC="$1"
DEST="$2"
NAME="$3"
[ "" != "$SRC" ] || exit 1
[ "" != "$DEST" ] || exit 2
[ "" != "$NAME" ] || exit 3
[ "$SRC" != "$DEST" ] || exit 4

echo "$SRC/$NAME"
 [ ! -d  "$SRC/$NAME" ] || exit 5