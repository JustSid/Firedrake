#!/bin/bash
BASEDIR=$(dirname $0)
QEMU_NET="-net nic,model=rtl8139 -net user"

if [ $# -eq 0 ]; then
	qemu-system-i386 $QEMU_NET "$BASEDIR/boot/Firedrake.iso"
	exit 0
fi

if [ "$@" == "--debug" ]; then
	qemu-system-i386 -d int,cpu_reset -no-reboot -no-shutdown $QEMU_NET "$BASEDIR/boot/Firedrake.iso"
	exit 0
fi