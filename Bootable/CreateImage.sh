BASEDIR="$(cd -P "$(dirname "$0")" && pwd)"

rm -f -r "$BASEDIR/Firedrake.iso"
rm -f -r "$BASEDIR/image"

mkdir -p "$BASEDIR/image/boot/grub/i386-pc"

cp "$BASEDIR/grub.cfg" "$BASEDIR/image/boot/grub/grub.cfg"
cp "$BASEDIR/../Kernel/firedrake" "$BASEDIR/image/boot/firedrake"

grub-mkrescue --modules="linux ext2 fshelp ls boot pc" --output="$BASEDIR/Firedrake.iso" "$BASEDIR/image"

rm -f -r "$BASEDIR/image"