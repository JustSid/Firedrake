#!/bin/bash
BASEDIR=$(dirname $0)

if [ $# -eq 0 ]; then
    qemu -net nic,model=rtl8139 -net user "$BASEDIR/boot/Firedrake.iso"
    exit 0
fi

if [ "$@" == "--debug" ]; then
	qemu -d int,cpu_reset -net nic,model=rtl8139 -net user -no-reboot -no-shutdown "$BASEDIR/boot/Firedrake.iso"
	exit 0
fi