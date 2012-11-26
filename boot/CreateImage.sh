#!/bin/bash
BASEDIR="$(cd -P "$(dirname "$0")" && pwd)"
runobjdump=true

## Arrays
programs=(hellostatic)
libraries=()
drivers=(libkernel libio libPCI libRTL8139)

## Misc
grubpath="$BASEDIR/image/boot/grub/grub.cfg"

## Setup the image folder
rm -f -r "$BASEDIR/Firedrake.iso"
rm -f -r "$BASEDIR/image"

mkdir -p "$BASEDIR/image/boot/grub/i386-pc"
mkdir -p "$BASEDIR/image/modules"

## Create the grub.cfg
echo -e -n "GRUB_DEFAULT=0\nGRUB_TIMEOUT=1\n\nmenuentry \"Firedrake\" {\n\tmultiboot /boot/firedrake\n\tmodule /boot/firedrake firedrake\n" > $grubpath

## Copy programs
for i in ${programs[@]}; do
	echo -e -n "\tmodule /modules/${i}.bin ${i}.bin\n" >> $grubpath
	cp "$BASEDIR/../bin/${i}/${i}.bin" "$BASEDIR/image/modules/${i}.bin"

	if [ $runobjdump = true ]; then
		objdump -d "$BASEDIR/../bin/${i}/${i}.bin" > "$BASEDIR/../bin/${i}/dump.txt"
	fi
done

## Copy libraries
for i in ${libraries[@]}; do
	echo -e -n "\tmodule /modules/${i}.so ${i}.so\n" >> $grubpath
	cp "$BASEDIR/../lib/${i}/${i}.so" "$BASEDIR/image/modules/${i}.so"

	if [ $runobjdump = true ]; then
		objdump -d "$BASEDIR/../lib/${i}/${i}.so" > "$BASEDIR/../lib/${i}/dump.txt"
	fi
done

## Copy drivers
for i in ${drivers[@]}; do
	echo -e -n "\tmodule /modules/${i}.so ${i}.so\n" >> $grubpath
	cp "$BASEDIR/../libkernel/${i}/${i}.so" "$BASEDIR/image/modules/${i}.so"

	if [ $runobjdump = true ]; then
		objdump -d "$BASEDIR/../libkernel/${i}/${i}.so" > "$BASEDIR/../libkernel/${i}/dump.txt"
	fi
done

## Finish the grub.cfg and copy the kernel
echo -e -n "}" >> $grubpath
cp "$BASEDIR/../sys/firedrake" "$BASEDIR/image/boot/firedrake"

if [ $runobjdump = true ]; then
	objdump -d "$BASEDIR/../sys/firedrake" > "$BASEDIR/../sys/dump.txt"
fi

## Create the image
grub-mkrescue --modules="ext2 fshelp boot pc" --output="$BASEDIR/Firedrake.iso" "$BASEDIR/image"

## Remove the temporary image folder
rm -f -r "$BASEDIR/image"
